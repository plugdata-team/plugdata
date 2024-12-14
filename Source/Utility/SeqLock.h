// SeqLock by Timur Doumler
// Thread-safe data structure for variables that are not compatible with atomics
// Lock-free, and store is even wait-free

#pragma once

template<typename T>
class SeqLock {
public:
    static_assert(std::is_trivially_copyable_v<T>);

    // Creates a seqlock_object with a default-constructed value.
    SeqLock()
    {
        store(T());
    }

    // Creates a seqlock_object with the given value.
    SeqLock(T t)
    {
        store(t);
    }

    // Reads and returns the current value.
    // Non-blocking guarantees: wait-free if there are no concurrent writes,
    // otherwise none.
    T load() const noexcept
    {
        T t;
        while (!try_load(t)) /* keep trying */
            ;
        return t;
    }

    // Attempts to read the current value and write it into the passed-in object.
    // Returns: true if the read succeeded, false otherwise.
    // Non-blocking guarantees: wait-free.
    bool try_load(T& t) const noexcept
    {
        std::size_t const seq1 = seq.load(std::memory_order_acquire);
        if (seq1 % 2 != 0)
            return false;

        atomic_load_per_byte_memcpy(&t, &data, sizeof(data), std::memory_order_acquire);

        std::size_t seq2 = seq.load(std::memory_order_relaxed);
        if (seq1 != seq2)
            return false;

        return true;
    }

    // Updates the current value to the value passed in.
    // Non-blocking guarantees: wait-free.
    void store(T t) noexcept
    {
        // Note: load + store usually has better performance characteristics than fetch_add(1)
        std::size_t const old_seq = seq.load(std::memory_order_relaxed);
        seq.store(old_seq + 1, std::memory_order_relaxed);

        atomic_store_per_byte_memcpy(&data, &t, sizeof(data), std::memory_order_release);

        seq.store(old_seq + 2, std::memory_order_release);
    }

private:
    char data[sizeof(T)];
    std::atomic<std::size_t> seq = 0;
    static_assert(decltype(seq)::is_always_lock_free);

    // These are implementations of the corresponding functions
    // atomic_load/store_per_byte_memcpy from the Concurrency TS 2.
    // They behave as if the source and dest bytes respectively
    // were individual atomic objects.
    // The implementations provided below is portable, but slow.
    // PRs with platform-optimised versions are welcome :)
    // The implementations provided below are also *technically*
    // UB because C++ does not let us loop over the bytes of
    // an object representation, but that is a known wording bug that
    // will be fixed by P1839; the technique should work on any
    // major compiler.

    // Preconditions:
    // - order is std::memory_order::acquire or std::memory_order::relaxed
    // - (char*)dest + [0, count) and (const char*)source + [0, count)
    //   are valid ranges that do not overlap
    // Effects:
    //   Copies count consecutive bytes pointed to by source into consecutive
    //   bytes pointed to by dest. Each individual load operation from a source
    //   byte is atomic with memory order order. These individual loads are
    //   unsequenced with respect to each other.
    inline void* atomic_load_per_byte_memcpy(void* dest, void const* source, size_t const count, std::memory_order order) const
    {
        assert(order == std::memory_order_acquire || order == std::memory_order_relaxed);

        char* dest_bytes = reinterpret_cast<char*>(dest);
        char const* src_bytes = reinterpret_cast<char const*>(source);

        for (std::size_t i = 0; i < count; ++i) {

#if defined(__clang__) || defined(__GNUC__) || defined(__GNUG__)
            dest_bytes[i] = __atomic_load_n(src_bytes + i, __ATOMIC_RELAXED);
#elif __cpp_lib_atomic_ref
            dest_bytes[i] = std::atomic_ref<char>(src_bytes[i]).load(std::memory_order_relaxed);
#else
            static_assert(false);
            // No atomic_ref or equivalent functionality available on this platform!
#endif
        }

        std::atomic_thread_fence(order);

        return dest;
    }

    // Preconditions:
    // - order is std::memory_order::release or std::memory_order::relaxed
    // - (char*)dest + [0, count) and (const char*)source + [0, count)
    //   are valid ranges that do not overlap
    // Effects:
    //   Copies count consecutive bytes pointed to by source into consecutive
    //   bytes pointed to by dest. Each individual store operation to a
    //   destination byte is atomic with memory order order. These individual
    //   stores are unsequenced with respect to each other.
    static inline void* atomic_store_per_byte_memcpy(void* dest, void const* source, size_t const count, std::memory_order order)
    {
        assert(order == std::memory_order_release || order == std::memory_order_relaxed);

        std::atomic_thread_fence(order);

        char* dest_bytes = reinterpret_cast<char*>(dest);
        char const* src_bytes = reinterpret_cast<char const*>(source);

        for (size_t i = 0; i < count; ++i) {
#if defined(__clang__) || defined(__GNUC__) || defined(__GNUG__)
            __atomic_store_n(dest_bytes + i, src_bytes[i], __ATOMIC_RELAXED);
#elif __cpp_lib_atomic_ref
            std::atomic_ref<char>(dest_bytes[i]).store(src_bytes[i], std::memory_order_relaxed);
#else
            static_assert(false);
            // No atomic_ref or equivalent functionality available on this platform!
#endif
        }

        return dest;
    }
};

template<typename T, typename = void>
struct AtomicStorageSelector {
    using type = SeqLock<T>; // Default to SeqLock if std::atomic<T> is not available
};

template<typename T>
struct AtomicStorageSelector<T, std::void_t<std::atomic<T>>> {
    using type = std::atomic<T>; // Use std::atomic<T> if it's available
};

enum AtomicMemoryOrder {
    Relaxed,
    ReleaseOrAcquire,
    Sequential
};

// The main Atomic class that selects the appropriate storage type (std::atomic or SeqLock)
template<typename T, AtomicMemoryOrder MemoryOrder = ReleaseOrAcquire>
class AtomicValue {
    // Select either std::atomic<T> or SeqLock<T> based on type T
    using AtomicStorageType = typename AtomicStorageSelector<T>::type;
    AtomicStorageType storage_value;

public:
    AtomicValue()
        : storage_value(T())
    {
    }

    AtomicValue(T initial_value)
        : storage_value(initial_value)
    {
    }

    T load() const noexcept
    {
        if constexpr (std::is_same_v<AtomicStorageType, std::atomic<T>>) {
            return storage_value.load(getReadOrder());
        } else {
            return storage_value.load();
        }
    }

    void store(T new_value) noexcept
    {
        if constexpr (std::is_same_v<AtomicStorageType, std::atomic<T>>) {
            storage_value.store(new_value, getWriteOrder());
        } else {
            storage_value.store(new_value);
        }
    }

    T load(std::memory_order order) const noexcept
    {
        static_assert(std::is_same_v<AtomicStorageType, std::atomic<T>>);
        return storage_value.load(order);
    }

    void store(T new_value, std::memory_order order) noexcept
    {
        static_assert(std::is_same_v<AtomicStorageType, std::atomic<T>>);
        storage_value.store(new_value, order);
    }

    static constexpr std::memory_order getReadOrder()
    {
        if constexpr (MemoryOrder == Relaxed) {
            return std::memory_order_relaxed;
        } else if constexpr (MemoryOrder == ReleaseOrAcquire) {
            return std::memory_order_acquire;
        } else {
            return std::memory_order_seq_cst;
        }
    }

    static constexpr std::memory_order getWriteOrder()
    {
        if constexpr (MemoryOrder == Relaxed) {
            return std::memory_order_relaxed;
        } else if constexpr (MemoryOrder == ReleaseOrAcquire) {
            return std::memory_order_release;
        } else {
            return std::memory_order_seq_cst;
        }
    }

    bool compare_exchange_weak(T& expected, T desired) noexcept
    {
        return storage_value.compare_exchange_weak(expected, desired,
            getWriteOrder(),
            getReadOrder());
    }

    T exchange(T const& to_exchange) noexcept
    {
        return std::atomic_exchange_explicit(&storage_value, to_exchange, getWriteOrder());
    }

    // Implicit loading by converting Atomic to T
    operator T() const noexcept
    {
        return load();
    }

    // Implicit storing by assignment
    AtomicValue& operator=(T new_value) noexcept
    {
        store(new_value);
        return *this;
    }

    // Atomic increment (prefix ++)
    template<typename U = T>
    auto operator++() noexcept -> std::enable_if_t<std::is_same_v<AtomicStorageType, std::atomic<U>>>
    {
        return storage_value.fetch_add(1, getWriteOrder()) + 1;
    }

    // Atomic increment (postfix ++)
    template<typename U = T>
    auto operator++(int) noexcept -> std::enable_if_t<std::is_same_v<AtomicStorageType, std::atomic<U>>>
    {
        storage_value.fetch_add(1, getWriteOrder());
    }

    // Atomic decrement (prefix --)
    template<typename U = T>
    auto operator--() noexcept -> std::enable_if_t<std::is_same_v<AtomicStorageType, std::atomic<U>>>
    {
        storage_value.fetch_sub(1, getWriteOrder()) - 1;
    }

    // Atomic decrement (postfix --)
    template<typename U = T>
    auto operator--(int) noexcept -> std::enable_if_t<std::is_same_v<AtomicStorageType, std::atomic<U>>>
    {
        return storage_value.fetch_sub(1, getWriteOrder());
    }

    // Atomic increment with value (+=)
    template<typename U = T>
    auto operator+=(T increment) noexcept -> std::enable_if_t<std::is_same_v<AtomicStorageType, std::atomic<U>>>
    {
        storage_value.fetch_add(increment, getWriteOrder());
    }

    template<typename U = T>
    auto operator-=(T decrement) noexcept -> std::enable_if_t<std::is_same_v<AtomicStorageType, std::atomic<U>>>
    {
        storage_value.fetch_sub(decrement, getWriteOrder());
    }

    template<typename U = T>
    auto operator->() const noexcept -> std::enable_if_t<std::is_pointer_v<U>, U>
    {
        return load();
    }
};
