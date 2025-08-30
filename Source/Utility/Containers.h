#pragma once

/*
 This header contains various optimised containers for plugdata.

 - SmallArray<T, StackSize>: Small-size optimised replacement for std::vector. Use as a general purpose array if memory allocation is not desirable. Will allocate a 64-byte buffer by default, unless you specify a stacksize
     Be careful when assigning a small array to another smallarray with a smaller size.
     Based on LLVM's SmallVector

 - HeapArray<T>: Replacement for std::vector with nicer helper methods. Use if contents are always too large for stack allocation

 - StackArray<T, StackSize>: Replacement for std::array with nicer helper methods. Use if the size of the array is known at compile time

 - PooledPtrArray<T, StackSize>: replacement for juce::OwnedArray (or std::vector<std::unique_ptr>>) that alloates objects in a single block. Calling reserve() will also reserve space for the object, not just for the pointers. This should help against memory fragmentation, and will reduce the number of needed memory allocations. You can specify a StackSize if the implementation of T is known at compile time

 - UnorderedMap<T>: Fast replacement for std::unordered_map. The implementation is from ankerl::unordered_dense

 - UnorderedSegmentedMap<T>: Fast replacement for std::unordered_map that tries to prevent moving elements. Slower for iteration or lookups than UnorderedMap, but sometimes you can use this to prevent elements from being copied. The implementation is from ankerl::unordered_dense

 - UnorderedSet<T>: Fast replacement for std::unordered_set. The implementation is from ankerl::unordered_dense

 - PointerIntPair<PointerType, IntBits, IntType>: Data structure to store a (maximum 3-bit) integer in the lower bytes of a pointer. Use when high data throughput is needed.
     Based on LLVM's PointerIntPair
 */

#ifndef HAS_CPP_ATTRIBUTE
#    if defined(__cplusplus) && defined(__has_cpp_attribute)
#        define HAS_CPP_ATTRIBUTE(x) __has_cpp_attribute(x)
#    else
#        define HAS_CPP_ATTRIBUTE(x) 0
#    endif
#endif

#if HAS_CPP_ATTRIBUTE(gsl::Owner)
#    define GSL_OWNER [[gsl::Owner]]
#else
#    define GSL_OWNER
#endif

#if __has_builtin(__builtin_expect) || defined(__GNUC__)
#    define EXPECT_LIKELY(EXPR) __builtin_expect((bool)(EXPR), true)
#    define EXPECT_UNLIKELY(EXPR) __builtin_expect((bool)(EXPR), false)
#else
#    define EXPECT_LIKELY(EXPR) (EXPR)
#    define EXPECT_UNLIKELY(EXPR) (EXPR)
#endif

#if __has_attribute(returns_nonnull)
#    define ATTRIBUTE_RETURNS_NONNULL __attribute__((returns_nonnull))
#elif defined(_MSC_VER)
#    define ATTRIBUTE_RETURNS_NONNULL _Ret_notnull_
#else
#    define LLVM_ATTRIBUTE_RETURNS_NONNULL
#endif

#if defined(__has_feature)
#    if __has_feature(address_sanitizer)
#        include <sanitizer/asan_interface.h>
#        define ASAN_ENABLED 1
#    endif
#endif

#include <cassert>
#include <algorithm>
#include <limits>

template<typename IteratorT>
class iterator_range;

template<class Iterator>
using EnableIfConvertibleToInputIterator = std::enable_if_t<std::is_convertible<
    typename std::iterator_traits<Iterator>::iterator_category,
    std::input_iterator_tag>::value>;

/// This is all the stuff common to all SmallArrays.
///
/// The template parameter specifies the type which should be used to hold the
/// Size and Capacity of the SmallArray, so it can be adjusted.
/// Using 32 bit size is desirable to shrink the size of the SmallArray.
/// Using 64 bit size is desirable for cases like SmallArray<char>, where a
/// 32 bit size would limit the vector to ~4GB. SmallArrays are used for
/// buffering bitcode output - which can exceed 4GB.
template<class Size_T>
class SmallArrayBase {
protected:
    void* BeginX;
    Size_T Size = 0, Capacity;

    /// The maximum value of the Size_T used.
    static constexpr size_t SizeTypeMax()
    {
        return std::numeric_limits<Size_T>::max();
    }

    SmallArrayBase() = delete;
    SmallArrayBase(void* FirstEl, size_t TotalCapacity)
        : BeginX(FirstEl)
        , Capacity(static_cast<Size_T>(TotalCapacity))
    {
    }

    /// This is a helper for \a grow() that's out of line to reduce code
    /// duplication.  This function will report a fatal error if it can't grow at
    /// least to \p MinSize.
    void* mallocForGrow(void* FirstEl, size_t MinSize, size_t TSize,
        size_t& NewCapacity);

    /// This is an implementation of the grow() method which only works
    /// on POD-like data types and is out of line to reduce code duplication.
    /// This function will report a fatal error if it cannot increase capacity.
    void grow_pod(void* FirstEl, size_t MinSize, size_t TSize);

public:
    size_t size() const { return Size; }
    size_t capacity() const { return Capacity; }

    [[nodiscard]] bool empty() const { return !Size; }

    [[nodiscard]] bool not_empty() const { return Size; }

protected:
    /// Set the array size to \p N, which the current array must have enough
    /// capacity for.
    ///
    /// This does not construct or destroy any elements in the vector.
    void set_size(size_t N)
    {
        assert(N <= capacity()); // implies no overflow in assignment
        Size = static_cast<Size_T>(N);
    }

    /// Set the array data pointer to \p Begin and capacity to \p N.
    ///
    /// This does not construct or destroy any elements in the vector.
    //  This does not clean up any existing allocation.
    void set_allocation_range(void* Begin, size_t N)
    {
        assert(N <= SizeTypeMax());
        BeginX = Begin;
        Capacity = static_cast<Size_T>(N);
    }
};

template<class T>
using SmallArraySizeType = std::conditional_t<sizeof(T) < 4 && sizeof(void*) >= 8, uint64_t,
    uint32_t>;

/// Figure out the offset of the first element.
template<class T, typename = void>
struct SmallArrayAlignmentAndSize {
    alignas(SmallArrayBase<SmallArraySizeType<T>>) char Base[sizeof(
        SmallArrayBase<SmallArraySizeType<T>>)];
    alignas(T) char FirstEl[sizeof(T)];
};

/// This is the part of SmallArrayTemplateBase which does not depend on whether
/// the type T is a POD. The extra dummy template argument is used by ArrayRef
/// to avoid unnecessarily requiring T to be complete.
template<typename T, typename = void>
class SmallArrayTemplateCommon
    : public SmallArrayBase<SmallArraySizeType<T>> {
    using Base = SmallArrayBase<SmallArraySizeType<T>>;

protected:
    /// Find the address of the first element.  For this pointer math to be valid
    /// with small-size of 0 for T with lots of alignment, it's important that
    /// SmallArrayStorage is properly-aligned even for small-size of 0.
    void* getFirstEl() const
    {
        return const_cast<void*>(reinterpret_cast<void const*>(
            reinterpret_cast<char const*>(this) + offsetof(SmallArrayAlignmentAndSize<T>, FirstEl)));
    }
    // Space after 'FirstEl' is clobbered, do not add any instance vars after it.

    SmallArrayTemplateCommon(size_t Size)
        : Base(getFirstEl(), Size)
    {
    }

    void grow_pod(size_t MinSize, size_t TSize)
    {
        Base::grow_pod(getFirstEl(), MinSize, TSize);
    }

    /// Return true if this is a SmallArray which has not had dynamic
    /// memory allocated for it.
    bool isSmall() const { return this->BeginX == getFirstEl(); }

    /// Put this vector in a state of being small.
    void resetToSmall()
    {
        this->BeginX = getFirstEl();
        this->Size = this->Capacity = 0; // FIXME: Setting Capacity to 0 is suspect.
    }

    /// Return true if V is an internal reference to the given range.
    static bool isReferenceToRange(void const* V, void const* First, void const* Last)
    {
        // Use std::less to avoid UB.
        std::less<> LessThan;
        return !LessThan(V, First) && LessThan(V, Last);
    }

    /// Return true if V is an internal reference to this vector.
    bool isReferenceToStorage(void const* V) const
    {
        return isReferenceToRange(V, this->begin(), this->end());
    }

    /// Return true if First and Last form a valid (possibly empty) range in this
    /// vector's storage.
    bool isRangeInStorage(void const* First, void const* Last) const
    {
        // Use std::less to avoid UB.
        std::less<> LessThan;
        return !LessThan(First, this->begin()) && !LessThan(Last, First) && !LessThan(this->end(), Last);
    }

    /// Return true unless Elt will be invalidated by resizing the vector to
    /// NewSize.
    bool isSafeToReferenceAfterResize(void const* Elt, size_t NewSize)
    {
        // Past the end.
        if (EXPECT_LIKELY(!isReferenceToStorage(Elt)))
            return true;

        // Return false if Elt will be destroyed by shrinking.
        if (NewSize <= this->size())
            return Elt < this->begin() + NewSize;

        // Return false if we need to grow.
        return NewSize <= this->capacity();
    }

    /// Check whether Elt will be invalidated by resizing the vector to NewSize.
    void assertSafeToReferenceAfterResize(void const* Elt, size_t const NewSize)
    {
        assert(isSafeToReferenceAfterResize(Elt, NewSize) && "Attempting to reference an element of the vector in an operation "
                                                             "that invalidates it");
    }

    /// Check whether Elt will be invalidated by increasing the size of the
    /// vector by N.
    void assertSafeToAdd(void const* Elt, size_t N = 1)
    {
        this->assertSafeToReferenceAfterResize(Elt, this->size() + N);
    }

    /// Check whether any part of the range will be invalidated by clearing.
    void assertSafeToReferenceAfterClear(T const* From, T const* To)
    {
        if (From == To)
            return;
        this->assertSafeToReferenceAfterResize(From, 0);
        this->assertSafeToReferenceAfterResize(To - 1, 0);
    }
    template<
        class ItTy,
        std::enable_if_t<!std::is_same<std::remove_const_t<ItTy>, T*>::value,
            bool>
        = false>
    static void assertSafeToReferenceAfterClear(ItTy, ItTy) { }

    /// Check whether any part of the range will be invalidated by growing.
    void assertSafeToAddRange(T const* From, T const* To)
    {
        if (From == To)
            return;
        this->assertSafeToAdd(From, To - From);
        this->assertSafeToAdd(To - 1, To - From);
    }
    template<
        class ItTy,
        std::enable_if_t<!std::is_same<std::remove_const_t<ItTy>, T*>::value,
            bool>
        = false>
    static void assertSafeToAddRange(ItTy, ItTy) { }

    /// Reserve enough space to add one element, and return the updated element
    /// pointer in case it was a reference to the storage.
    template<class U>
    static T const* reserveForParamAndGetAddressImpl(U* This, T const& Elt,
        size_t N)
    {
        size_t NewSize = This->size() + N;
        if (EXPECT_LIKELY(NewSize <= This->capacity()))
            return &Elt;

        bool ReferencesStorage = false;
        int64_t Index = -1;
        if (!U::TakesParamByValue) {
            if (EXPECT_UNLIKELY(This->isReferenceToStorage(&Elt))) {
                ReferencesStorage = true;
                Index = &Elt - This->begin();
            }
        }
        This->grow(NewSize);
        return ReferencesStorage ? This->begin() + Index : &Elt;
    }

public:
    using size_type = size_t;
    using difference_type = ptrdiff_t;
    using value_type = T;
    using iterator = T*;
    using const_iterator = T const*;

    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using reverse_iterator = std::reverse_iterator<iterator>;

    using reference = T&;
    using const_reference = T const&;
    using pointer = T*;
    using const_pointer = T const*;

    using Base::capacity;
    using Base::empty;
    using Base::size;

    // forward iterator creation methods.
    iterator begin() { return static_cast<iterator>(this->BeginX); }
    const_iterator begin() const { return static_cast<const_iterator>(this->BeginX); }
    iterator end() { return begin() + size(); }
    const_iterator end() const { return begin() + size(); }

    // reverse iterator creation methods.
    reverse_iterator rbegin() { return reverse_iterator(end()); }
    const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
    reverse_iterator rend() { return reverse_iterator(begin()); }
    const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

    size_type size_in_bytes() const { return size() * sizeof(T); }
    size_type max_size() const
    {
        return std::min<size_type>(this->SizeTypeMax(), static_cast<size_type>(-1) / sizeof(T));
    }

    size_t capacity_in_bytes() const { return capacity() * sizeof(T); }

    /// Return a pointer to the vector's buffer, even if empty().
    pointer data() { return pointer(begin()); }
    /// Return a pointer to the vector's buffer, even if empty().
    const_pointer data() const { return const_pointer(begin()); }

    reference operator[](size_type idx)
    {
        assert(idx < size());
        return begin()[idx];
    }
    const_reference operator[](size_type idx) const
    {
        assert(idx < size());
        return begin()[idx];
    }

    reference front()
    {
        assert(!empty());
        return begin()[0];
    }
    const_reference front() const
    {
        assert(!empty());
        return begin()[0];
    }

    reference back()
    {
        assert(!empty());
        return end()[-1];
    }
    const_reference back() const
    {
        assert(!empty());
        return end()[-1];
    }
};

/// SmallArrayTemplateBase<TriviallyCopyable = false> - This is where we put
/// method implementations that are designed to work with non-trivial T's.
///
/// We approximate is_trivially_copyable with trivial move/copy construction and
/// trivial destruction. While the standard doesn't specify that you're allowed
/// copy these types with memcpy, there is no way for the type to observe this.
/// This catches the important case of std::pair<POD, POD>, which is not
/// trivially assignable.
template<typename T, bool = std::is_trivially_copy_constructible<T>::value && std::is_trivially_move_constructible<T>::value && std::is_trivially_destructible<T>::value>
class SmallArrayTemplateBase : public SmallArrayTemplateCommon<T> {
    friend class SmallArrayTemplateCommon<T>;

protected:
    static constexpr bool TakesParamByValue = false;
    using ValueParamT = T const&;

    SmallArrayTemplateBase(size_t Size)
        : SmallArrayTemplateCommon<T>(Size)
    {
    }

    static void destroy_range(T* S, T* E)
    {
        while (S != E) {
            --E;
            E->~T();
        }
    }

    /// Move the range [I, E) into the uninitialized memory starting with "Dest",
    /// constructing elements as needed.
    template<typename It1, typename It2>
    static void uninitialized_move(It1 I, It1 E, It2 Dest)
    {
        std::uninitialized_move(I, E, Dest);
    }

    /// Copy the range [I, E) onto the uninitialized memory starting with "Dest",
    /// constructing elements as needed.
    template<typename It1, typename It2>
    static void uninitialized_copy(It1 I, It1 E, It2 Dest)
    {
        std::uninitialized_copy(I, E, Dest);
    }

    /// Grow the allocated memory (without initializing new elements), doubling
    /// the size of the allocated memory. Guarantees space for at least one more
    /// element, or MinSize more elements if specified.
    void grow(size_t MinSize = 0);

    /// Create a new allocation big enough for \p MinSize and pass back its size
    /// in \p NewCapacity. This is the first section of \a grow().
    T* mallocForGrow(size_t MinSize, size_t& NewCapacity);

    /// Move existing elements over to the new allocation \p NewElts, the middle
    /// section of \a grow().
    void moveElementsForGrow(T* NewElts);

    /// Transfer ownership of the allocation, finishing up \a grow().
    void takeAllocationForGrow(T* NewElts, size_t NewCapacity);

    /// Reserve enough space to add one element, and return the updated element
    /// pointer in case it was a reference to the storage.
    T const* reserveForParamAndGetAddress(T const& Elt, size_t N = 1)
    {
        return this->reserveForParamAndGetAddressImpl(this, Elt, N);
    }

    /// Reserve enough space to add one element, and return the updated element
    /// pointer in case it was a reference to the storage.
    T* reserveForParamAndGetAddress(T& Elt, size_t N = 1)
    {
        return const_cast<T*>(
            this->reserveForParamAndGetAddressImpl(this, Elt, N));
    }

    static T&& forward_value_param(T&& V) { return std::move(V); }
    static T const& forward_value_param(T const& V) { return V; }

    void growAndAssign(size_t NumElts, T const& Elt)
    {
        // Grow manually in case Elt is an internal reference.
        size_t NewCapacity;
        T* NewElts = mallocForGrow(NumElts, NewCapacity);
        std::uninitialized_fill_n(NewElts, NumElts, Elt);
        this->destroy_range(this->begin(), this->end());
        takeAllocationForGrow(NewElts, NewCapacity);
        this->set_size(NumElts);
    }

    template<typename... ArgTypes>
    T& growAndEmplaceBack(ArgTypes&&... Args)
    {
        // Grow manually in case one of Args is an internal reference.
        size_t NewCapacity;
        T* NewElts = mallocForGrow(0, NewCapacity);
        ::new (static_cast<void*>(NewElts + this->size())) T(std::forward<ArgTypes>(Args)...);
        moveElementsForGrow(NewElts);
        takeAllocationForGrow(NewElts, NewCapacity);
        this->set_size(this->size() + 1);
        return this->back();
    }

    void push_back(T const& Elt)
    {
        T const* EltPtr = reserveForParamAndGetAddress(Elt);
        ::new (static_cast<void*>(this->end())) T(*EltPtr);
        this->set_size(this->size() + 1);
    }

    void push_back(T&& Elt)
    {
        T* EltPtr = reserveForParamAndGetAddress(Elt);
        ::new (static_cast<void*>(this->end())) T(::std::move(*EltPtr));
        this->set_size(this->size() + 1);
    }

    void pop_back()
    {
        this->set_size(this->size() - 1);
        this->end()->~T();
    }
};

// Define this out-of-line to dissuade the C++ compiler from inlining it.
template<typename T, bool TriviallyCopyable>
void SmallArrayTemplateBase<T, TriviallyCopyable>::grow(size_t const MinSize)
{
    size_t NewCapacity;
    T* NewElts = mallocForGrow(MinSize, NewCapacity);
    moveElementsForGrow(NewElts);
    takeAllocationForGrow(NewElts, NewCapacity);
}

template<typename T, bool TriviallyCopyable>
T* SmallArrayTemplateBase<T, TriviallyCopyable>::mallocForGrow(
    size_t MinSize, size_t& NewCapacity)
{
    return static_cast<T*>(
        SmallArrayBase<SmallArraySizeType<T>>::mallocForGrow(
            this->getFirstEl(), MinSize, sizeof(T), NewCapacity));
}

// Define this out-of-line to dissuade the C++ compiler from inlining it.
template<typename T, bool TriviallyCopyable>
void SmallArrayTemplateBase<T, TriviallyCopyable>::moveElementsForGrow(
    T* NewElts)
{
    // Move the elements over.
    this->uninitialized_move(this->begin(), this->end(), NewElts);

    // Destroy the original elements.
    destroy_range(this->begin(), this->end());
}

// Define this out-of-line to dissuade the C++ compiler from inlining it.
template<typename T, bool TriviallyCopyable>
void SmallArrayTemplateBase<T, TriviallyCopyable>::takeAllocationForGrow(
    T* NewElts, size_t NewCapacity)
{
    // If this wasn't grown from the inline copy, deallocate the old space.
    if (!this->isSmall())
        free(this->begin());

    this->set_allocation_range(NewElts, NewCapacity);
}

/// SmallArrayTemplateBase<TriviallyCopyable = true> - This is where we put
/// method implementations that are designed to work with trivially copyable
/// T's. This allows using memcpy in place of copy/move construction and
/// skipping destruction.
template<typename T>
class SmallArrayTemplateBase<T, true> : public SmallArrayTemplateCommon<T> {
    friend class SmallArrayTemplateCommon<T>;

protected:
    /// True if it's cheap enough to take parameters by value. Doing so avoids
    /// overhead related to mitigations for reference invalidation.
    static constexpr bool TakesParamByValue = sizeof(T) <= 2 * sizeof(void*);

    /// Either const T& or T, depending on whether it's cheap enough to take
    /// parameters by value.
    using ValueParamT = std::conditional_t<TakesParamByValue, T, T const&>;

    SmallArrayTemplateBase(size_t Size)
        : SmallArrayTemplateCommon<T>(Size)
    {
    }

    // No need to do a destroy loop for POD's.
    static void destroy_range(T*, T*) { }

    /// Move the range [I, E) onto the uninitialized memory
    /// starting with "Dest", constructing elements into it as needed.
    template<typename It1, typename It2>
    static void uninitialized_move(It1 I, It1 E, It2 Dest)
    {
        // Just do a copy.
        uninitialized_copy(I, E, Dest);
    }

    /// Copy the range [I, E) onto the uninitialized memory
    /// starting with "Dest", constructing elements into it as needed.
    template<typename It1, typename It2>
    static void uninitialized_copy(It1 I, It1 E, It2 Dest)
    {
        // Arbitrary iterator types; just use the basic implementation.
        std::uninitialized_copy(I, E, Dest);
    }

    /// Copy the range [I, E) onto the uninitialized memory
    /// starting with "Dest", constructing elements into it as needed.
    template<typename T1, typename T2>
    static void uninitialized_copy(
        T1* I, T1* E, T2* Dest,
        std::enable_if_t<std::is_same<std::remove_const_t<T1>, T2>::value>* = nullptr)
    {
        // Use memcpy for PODs iterated by pointers (which includes SmallArray
        // iterators): std::uninitialized_copy optimizes to memmove, but we can
        // use memcpy here. Note that I and E are iterators and thus might be
        // invalid for memcpy if they are equal.
        if (I != E)
            memcpy(reinterpret_cast<void*>(Dest), I, (E - I) * sizeof(T));
    }

    /// Double the size of the allocated memory, guaranteeing space for at
    /// least one more element or MinSize if specified.
    void grow(size_t MinSize = 0) { this->grow_pod(MinSize, sizeof(T)); }

    /// Reserve enough space to add one element, and return the updated element
    /// pointer in case it was a reference to the storage.
    T const* reserveForParamAndGetAddress(T const& Elt, size_t N = 1)
    {
        return this->reserveForParamAndGetAddressImpl(this, Elt, N);
    }

    /// Reserve enough space to add one element, and return the updated element
    /// pointer in case it was a reference to the storage.
    T* reserveForParamAndGetAddress(T& Elt, size_t N = 1)
    {
        return const_cast<T*>(
            this->reserveForParamAndGetAddressImpl(this, Elt, N));
    }

    /// Copy \p V or return a reference, depending on \a ValueParamT.
    static ValueParamT forward_value_param(ValueParamT V) { return V; }

    void growAndAssign(size_t NumElts, T Elt)
    {
        // Elt has been copied in case it's an internal reference, side-stepping
        // reference invalidation problems without losing the realloc optimization.
        this->set_size(0);
        this->grow(NumElts);
        std::uninitialized_fill_n(this->begin(), NumElts, Elt);
        this->set_size(NumElts);
    }

    template<typename... ArgTypes>
    T& growAndEmplaceBack(ArgTypes&&... Args)
    {
        // Use push_back with a copy in case Args has an internal reference,
        // side-stepping reference invalidation problems without losing the realloc
        // optimization.
        push_back(T(std::forward<ArgTypes>(Args)...));
        return this->back();
    }

public:
    void push_back(ValueParamT Elt)
    {
        T const* EltPtr = reserveForParamAndGetAddress(Elt);
        memcpy(reinterpret_cast<void*>(this->end()), EltPtr, sizeof(T));
        this->set_size(this->size() + 1);
    }

    void pop_back() { this->set_size(this->size() - 1); }
};

/// This class consists of common code factored out of the SmallArray class to
/// reduce code duplication based on the SmallArray 'N' template parameter.
template<typename T>
class SmallArrayImpl : public SmallArrayTemplateBase<T> {
    using SuperClass = SmallArrayTemplateBase<T>;

public:
    using iterator = typename SuperClass::iterator;
    using const_iterator = typename SuperClass::const_iterator;
    using reference = typename SuperClass::reference;
    using size_type = typename SuperClass::size_type;

protected:
    using SmallArrayTemplateBase<T>::TakesParamByValue;
    using ValueParamT = typename SuperClass::ValueParamT;

    // Default ctor - Initialize to empty.
    explicit SmallArrayImpl(unsigned N)
        : SmallArrayTemplateBase<T>(N)
    {
    }

    void assignRemote(SmallArrayImpl&& RHS)
    {
        this->destroy_range(this->begin(), this->end());
        if (!this->isSmall())
            free(this->begin());
        this->BeginX = RHS.BeginX;
        this->Size = RHS.Size;
        this->Capacity = RHS.Capacity;
        RHS.resetToSmall();
    }

    ~SmallArrayImpl()
    {
        // Subclass has already destructed this vector's elements.
        // If this wasn't grown from the inline copy, deallocate the old space.
        if (!this->isSmall())
            free(this->begin());
    }

public:
    SmallArrayImpl(SmallArrayImpl const&) = delete;

    template<typename U>
    [[nodiscard]] bool contains(U const& to_find) const
    {
        return std::find(this->begin(), this->end(), to_find) != this->end();
    }

    template<typename Predicate>
    [[nodiscard]] bool contains(T const& to_find, Predicate pred)
    {
        for (auto const& elt : *this) {
            if (pred(elt, to_find))
                return true;
        }

        return false;
    }

    template<typename U>
    [[nodiscard]] int index_of(U const& to_find) const
    {
        auto it = std::find(this->begin(), this->end(), to_find);
        return it == this->end() ? -1 : static_cast<int>(it - this->begin());
    }

    template<typename U>
    [[nodiscard]] int index_of_address(U const& to_find) const
    {
        auto it = std::find_if(this->begin(), this->end(),
            [&to_find](auto const& elem) { return &elem == &to_find; });
        return it == this->end() ? -1 : static_cast<int>(it - this->begin());
    }

    bool remove_one(T const& to_find)
    {
        auto it = std::find(this->begin(), this->end(), to_find);
        if (it != this->end()) {
            this->erase(it);
            return true;
        }
        return false; // Element not found
    }

    template<typename Predicate>
    bool remove_one(T const& to_find, Predicate pred)
    {
        auto it = std::find_if(this->begin(), this->end(), [&](T const& element) {
            return pred(element, to_find);
        });
        if (it != this->end()) {
            this->erase(it);
            return true;
        }
        return false; // Element not found
    }

    bool remove_all(T const& to_find)
    {
        auto initial_size = this->size();
        this->erase(std::remove(this->begin(), this->end(), to_find), this->end());
        return this->size() < initial_size; // True if at least one element was removed
    }

    bool remove_at(size_t index)
    {
        if (index < this->size()) {
            this->erase(this->begin() + index);
            return true;
        }
        return false; // Index out of bounds
    }

    bool remove_range(size_t start, size_t end)
    {
        if (start >= end || start >= this->size() || end > this->size()) {
            return false; // Invalid range
        }
        this->erase(this->begin() + start, this->begin() + end);
        return true;
    }

    bool add_unique(T const& to_add)
    {
        if (std::find(this->begin(), this->end(), to_add) == this->end()) {
            this->push_back(to_add);
            return true;
        }
        return false; // Element already exists
    }

    template<typename Predicate>
    bool add_unique(T const& to_add, Predicate pred)
    {
        if (std::find_if(this->begin(), this->end(), [&](T const& element) {
                return pred(element, to_add);
            })
            == this->end()) {
            this->push_back(to_add);
            return true;
        }
        return false; // Element already exists
    }

    void add(T const& to_add)
    {
        this->push_back(to_add);
    }

    template<typename U>
    void add_array(U const& array)
    {
        this->reserve(this->capacity() + array.size());
        for (auto const& elt : array)
            this->add(elt);
    }

    void pop()
    {
        this->pop_back();
    }

    int add_sorted(T const& value)
    {
        auto it = std::lower_bound(this->begin(), this->end(), value);
        auto next = this->insert(it, value);
        return std::distance(this->begin(), next);
    }

    int add_sorted(bool (*sort_fn)(T const&, T const&), T const& value)
    {
        auto it = std::lower_bound(this->begin(), this->end(), value, sort_fn);
        auto next = this->insert(it, value);
        return std::distance(this->begin(), next);
    }

    void sort()
    {
        std::sort(this->begin(), this->end());
    }

    void sort(std::function<bool(T const&, T const&)> sort_fn)
    {
        std::sort(this->begin(), this->end(), sort_fn);
    }

    void move(size_t from_index, size_t to_index)
    {
        if (from_index < to_index) {
            std::rotate(this->begin() + from_index, this->begin() + from_index + 1, this->begin() + to_index + 1);
        } else {
            std::rotate(this->begin() + to_index, this->begin() + from_index, this->begin() + from_index + 1);
        }
    }

    void clear()
    {
        this->destroy_range(this->begin(), this->end());
        this->Size = 0;
    }

private:
    // Make set_size() private to avoid misuse in subclasses.
    using SuperClass::set_size;

    template<bool ForOverwrite>
    void resizeImpl(size_type N)
    {
        if (N == this->size())
            return;

        if (N < this->size()) {
            this->truncate(N);
            return;
        }

        this->reserve(N);
        for (auto I = this->end(), E = this->begin() + N; I != E; ++I)
            if (ForOverwrite)
                new (&*I) T;
            else
                new (&*I) T();
        this->set_size(N);
    }

public:
    void resize(size_type N) { resizeImpl<false>(N); }

    /// Like resize, but \ref T is POD, the new values won't be initialized.
    void resize_for_overwrite(size_type N) { resizeImpl<true>(N); }

    /// Like resize, but requires that \p N is less than \a size().
    void truncate(size_type N)
    {
        assert(this->size() >= N && "Cannot increase size with truncate");
        this->destroy_range(this->begin() + N, this->end());
        this->set_size(N);
    }

    void resize(size_type N, ValueParamT NV)
    {
        if (N == this->size())
            return;

        if (N < this->size()) {
            this->truncate(N);
            return;
        }

        // N > this->size(). Defer to append.
        this->append(N - this->size(), NV);
    }

    void reserve(size_type N)
    {
        if (this->capacity() < N)
            this->grow(N);
    }

    void pop_back_n(size_type NumItems)
    {
        assert(this->size() >= NumItems);
        truncate(this->size() - NumItems);
    }

    [[nodiscard]] T pop_back_val()
    {
        T Result = ::std::move(this->back());
        this->pop_back();
        return Result;
    }

    void swap(SmallArrayImpl& RHS);

    /// Add the specified range to the end of the SmallArray.
    template<typename ItTy, typename = EnableIfConvertibleToInputIterator<ItTy>>
    void append(ItTy in_start, ItTy in_end)
    {
        this->assertSafeToAddRange(in_start, in_end);
        size_type NumInputs = std::distance(in_start, in_end);
        this->reserve(this->size() + NumInputs);
        this->uninitialized_copy(in_start, in_end, this->end());
        this->set_size(this->size() + NumInputs);
    }

    /// Append \p NumInputs copies of \p Elt to the end.
    void append(size_type NumInputs, ValueParamT Elt)
    {
        T const* EltPtr = this->reserveForParamAndGetAddress(Elt, NumInputs);
        std::uninitialized_fill_n(this->end(), NumInputs, *EltPtr);
        this->set_size(this->size() + NumInputs);
    }

    void append(std::initializer_list<T> IL)
    {
        append(IL.begin(), IL.end());
    }

    void append(SmallArrayImpl const& RHS) { append(RHS.begin(), RHS.end()); }

    void assign(size_type NumElts, ValueParamT Elt)
    {
        // Note that Elt could be an internal reference.
        if (NumElts > this->capacity()) {
            this->growAndAssign(NumElts, Elt);
            return;
        }

        // Assign over existing elements.
        std::fill_n(this->begin(), std::min<size_type>(NumElts, this->size()), Elt);
        if (NumElts > this->size())
            std::uninitialized_fill_n(this->end(), NumElts - this->size(), Elt);
        else if (NumElts < this->size())
            this->destroy_range(this->begin() + NumElts, this->end());
        this->set_size(NumElts);
    }

    // FIXME: Consider assigning over existing elements, rather than clearing &
    // re-initializing them - for all assign(...) variants.

    template<typename ItTy, typename = EnableIfConvertibleToInputIterator<ItTy>>
    void assign(ItTy in_start, ItTy in_end)
    {
        this->assertSafeToReferenceAfterClear(in_start, in_end);
        clear();
        append(in_start, in_end);
    }

    void assign(std::initializer_list<T> IL)
    {
        clear();
        append(IL);
    }

    void assign(SmallArrayImpl const& RHS) { assign(RHS.begin(), RHS.end()); }

    iterator erase(const_iterator CI)
    {
        // Just cast away constness because this is a non-const member function.
        iterator I = const_cast<iterator>(CI);

        assert(this->isReferenceToStorage(CI) && "Iterator to erase is out of bounds.");

        iterator N = I;
        // Shift all elts down one.
        std::move(I + 1, this->end(), I);
        // Drop the last elt.
        this->pop_back();
        return N;
    }

    iterator erase(const_iterator CS, const_iterator CE)
    {
        // Just cast away constness because this is a non-const member function.
        iterator S = const_cast<iterator>(CS);
        iterator E = const_cast<iterator>(CE);

        assert(this->isRangeInStorage(S, E) && "Range to erase is out of bounds.");

        iterator N = S;
        // Shift all elts down.
        iterator I = std::move(E, this->end(), S);
        // Drop the last elts.
        this->destroy_range(I, this->end());
        this->set_size(I - this->begin());
        return N;
    }

private:
    template<class ArgType>
    iterator insert_one_impl(iterator I, ArgType&& Elt)
    {
        // Callers ensure that ArgType is derived from T.
        static_assert(
            std::is_same<std::remove_const_t<std::remove_reference_t<ArgType>>,
                T>::value,
            "ArgType must be derived from T!");

        if (I == this->end()) { // Important special case for empty vector.
            this->push_back(::std::forward<ArgType>(Elt));
            return this->end() - 1;
        }

        assert(this->isReferenceToStorage(I) && "Insertion iterator is out of bounds.");

        // Grow if necessary.
        size_t Index = I - this->begin();
        std::remove_reference_t<ArgType>* EltPtr = this->reserveForParamAndGetAddress(Elt);
        I = this->begin() + Index;

        ::new (static_cast<void*>(this->end())) T(::std::move(this->back()));
        // Push everything else over.
        std::move_backward(I, this->end() - 1, this->end());
        this->set_size(this->size() + 1);

        // If we just moved the element we're inserting, be sure to update
        // the reference (never happens if TakesParamByValue).
        static_assert(!TakesParamByValue || std::is_same<ArgType, T>::value,
            "ArgType must be 'T' when taking by value!");
        if (!TakesParamByValue && this->isReferenceToRange(EltPtr, I, this->end()))
            ++EltPtr;

        *I = ::std::forward<ArgType>(*EltPtr);
        return I;
    }

public:
    iterator insert(size_t index, T const& value)
    {
        return this->insert(this->begin() + index, value);
    }

    iterator insert(iterator I, T&& Elt)
    {
        return insert_one_impl(I, this->forward_value_param(std::move(Elt)));
    }

    iterator insert(iterator I, T const& Elt)
    {
        return insert_one_impl(I, this->forward_value_param(Elt));
    }

    iterator insert(iterator I, size_type NumToInsert, ValueParamT Elt)
    {
        // Convert iterator to elt# to avoid invalidating iterator when we reserve()
        size_t InsertElt = I - this->begin();

        if (I == this->end()) { // Important special case for empty vector.
            append(NumToInsert, Elt);
            return this->begin() + InsertElt;
        }

        assert(this->isReferenceToStorage(I) && "Insertion iterator is out of bounds.");

        // Ensure there is enough space, and get the (maybe updated) address of
        // Elt.
        T const* EltPtr = this->reserveForParamAndGetAddress(Elt, NumToInsert);

        // Uninvalidate the iterator.
        I = this->begin() + InsertElt;

        // If there are more elements between the insertion point and the end of the
        // range than there are being inserted, we can use a simple approach to
        // insertion.  Since we already reserved space, we know that this won't
        // reallocate the vector.
        if (static_cast<size_t>(this->end() - I) >= NumToInsert) {
            T* OldEnd = this->end();
            append(std::move_iterator<iterator>(this->end() - NumToInsert),
                std::move_iterator<iterator>(this->end()));

            // Copy the existing elements that get replaced.
            std::move_backward(I, OldEnd - NumToInsert, OldEnd);

            // If we just moved the element we're inserting, be sure to update
            // the reference (never happens if TakesParamByValue).
            if (!TakesParamByValue && I <= EltPtr && EltPtr < this->end())
                EltPtr += NumToInsert;

            std::fill_n(I, NumToInsert, *EltPtr);
            return I;
        }

        // Otherwise, we're inserting more elements than exist already, and we're
        // not inserting at the end.

        // Move over the elements that we're about to overwrite.
        T* OldEnd = this->end();
        this->set_size(this->size() + NumToInsert);
        size_t NumOverwritten = OldEnd - I;
        this->uninitialized_move(I, OldEnd, this->end() - NumOverwritten);

        // If we just moved the element we're inserting, be sure to update
        // the reference (never happens if TakesParamByValue).
        if (!TakesParamByValue && I <= EltPtr && EltPtr < this->end())
            EltPtr += NumToInsert;

        // Replace the overwritten part.
        std::fill_n(I, NumOverwritten, *EltPtr);

        // Insert the non-overwritten middle part.
        std::uninitialized_fill_n(OldEnd, NumToInsert - NumOverwritten, *EltPtr);
        return I;
    }

    template<typename ItTy, typename = EnableIfConvertibleToInputIterator<ItTy>>
    iterator insert(iterator I, ItTy From, ItTy To)
    {
        // Convert iterator to elt# to avoid invalidating iterator when we reserve()
        size_t InsertElt = I - this->begin();

        if (I == this->end()) { // Important special case for empty vector.
            append(From, To);
            return this->begin() + InsertElt;
        }

        assert(this->isReferenceToStorage(I) && "Insertion iterator is out of bounds.");

        // Check that the reserve that follows doesn't invalidate the iterators.
        this->assertSafeToAddRange(From, To);

        size_t NumToInsert = std::distance(From, To);

        // Ensure there is enough space.
        reserve(this->size() + NumToInsert);

        // Uninvalidate the iterator.
        I = this->begin() + InsertElt;

        // If there are more elements between the insertion point and the end of the
        // range than there are being inserted, we can use a simple approach to
        // insertion.  Since we already reserved space, we know that this won't
        // reallocate the vector.
        if (static_cast<size_t>(this->end() - I) >= NumToInsert) {
            T* OldEnd = this->end();
            append(std::move_iterator<iterator>(this->end() - NumToInsert),
                std::move_iterator<iterator>(this->end()));

            // Copy the existing elements that get replaced.
            std::move_backward(I, OldEnd - NumToInsert, OldEnd);

            std::copy(From, To, I);
            return I;
        }

        // Otherwise, we're inserting more elements than exist already, and we're
        // not inserting at the end.

        // Move over the elements that we're about to overwrite.
        T* OldEnd = this->end();
        this->set_size(this->size() + NumToInsert);
        size_t NumOverwritten = OldEnd - I;
        this->uninitialized_move(I, OldEnd, this->end() - NumOverwritten);

        // Replace the overwritten part.
        for (T* J = I; NumOverwritten > 0; --NumOverwritten) {
            *J = *From;
            ++J;
            ++From;
        }

        // Insert the non-overwritten middle part.
        this->uninitialized_copy(From, To, OldEnd);
        return I;
    }

    void insert(iterator I, std::initializer_list<T> IL)
    {
        insert(I, IL.begin(), IL.end());
    }

    template<typename... ArgTypes>
    reference emplace_back(ArgTypes&&... Args)
    {
        if (EXPECT_UNLIKELY(this->size() >= this->capacity()))
            return this->growAndEmplaceBack(std::forward<ArgTypes>(Args)...);

        ::new (static_cast<void*>(this->end())) T(std::forward<ArgTypes>(Args)...);
        this->set_size(this->size() + 1);
        return this->back();
    }

    SmallArrayImpl& operator=(SmallArrayImpl const& RHS);

    SmallArrayImpl& operator=(SmallArrayImpl&& RHS);

    bool operator==(SmallArrayImpl const& RHS) const
    {
        if (this->size() != RHS.size())
            return false;
        return std::equal(this->begin(), this->end(), RHS.begin());
    }
    bool operator!=(SmallArrayImpl const& RHS) const
    {
        return !(*this == RHS);
    }

    bool operator<(SmallArrayImpl const& RHS) const
    {
        return std::lexicographical_compare(this->begin(), this->end(),
            RHS.begin(), RHS.end());
    }
    bool operator>(SmallArrayImpl const& RHS) const { return RHS < *this; }
    bool operator<=(SmallArrayImpl const& RHS) const { return !(*this > RHS); }
    bool operator>=(SmallArrayImpl const& RHS) const { return !(*this < RHS); }
};

template<typename T>
void SmallArrayImpl<T>::swap(SmallArrayImpl<T>& RHS)
{
    if (this == &RHS)
        return;

    // We can only avoid copying elements if neither vector is small.
    if (!this->isSmall() && !RHS.isSmall()) {
        std::swap(this->BeginX, RHS.BeginX);
        std::swap(this->Size, RHS.Size);
        std::swap(this->Capacity, RHS.Capacity);
        return;
    }
    this->reserve(RHS.size());
    RHS.reserve(this->size());

    // Swap the shared elements.
    size_t NumShared = this->size();
    if (NumShared > RHS.size())
        NumShared = RHS.size();
    for (size_type i = 0; i != NumShared; ++i)
        std::swap((*this)[i], RHS[i]);

    // Copy over the extra elts.
    if (this->size() > RHS.size()) {
        size_t EltDiff = this->size() - RHS.size();
        this->uninitialized_copy(this->begin() + NumShared, this->end(), RHS.end());
        RHS.set_size(RHS.size() + EltDiff);
        this->destroy_range(this->begin() + NumShared, this->end());
        this->set_size(NumShared);
    } else if (RHS.size() > this->size()) {
        size_t EltDiff = RHS.size() - this->size();
        this->uninitialized_copy(RHS.begin() + NumShared, RHS.end(), this->end());
        this->set_size(this->size() + EltDiff);
        this->destroy_range(RHS.begin() + NumShared, RHS.end());
        RHS.set_size(NumShared);
    }
}

template<typename T>
SmallArrayImpl<T>& SmallArrayImpl<T>::
operator=(SmallArrayImpl<T> const& RHS)
{
    // Avoid self-assignment.
    if (this == &RHS)
        return *this;

    // If we already have sufficient space, assign the common elements, then
    // destroy any excess.
    size_t RHSSize = RHS.size();
    size_t CurSize = this->size();
    if (CurSize >= RHSSize) {
        // Assign common elements.
        iterator NewEnd;
        if (RHSSize)
            NewEnd = std::copy(RHS.begin(), RHS.begin() + RHSSize, this->begin());
        else
            NewEnd = this->begin();

        // Destroy excess elements.
        this->destroy_range(NewEnd, this->end());

        // Trim.
        this->set_size(RHSSize);
        return *this;
    }

    // If we have to grow to have enough elements, destroy the current elements.
    // This allows us to avoid copying them during the grow.
    // FIXME: don't do this if they're efficiently moveable.
    if (this->capacity() < RHSSize) {
        // Destroy current elements.
        this->clear();
        CurSize = 0;
        this->grow(RHSSize);
    } else if (CurSize) {
        // Otherwise, use assignment for the already-constructed elements.
        std::copy(RHS.begin(), RHS.begin() + CurSize, this->begin());
    }

    // Copy construct the new elements in place.
    this->uninitialized_copy(RHS.begin() + CurSize, RHS.end(),
        this->begin() + CurSize);

    // Set end.
    this->set_size(RHSSize);
    return *this;
}

template<typename T>
SmallArrayImpl<T>& SmallArrayImpl<T>::operator=(SmallArrayImpl<T>&& RHS)
{
    // Avoid self-assignment.
    if (this == &RHS)
        return *this;

    // If the RHS isn't small, clear this vector and then steal its buffer.
    if (!RHS.isSmall()) {
        this->assignRemote(std::move(RHS));
        return *this;
    }

    // If we already have sufficient space, assign the common elements, then
    // destroy any excess.
    size_t RHSSize = RHS.size();
    size_t CurSize = this->size();
    if (CurSize >= RHSSize) {
        // Assign common elements.
        iterator NewEnd = this->begin();
        if (RHSSize)
            NewEnd = std::move(RHS.begin(), RHS.end(), NewEnd);

        // Destroy excess elements and trim the bounds.
        this->destroy_range(NewEnd, this->end());
        this->set_size(RHSSize);

        // Clear the RHS.
        RHS.clear();

        return *this;
    }

    // If we have to grow to have enough elements, destroy the current elements.
    // This allows us to avoid copying them during the grow.
    // FIXME: this may not actually make any sense if we can efficiently move
    // elements.
    if (this->capacity() < RHSSize) {
        // Destroy current elements.
        this->clear();
        CurSize = 0;
        this->grow(RHSSize);
    } else if (CurSize) {
        // Otherwise, use assignment for the already-constructed elements.
        std::move(RHS.begin(), RHS.begin() + CurSize, this->begin());
    }

    // Move-construct the new elements in place.
    this->uninitialized_move(RHS.begin() + CurSize, RHS.end(),
        this->begin() + CurSize);

    // Set end.
    this->set_size(RHSSize);

    RHS.clear();
    return *this;
}

/// Storage for the SmallArray elements.  This is specialized for the N=0 case
/// to avoid allocating unnecessary storage.
template<typename T, unsigned N>
struct SmallArrayStorage {
    alignas(T) char InlineElts[N * sizeof(T)];
};

/// We need the storage to be properly aligned even for small-size of 0 so that
/// the pointer math in \a SmallArrayTemplateCommon::getFirstEl() is
/// well-defined.
template<typename T>
struct alignas(T) SmallArrayStorage<T, 0> { };

/// Forward declaration of SmallArray so that
/// calculateSmallArrayDefaultInlinedElements can reference
/// `sizeof(SmallArray<T, 0>)`.
template<typename T, unsigned N>
class GSL_OWNER SmallArray;

/// Helper class for calculating the default number of inline elements for
/// `SmallArray<T>`.
///
/// This should be migrated to a constexpr function when our minimum
/// compiler support is enough for multi-statement constexpr functions.
template<typename T>
struct CalculateSmallArrayDefaultInlinedElements {
    // Parameter controlling the default number of inlined elements
    // for `SmallArray<T>`.
    //
    // The default number of inlined elements ensures that
    // 1. There is at least one inlined element.
    // 2. `sizeof(SmallArray<T>) <= kPreferredSmallArraySizeof` unless
    // it contradicts 1.
    static constexpr size_t kPreferredSmallArraySizeof = 64;

    // static_assert that sizeof(T) is not "too big".
    //
    // Because our policy guarantees at least one inlined element, it is possible
    // for an arbitrarily large inlined element to allocate an arbitrarily large
    // amount of inline storage. We generally consider it an antipattern for a
    // SmallArray to allocate an excessive amount of inline storage, so we want
    // to call attention to these cases and make sure that users are making an
    // intentional decision if they request a lot of inline storage.
    //
    // We want this assertion to trigger in pathological cases, but otherwise
    // not be too easy to hit. To accomplish that, the cutoff is actually somewhat
    // larger than kPreferredSmallArraySizeof (otherwise,
    // `SmallArray<SmallArray<T>>` would be one easy way to trip it, and that
    // pattern seems useful in practice).
    //
    // One wrinkle is that this assertion is in theory non-portable, since
    // sizeof(T) is in general platform-dependent. However, we don't expect this
    // to be much of an issue, because most LLVM development happens on 64-bit
    // hosts, and therefore sizeof(T) is expected to *decrease* when compiled for
    // 32-bit hosts, dodging the issue. The reverse situation, where development
    // happens on a 32-bit host and then fails due to sizeof(T) *increasing* on a
    // 64-bit host, is expected to be very rare.
    static_assert(
        sizeof(T) <= 512,
        "You are trying to use a default number of inlined elements for "
        "`SmallArray<T>` but `sizeof(T)` is really big! Please use an "
        "explicit number of inlined elements with `SmallArray<T, N>` to make "
        "sure you really want that much inline storage.");

    // Discount the size of the header itself when calculating the maximum inline
    // bytes.
    static constexpr size_t PreferredInlineBytes = kPreferredSmallArraySizeof - sizeof(SmallArray<T, 0>);
    static constexpr size_t NumElementsThatFit = PreferredInlineBytes / sizeof(T);
    static constexpr size_t value = NumElementsThatFit == 0 ? 1 : NumElementsThatFit;
};

/// This is a 'vector' (really, a variable-sized array), optimized
/// for the case when the array is small.  It contains some number of elements
/// in-place, which allows it to avoid heap allocation when the actual number of
/// elements is below that threshold.  This allows normal "small" cases to be
/// fast without losing generality for large inputs.
///
/// \note
/// In the absence of a well-motivated choice for the number of inlined
/// elements \p N, it is recommended to use \c SmallArray<T> (that is,
/// omitting the \p N). This will choose a default number of inlined elements
/// reasonable for allocation on the stack (for example, trying to keep \c
/// sizeof(SmallArray<T>) around 64 bytes).
///
/// \warning This does not attempt to be exception safe.
///
/// \see https://llvm.org/docs/ProgrammersManual.html#llvm-adt-SmallArray-h
template<typename T,
    unsigned N = CalculateSmallArrayDefaultInlinedElements<T>::value>
class GSL_OWNER SmallArray : public SmallArrayImpl<T>
    , SmallArrayStorage<T, N> {
public:
    SmallArray()
        : SmallArrayImpl<T>(N)
    {
    }

    ~SmallArray()
    {
        // Destroy the constructed elements in the vector.
        this->destroy_range(this->begin(), this->end());
    }

    explicit SmallArray(size_t Size)
        : SmallArrayImpl<T>(N)
    {
        this->resize(Size);
    }

    SmallArray(size_t Size, T const& Value)
        : SmallArrayImpl<T>(N)
    {
        this->assign(Size, Value);
    }

    template<typename ItTy, typename = EnableIfConvertibleToInputIterator<ItTy>>
    SmallArray(ItTy S, ItTy E)
        : SmallArrayImpl<T>(N)
    {
        this->append(S, E);
    }

    template<typename RangeTy>
    explicit SmallArray(iterator_range<RangeTy> const& R)
        : SmallArrayImpl<T>(N)
    {
        this->append(R.begin(), R.end());
    }

    SmallArray(std::initializer_list<T> IL)
        : SmallArrayImpl<T>(N)
    {
        this->append(IL);
    }

    SmallArray(SmallArray const& RHS)
        : SmallArrayImpl<T>(N)
    {
        if (!RHS.empty())
            SmallArrayImpl<T>::operator=(RHS);
    }

    SmallArray& operator=(SmallArray const& RHS)
    {
        SmallArrayImpl<T>::operator=(RHS);
        return *this;
    }

    SmallArray(SmallArray&& RHS)
        : SmallArrayImpl<T>(N)
    {
        if (!RHS.empty())
            SmallArrayImpl<T>::operator=(::std::move(RHS));
    }

    SmallArray(SmallArrayImpl<T>&& RHS)
        : SmallArrayImpl<T>(N)
    {
        if (!RHS.empty())
            SmallArrayImpl<T>::operator=(::std::move(RHS));
    }

    SmallArray& operator=(SmallArray&& RHS)
    {
        if (N) {
            SmallArrayImpl<T>::operator=(::std::move(RHS));
            return *this;
        }
        // SmallArrayImpl<T>::operator= does not leverage N==0. Optimize the
        // case.
        if (this == &RHS)
            return *this;
        if (RHS.empty()) {
            this->destroy_range(this->begin(), this->end());
            this->Size = 0;
        } else {
            this->assignRemote(std::move(RHS));
        }
        return *this;
    }

    SmallArray& operator=(SmallArrayImpl<T>&& RHS)
    {
        SmallArrayImpl<T>::operator=(::std::move(RHS));
        return *this;
    }

    SmallArray& operator=(std::initializer_list<T> IL)
    {
        this->assign(IL);
        return *this;
    }
};

template<typename T, unsigned N>
inline size_t capacity_in_bytes(SmallArray<T, N> const& X)
{
    return X.capacity_in_bytes();
}

template<typename RangeType>
using ValueTypeFromRangeType = std::remove_const_t<std::remove_reference_t<decltype(*std::begin(
    std::declval<RangeType&>()))>>;

/// Given a range of type R, iterate the entire range and return a
/// SmallArray with elements of the vector.  This is useful, for example,
/// when you want to iterate a range and then sort the results.
template<unsigned Size, typename R>
SmallArray<ValueTypeFromRangeType<R>, Size> to_vector(R&& Range)
{
    return { std::begin(Range), std::end(Range) };
}
template<typename R>
SmallArray<ValueTypeFromRangeType<R>> to_vector(R&& Range)
{
    return { std::begin(Range), std::end(Range) };
}

template<typename Out, unsigned Size, typename R>
SmallArray<Out, Size> to_vector_of(R&& Range)
{
    return { std::begin(Range), std::end(Range) };
}

template<typename Out, typename R>
SmallArray<Out> to_vector_of(R&& Range)
{
    return { std::begin(Range), std::end(Range) };
}

// Explicit instantiations
extern template class SmallArrayBase<uint32_t>;
#if SIZE_MAX > UINT32_MAX
extern template class SmallArrayBase<uint64_t>;
#endif

namespace std {

/// Implement std::swap in terms of SmallArray swap.
template<typename T>
inline void
swap(SmallArrayImpl<T>& LHS, SmallArrayImpl<T>& RHS)
{
    LHS.swap(RHS);
}

/// Implement std::swap in terms of SmallArray swap.
template<typename T, unsigned N>
inline void
swap(SmallArray<T, N>& LHS, SmallArray<T, N>& RHS)
{
    LHS.swap(RHS);
}

} // end namespace std

/// Report that MinSize doesn't fit into this vector's size type. Throws
/// std::length_error or calls report_fatal_error.
[[noreturn]] static void report_size_overflow(size_t MinSize, size_t MaxSize);
static void report_size_overflow(size_t const MinSize, size_t const MaxSize)
{
    std::string const Reason = "SmallArray unable to grow. Requested capacity (" + std::to_string(MinSize) + ") is larger than maximum value for size type (" + std::to_string(MaxSize) + ")";
    throw std::length_error(Reason);
}

/// Report that this vector is already at maximum capacity. Throws
/// std::length_error or calls report_fatal_error.
[[noreturn]] static void report_at_maximum_capacity(size_t MaxSize);
static void report_at_maximum_capacity(size_t const MaxSize)
{
    std::string const Reason = "SmallArray capacity unable to grow. Already at maximum size " + std::to_string(MaxSize);
    throw std::length_error(Reason);
}

// Note: Moving this function into the header may cause performance regression.
template<class Size_T>
static size_t getNewCapacity(size_t const MinSize, size_t TSize, size_t const OldCapacity)
{
    constexpr size_t MaxSize = std::numeric_limits<Size_T>::max();

    // Ensure we can fit the new capacity.
    // This is only going to be applicable when the capacity is 32 bit.
    if (MinSize > MaxSize)
        report_size_overflow(MinSize, MaxSize);

    // Ensure we can meet the guarantee of space for at least one more element.
    // The above check alone will not catch the case where grow is called with a
    // default MinSize of 0, but the current capacity cannot be increased.
    // This is only going to be applicable when the capacity is 32 bit.
    if (OldCapacity == MaxSize)
        report_at_maximum_capacity(MaxSize);

    // In theory 2*capacity can overflow if the capacity is 64 bit, but the
    // original capacity would never be large enough for this to be a problem.
    size_t const NewCapacity = 2 * OldCapacity + 1; // Always grow.
    return std::clamp(NewCapacity, MinSize, MaxSize);
}

ATTRIBUTE_RETURNS_NONNULL inline void* safe_malloc(size_t const Sz)
{
    void* Result = std::malloc(Sz);
    if (Result == nullptr) {
        // It is implementation-defined whether allocation occurs if the space
        // requested is zero (ISO/IEC 9899:2018 7.22.3). Retry, requesting
        // non-zero, if the space requested was zero.
        if (Sz == 0)
            return safe_malloc(1);

        throw std::bad_alloc();
    }
    return Result;
}

ATTRIBUTE_RETURNS_NONNULL inline void* safe_realloc(void* Ptr, size_t const Sz)
{
    void* Result = std::realloc(Ptr, Sz);
    if (Result == nullptr) {
        // It is implementation-defined whether allocation occurs if the space
        // requested is zero (ISO/IEC 9899:2018 7.22.3). Retry, requesting
        // non-zero, if the space requested was zero.
        if (Sz == 0)
            return safe_malloc(1);

        throw std::bad_alloc();
    }
    return Result;
}

/// If vector was first created with capacity 0, getFirstEl() points to the
/// memory right after, an area unallocated. If a subsequent allocation,
/// that grows the vector, happens to return the same pointer as getFirstEl(),
/// get a new allocation, otherwise isSmall() will falsely return that no
/// allocation was done (true) and the memory will not be freed in the
/// destructor. If a VSize is given (vector size), also copy that many
/// elements to the new allocation - used if realloca fails to increase
/// space, and happens to allocate precisely at BeginX.
/// This is unlikely to be called often, but resolves a memory leak when the
/// situation does occur.
static void* replaceAllocation(void* NewElts, size_t const TSize, size_t const NewCapacity,
    size_t const VSize = 0)
{
    void* NewEltsReplace = safe_malloc(NewCapacity * TSize);
    if (VSize)
        memcpy(NewEltsReplace, NewElts, VSize * TSize);
    free(NewElts);
    return NewEltsReplace;
}

// Note: Moving this function into the header may cause performance regression.
template<class Size_T>
void* SmallArrayBase<Size_T>::mallocForGrow(void* FirstEl, size_t const MinSize,
    size_t const TSize,
    size_t& NewCapacity)
{
    NewCapacity = getNewCapacity<Size_T>(MinSize, TSize, this->capacity());
    // Even if capacity is not 0 now, if the vector was originally created with
    // capacity 0, it's possible for the malloc to return FirstEl.
    void* NewElts = safe_malloc(NewCapacity * TSize);
    if (NewElts == FirstEl)
        NewElts = ::replaceAllocation(NewElts, TSize, NewCapacity);
    return NewElts;
}

// Note: Moving this function into the header may cause performance regression.
template<class Size_T>
void SmallArrayBase<Size_T>::grow_pod(void* FirstEl, size_t const MinSize,
    size_t const TSize)
{
    size_t const NewCapacity = getNewCapacity<Size_T>(MinSize, TSize, this->capacity());
    void* NewElts;
    if (BeginX == FirstEl) {
        NewElts = safe_malloc(NewCapacity * TSize);
        if (NewElts == FirstEl)
            NewElts = ::replaceAllocation(NewElts, TSize, NewCapacity);

        // Copy the elements over.  No need to run dtors on PODs.
        memcpy(NewElts, this->BeginX, size() * TSize);
    } else {
        // If this wasn't grown from the inline copy, grow the allocated space.
        NewElts = safe_realloc(this->BeginX, NewCapacity * TSize);
        if (NewElts == FirstEl)
            NewElts = ::replaceAllocation(NewElts, TSize, NewCapacity, size());
    }

    this->set_allocation_range(NewElts, NewCapacity);
}

template class SmallArrayBase<uint32_t>;

// Disable the uint64_t instantiation for 32-bit builds.
// Both uint32_t and uint64_t instantiations are needed for 64-bit builds.
// This instantiation will never be used in 32-bit builds, and will cause
// warnings when sizeof(Size_T) > sizeof(size_t).
#if SIZE_MAX > UINT32_MAX
template class SmallArrayBase<uint64_t>;

// Assertions to ensure this #if stays in sync with SmallArraySizeType.
static_assert(sizeof(SmallArraySizeType<char>) == sizeof(uint64_t),
    "Expected SmallArrayBase<uint64_t> variant to be in use.");
#else
static_assert(sizeof(SmallArraySizeType<char>) == sizeof(uint32_t),
    "Expected SmallArrayBase<uint32_t> variant to be in use.");
#endif

template<typename T>
class HeapArray {
public:
    using Iterator = typename std::vector<T>::iterator;

    HeapArray() = default;

    HeapArray(size_t size, T const& default_value)
        : data_(size, default_value)
    {
    }

    HeapArray(size_t size)
        : data_(size)
    {
    }

    HeapArray(std::initializer_list<T> init_list)
        : data_(init_list)
    {
    }

    HeapArray(Iterator start, Iterator end)
        : data_(start, end)
    {
    }

    HeapArray(HeapArray const& other)
        : data_(other.data_)
    {
    }

    // Adds an element if it doesn't already exist
    bool add_unique(T const& to_find)
    {
        if (std::find(data_.begin(), data_.end(), to_find) == data_.end()) {
            data_.push_back(to_find);
            return true;
        }
        return false; // Element already exists
    }

    // Remove the first occurrence of an element, if it exists
    bool remove_one(T const& to_find)
    {
        auto it = std::find(data_.begin(), data_.end(), to_find);
        if (it != data_.end()) {
            data_.erase(it);
            return true;
        }
        return false; // Element not found
    }

    // Remove all occurrences of an element
    bool remove_all(T const& to_find)
    {
        auto initial_size = data_.size();
        data_.erase(std::remove(data_.begin(), data_.end(), to_find), data_.end());
        return data_.size() < initial_size; // True if at least one element was removed
    }

    // Remove element at a specific index
    bool remove_at(size_t index)
    {
        if (index < data_.size()) {
            data_.erase(data_.begin() + index);
            return true;
        }
        return false;
    }

    bool remove_range(size_t start, size_t end)
    {
        if (start >= end || start >= data_.size() || end > data_.size()) {
            return false; // Invalid range
        }
        data_.erase(data_.begin() + start, data_.begin() + end);
        return true;
    }

    void add(T const& value) { data_.push_back(value); }

    template<typename U>
    void add_array(U const& array)
    {
        reserve(data_.size() + array.size());
        for (auto const& elt : array)
            add(elt);
    }

    template<typename... Args>
    void emplace_back(Args&&... args) { data_.emplace_back(std::forward<Args>(args)...); }

    int add_sorted(T const& value)
    {
        auto it = std::lower_bound(data_.begin(), data_.end(), value);
        auto next = data_.insert(it, value);
        return std::distance(data_.begin(), next);
    }

    size_t size() const { return data_.size(); }
    T& operator[](size_t index) { return data_[index]; }
    T const& operator[](size_t index) const { return data_[index]; }
    void clear() { data_.clear(); }
    T* data() { return data_.data(); }

    template<typename U>
    [[nodiscard]] bool contains(U const& to_find) const
    {
        return std::find(data_.begin(), data_.end(), to_find) != end();
    }

    template<typename U>
    [[nodiscard]] int index_of(U const& to_find) const
    {
        auto it = std::find(data_.begin(), data_.end(), to_find);
        return it == data_.end() ? -1 : static_cast<int>(it - data_.begin());
    }

    template<typename U>
    [[nodiscard]] int index_of_address(U const& to_find) const
    {
        auto it = std::find_if(data_.begin(), data_.end(),
            [&to_find](auto const& elem) { return &elem == &to_find; });
        return it == data_.end() ? -1 : static_cast<int>(it - data_.begin());
    }

    auto begin() { return data_.begin(); }
    auto end() { return data_.end(); }
    auto begin() const { return data_.begin(); }
    auto end() const { return data_.end(); }

    auto rbegin() { return data_.rbegin(); }
    auto rend() { return data_.rend(); }
    auto rbegin() const { return data_.rbegin(); }
    auto rend() const { return data_.rend(); }

    T& front() { return data_.front(); }
    T const& front() const { return data_.front(); }
    T& back() { return data_.back(); }
    T const& back() const { return data_.back(); }

    bool empty() const { return data_.empty(); }
    bool not_empty() const { return !data_.empty(); }

    auto& vector() { return data_; }
    void reserve(size_t capacity) { data_.reserve(capacity); }
    void resize(size_t capacity) { data_.resize(capacity); }
    void resize(size_t capacity, T const value) { data_.resize(capacity, value); }

    void erase(size_t index) { data_.erase(data_.begin() + index); }

    void move(size_t from_index, size_t to_index)
    {
        if (from_index < to_index) {
            std::rotate(data_.begin() + from_index, data_.begin() + from_index + 1, data_.begin() + to_index + 1);
        } else {
            std::rotate(data_.begin() + to_index, data_.begin() + from_index, data_.begin() + from_index + 1);
        }
    }

    void sort() { std::sort(data_.begin(), data_.end()); }

    void sort(int (*sort_fn)(T const&, T const&))
    {
        std::sort(data_.begin(), data_.end(), sort_fn);
    }

    void sort(int (*sort_fn)(T const, T const))
    {
        std::sort(data_.begin(), data_.end(), sort_fn);
    }

    void sort(std::function<int(T const&, T const&)> sort_fn)
    {
        std::sort(data_.begin(), data_.end(), sort_fn);
    }

    template<typename PredicateType>
    int remove_if(PredicateType&& predicate)
    {
        int num_removed = 0;
        for (int i = data_.size(); --i >= 0;) {
            if (predicate(data_[i])) {
                erase(i);
                ++num_removed;
            }
        }

        return num_removed;
    }

    Iterator erase(Iterator pos)
    {
        return data_.erase(pos);
    }

    Iterator erase(Iterator first, Iterator last)
    {
        return data_.erase(first, last);
    }

    void insert(size_t index, T const& value)
    {
        data_.insert(data_.begin() + index, value);
    }

    void insert(size_t index, size_t count, T const& value)
    {
        data_.insert(data_.begin() + index, count, value);
    }

    template<typename InputIt>
    void insert(size_t index, InputIt first, InputIt last)
    {
        data_.insert(data_.begin() + index, first, last);
    }

    Iterator insert(Iterator i, size_t num_to_insert, T const& elt)
    {
        return data_.insert(i, num_to_insert, elt);
    }

    template<typename ItTy, typename = EnableIfConvertibleToInputIterator<ItTy>>
    Iterator insert(Iterator i, ItTy from, ItTy to)
    {
        return data_.insert(i, from, to);
    }

private:
    std::vector<T> data_;
};

template<typename T, int N>
class StackArray {
public:
    using Iterator = typename std::array<T, N>::iterator;
    using ConstIterator = typename std::array<T, N>::const_iterator;

    std::array<T, N> data_;

    size_t size() const { return N; }

    T& operator[](size_t index)
    {
#if JUCE_DEBUG
        if (index >= N)
            throw std::out_of_range("Index out of bounds");
#endif
        return data_[index];
    }

    T const& operator[](size_t index) const
    {
#if JUCE_DEBUG
        if (index >= N)
            throw std::out_of_range("Index out of bounds");
#endif
        return data_[index];
    }

    void clear()
    {
        for (int i = 0; i < N; ++i) {
            data_[i] = T(); // Reset to default
        }
    }

    T* data() { return data_.data(); }

    template<typename U>
    [[nodiscard]] bool contains(U const& to_find) const
    {
        return std::find(data_.begin(), data_.end(), to_find) != data_.end();
    }

    template<typename U>
    [[nodiscard]] int index_of(U const& to_find) const
    {
        auto it = std::find(data_.begin(), data_.end(), to_find);
        return it == data_.end() ? -1 : static_cast<int>(it - data_.begin());
    }

    template<typename U>
    [[nodiscard]] int index_of_address(U const& to_find) const
    {
        auto it = std::find_if(data_.begin(), data_.end(),
            [&to_find](auto const& elem) { return &elem == &to_find; });
        return it == data_.end() ? -1 : static_cast<int>(it - data_.begin());
    }

    Iterator begin() { return data_.begin(); }
    Iterator end() { return data_.end(); }
    ConstIterator begin() const { return data_.cbegin(); }
    ConstIterator end() const { return data_.cend(); }

    T& front()
    {
#if JUCE_DEBUG
        if (N == 0)
            throw std::out_of_range("Array is empty");
#endif
        return data_[0];
    }

    T const& front() const
    {
#if JUCE_DEBUG
        if (N == 0)
            throw std::out_of_range("Array is empty");
#endif
        return data_[0];
    }

    T& back()
    {
#if JUCE_DEBUG
        if (N == 0)
            throw std::out_of_range("Array is empty");
#endif
        return data_[N - 1];
    }

    T const& back() const
    {
#if JUCE_DEBUG
        if (N == 0)
            throw std::out_of_range("Array is empty");
#endif
        return data_[N - 1];
    }

    // Move elements in the array
    void move(size_t from_index, size_t to_index)
    {
#if JUCE_DEBUG
        if (from_index >= N || to_index >= N) {
            throw std::out_of_range("Index out of bounds");
        }
#endif

        if (from_index < to_index) {
            std::rotate(data_.begin() + from_index, data_.begin() + from_index + 1, data_.begin() + to_index + 1);
        } else {
            std::rotate(data_.begin() + to_index, data_.begin() + from_index, data_.begin() + from_index + 1);
        }
    }

    // Sort methods
    void sort()
    {
        std::sort(data_.begin(), data_.end());
    }

    void sort(int (*sort_fn)(T const&, T const&))
    {
        std::sort(data_.begin(), data_.end(), sort_fn);
    }

    void sort(int (*sort_fn)(T const, T const))
    {
        std::sort(data_.begin(), data_.end(), sort_fn);
    }

    void sort(std::function<int(T const&, T const&)> sort_fn)
    {
        std::sort(data_.begin(), data_.end(), sort_fn);
    }

    void sort(std::function<int(T const, T const)> sort_fn)
    {
        std::sort(data_.begin(), data_.end(), sort_fn);
    }
};

// Array of owned object pointers. The pointers will all be allocated from few contiguous buffers. The size of the buffer can be set with BlocksPerChunk.
// If StackType and StackSize are defined, objects will first be allocated on stack before we allocate a heap buffer. This can be used to speed up memory access, since you can allocate the actual objects inside of the parent struct.
// To use a stack, the target object declaration needs to be known at construction time
template<typename T, int BlocksPerChunk = 32, int StackSize = 0>
class PooledPtrArray {
public:
    using Iterator = typename SmallArray<T*>::iterator;

    explicit PooledPtrArray() = default;

    ~PooledPtrArray()
    {
        clear(); // Ensure all owned objects are destroyed

        for (auto [ptr, size] : free_list) {
            allocator_.deallocate(ptr, size);
        }
    }

    // Remove methods
    bool remove_one(T const* to_find)
    {
        auto it = std::find_if(data_.begin(), data_.end(), [to_find](T* ptr) { return ptr == to_find; });
        if (it != data_.end()) {
            deallocate_and_destroy(*it);
            data_.erase(it);
            return true;
        }
        return false;
    }

    bool remove_at(size_t index)
    {
        if (index < data_.size()) {
            deallocate_and_destroy(data_[index]);
            data_.erase(data_.begin() + index);
            return true;
        }
        return false;
    }

    template<typename U>
    [[nodiscard]] int index_of(U const& to_find) const
    {
        auto it = std::find(this->begin(), this->end(), to_find);
        return it == this->end() ? -1 : static_cast<int>(it - this->begin());
    }

    template<typename... Args>
    T* add(Args&&... args)
    {
        data_.push_back(allocate_and_construct(std::forward<Args>(args)...));
        return data_.back();
    }

    template<typename U>
    [[nodiscard]] bool contains(U const& to_find) const
    {
        return std::find(this->begin(), this->end(), to_find) != this->end();
    }
    
    // Other necessary methods, simplified
    bool empty() const { return data_.empty(); }
    bool not_empty() const { return !data_.empty(); }
    size_t size() const { return data_.size(); }

    auto begin() { return data_.begin(); }
    auto end() { return data_.end(); }
    auto begin() const { return data_.begin(); }
    auto end() const { return data_.end(); }

    auto rbegin() { return data_.rbegin(); }
    auto rend() { return data_.rend(); }
    auto rbegin() const { return data_.rbegin(); }
    auto rend() const { return data_.rend(); }

    // Access methods
    T* front() { return data_.front(); }
    T const* front() const { return data_.front(); }
    T* back() { return data_.back(); }
    T const* back() const { return data_.back(); }

    T* operator[](size_t index) { return data_[index]; }
    T const* operator[](size_t index) const
    {
        return data_[index];
    }
    T* data() { return data_.data(); }

    // Clear all elements and deallocate them
    void clear()
    {
        for (auto ptr : data_) {
            deallocate_and_destroy(ptr);
        }
        data_.clear();
    }

    void reserve(size_t capacity)
    {
        data_.reserve(capacity);
        preallocate(std::max<int>(static_cast<int>(capacity) - size(), 0));
    }

    void erase(size_t index)
    {
        deallocate_and_destroy(data_[index]);
        data_.erase(data_.begin() + index);
    }

    void move(size_t from_index, size_t to_index)
    {
        if (from_index < to_index) {
            std::rotate(data_.begin() + from_index, data_.begin() + from_index + 1, data_.begin() + to_index + 1);
        } else {
            std::rotate(data_.begin() + to_index, data_.begin() + from_index, data_.begin() + from_index + 1);
        }
    }

    void sort() { std::sort(data_.begin(), data_.end()); }

    void sort(int (*sort_fn)(T const&, T const&))
    {
        std::sort(data_.begin(), data_.end(), sort_fn);
    }

    void sort(int (*sort_fn)(T const, T const))
    {
        std::sort(data_.begin(), data_.end(), sort_fn);
    }

    void sort(std::function<int(T const&, T const&)> sort_fn)
    {
        std::sort(data_.begin(), data_.end(), sort_fn);
    }

    template<typename PredicateType>
    int remove_if(PredicateType&& predicate)
    {
        int num_removed = 0;
        for (int i = data_.size(); --i >= 0;) {
            if (predicate(data_[i])) {
                erase(i);
                ++num_removed;
            }
        }

        return num_removed;
    }

    template<typename... Args>
    void insert(int index, Args&&... args)
    {
        data_.insert(index, allocate_and_construct(std::forward<Args>(args)...));
    }

private:
    // Helper method to allocate and construct objects using the memory resource
    template<typename... Args>
    T* allocate_and_construct(Args&&... args)
    {
        if constexpr (StackSize > 0) {
            if (stackUsed < StackSize) {
                T* ptr = reinterpret_cast<T*>(stackBuffer.data()) + stackUsed;
                stackUsed++;
                new (ptr) T(std::forward<Args>(args)...); // Placement new
                return ptr;
            }
        }
        if (reuse_list.not_empty()) {
            // Reuse an object from the free list
            T* ptr = reuse_list.back();
            reuse_list.pop();
#if ASAN_ENABLED
            __asan_unpoison_memory_region(ptr, sizeof(T));
#endif
            new (ptr) T(std::forward<Args>(args)...); // Placement new
            return ptr;
        }

        if (num_preallocated == 0)
            preallocate(BlocksPerChunk);
        num_preallocated--;
        T* ptr = preallocated++;
#if ASAN_ENABLED
        __asan_unpoison_memory_region(ptr, sizeof(T));
#endif
        new (ptr) T(std::forward<Args>(args)...);
        return ptr;
    }

    // Helper method to destroy and deallocate objects
    void deallocate_and_destroy(T* ptr)
    {
        if (ptr) {
            ptr->~T();
            reuse_list.add(ptr);
#if ASAN_ENABLED
            __asan_poison_memory_region(ptr, sizeof(T));
#endif
        }
    }

    void preallocate(int amount)
    {
        // Skip preallocation if we have enough preallocated already, or if we have enough freed objects to use
        if (amount <= num_preallocated || amount <= reuse_list.size())
            return;

        // If we already have preallocated elements, move them into the free list so they can be resused
        // We do this so that we guarantee all objects are in a large contiguous block when you call reserve
        reuse_list.reserve(reuse_list.size() + num_preallocated);
        for (int i = 0; i < num_preallocated; i++) {
            reuse_list.add(preallocated + i);
#if ASAN_ENABLED
            __asan_poison_memory_region(preallocated + i, sizeof(T));
#endif
        }

        num_preallocated = amount;
        preallocated = allocator_.allocate(amount);
        free_list.emplace_back(preallocated, amount);
    }

    bool check_contiguity() const
    {
        if (data_.empty()) {
            return true;
        }

        T* previous_ptr = data_.front();
        for (size_t i = 1; i < data_.size(); ++i) {
            T* current_ptr = data_[i];
            auto const gap = (reinterpret_cast<uintptr_t>(current_ptr) - (reinterpret_cast<uintptr_t>(previous_ptr) + sizeof(T))) / sizeof(T);
            std::cout << gap << std::endl;

            // Check if the current pointer is exactly one T away from the previous pointer
            if (gap != 0) {
                std::cout << "Pointers are not contiguous at index " << i << ": "
                          << "Current pointer: " << current_ptr << ", Previous pointer: " << previous_ptr << std::endl;
                return false; // Found a gap
            }
            previous_ptr = current_ptr;
        }

        return true; // All pointers are contiguous
    }

    SmallArray<T*> data_;
    std::allocator<T> allocator_;
    size_t num_preallocated = 0;
    T* preallocated;

    // Only initialise stack buffer if
    template<typename U, bool IsComplete = true>
    struct StorageSelector {
        using type = typename std::aligned_storage<sizeof(U), alignof(U)>::type;
    };

    template<typename U>
    struct StorageSelector<U, false> {
        using type = std::array<char, 1>;
    };

    using StackBuffer = typename StorageSelector<T, (StackSize > 0)>::type;

    std::array<StackBuffer, StackSize> stackBuffer;
    size_t stackUsed = 0;

    SmallArray<T*> reuse_list;
    SmallArray<std::pair<T*, int>> free_list;
};

#include "UnorderedMap.h"

template<typename Key, typename T>
using UnorderedMap = ankerl::unordered_dense::map<Key, T>;

template<typename Key, typename T>
using UnorderedSegmentedMap = ankerl::unordered_dense::segmented_map<Key, T>;

template<typename Key>
using UnorderedSet = ankerl::unordered_dense::set<Key>;

template<typename Key>
using UnorderedSegmentedSet = ankerl::unordered_dense::segmented_set<Key>;


/// A traits type that is used to handle pointer types and things that are just
/// wrappers for pointers as a uniform entity.
template<typename T>
struct PointerLikeTypeTraits;

/// A tiny meta function to compute the log2 of a compile time constant.
template<size_t N>
struct ConstantLog2
    : std::integral_constant<size_t, ConstantLog2<N / 2>::value + 1> { };
template<>
struct ConstantLog2<1> : std::integral_constant<size_t, 0> { };

// Provide a trait to check if T is pointer-like.
template<typename T, typename U = void>
struct HasPointerLikeTypeTraits {
    static constexpr bool value = false;
};

// sizeof(T) is valid only for a complete T.
template<typename T>
struct HasPointerLikeTypeTraits<
    T, decltype(sizeof(PointerLikeTypeTraits<T>) + sizeof(T), void())> {
    static constexpr bool value = true;
};

template<typename T>
struct IsPointerLike {
    static bool const value = HasPointerLikeTypeTraits<T>::value;
};

template<typename T>
struct IsPointerLike<T*> {
    static constexpr bool value = true;
};

// Provide PointerLikeTypeTraits for non-cvr pointers.
template<typename T>
struct PointerLikeTypeTraits<T*> {
    static inline void* getAsVoidPointer(T* P) { return P; }
    static inline T* getFromVoidPointer(void* P) { return static_cast<T*>(P); }

    static constexpr int NumLowBitsAvailable = ConstantLog2<alignof(T)>::value;
};

template<>
struct PointerLikeTypeTraits<void*> {
    static inline void* getAsVoidPointer(void* P) { return P; }
    static inline void* getFromVoidPointer(void* P) { return P; }

    /// Note, we assume here that void* is related to raw malloc'ed memory and
    /// that malloc returns objects at least 4-byte aligned. However, this may be
    /// wrong, or pointers may be from something other than malloc. In this case,
    /// you should specify a real typed pointer or avoid this template.
    ///
    /// All clients should use assertions to do a run-time check to ensure that
    /// this is actually true.
    static constexpr int NumLowBitsAvailable = 2;
};

// Provide PointerLikeTypeTraits for const things.
template<typename T>
struct PointerLikeTypeTraits<T const> {
    typedef PointerLikeTypeTraits<T> NonConst;

    static inline void const* getAsVoidPointer(T const P)
    {
        return NonConst::getAsVoidPointer(P);
    }
    static inline T getFromVoidPointer(void const* P)
    {
        return NonConst::getFromVoidPointer(const_cast<void*>(P));
    }
    static constexpr int NumLowBitsAvailable = NonConst::NumLowBitsAvailable;
};

// Provide PointerLikeTypeTraits for const pointers.
template<typename T>
struct PointerLikeTypeTraits<T const*> {
    typedef PointerLikeTypeTraits<T*> NonConst;

    static inline void const* getAsVoidPointer(T const* P)
    {
        return NonConst::getAsVoidPointer(const_cast<T*>(P));
    }
    static inline T const* getFromVoidPointer(void const* P)
    {
        return NonConst::getFromVoidPointer(const_cast<void*>(P));
    }
    static constexpr int NumLowBitsAvailable = NonConst::NumLowBitsAvailable;
};

// Provide PointerLikeTypeTraits for uintptr_t.
template<>
struct PointerLikeTypeTraits<uintptr_t> {
    static inline void* getAsVoidPointer(uintptr_t const P)
    {
        return reinterpret_cast<void*>(P);
    }
    static inline uintptr_t getFromVoidPointer(void* P)
    {
        return reinterpret_cast<uintptr_t>(P);
    }
    // No bits are available!
    static constexpr int NumLowBitsAvailable = 0;
};

/// Provide suitable custom traits struct for function pointers.
///
/// Function pointers can't be directly given these traits as functions can't
/// have their alignment computed with `alignof` and we need different casting.
///
/// To rely on higher alignment for a specialized use, you can provide a
/// customized form of this template explicitly with higher alignment, and
/// potentially use alignment attributes on functions to satisfy that.
template<int Alignment, typename FunctionPointerT>
struct FunctionPointerLikeTypeTraits {
    static constexpr int NumLowBitsAvailable = ConstantLog2<Alignment>::value;
    static inline void* getAsVoidPointer(FunctionPointerT P)
    {
        assert((reinterpret_cast<uintptr_t>(P) & ~(static_cast<uintptr_t>(-1) << NumLowBitsAvailable)) == 0 && "Alignment not satisfied for an actual function pointer!");
        return reinterpret_cast<void*>(P);
    }
    static inline FunctionPointerT getFromVoidPointer(void* P)
    {
        return reinterpret_cast<FunctionPointerT>(P);
    }
};

/// Provide a default specialization for function pointers that assumes 4-byte
/// alignment.
///
/// We assume here that functions used with this are always at least 4-byte
/// aligned. This means that, for example, thumb functions won't work or systems
/// with weird unaligned function pointers won't work. But all practical systems
/// we support satisfy this requirement.
template<typename ReturnT, typename... ParamTs>
struct PointerLikeTypeTraits<ReturnT (*)(ParamTs...)>
    : FunctionPointerLikeTypeTraits<4, ReturnT (*)(ParamTs...)> { };

template<typename Ptr>
struct PunnedPointer {
    static_assert(sizeof(Ptr) == sizeof(intptr_t), "");

    // Asserts that allow us to let the compiler implement the destructor and
    // copy/move constructors
    static_assert(std::is_trivially_destructible<Ptr>::value, "");
    static_assert(std::is_trivially_copy_constructible<Ptr>::value, "");
    static_assert(std::is_trivially_move_constructible<Ptr>::value, "");

    explicit constexpr PunnedPointer(intptr_t i = 0) { *this = i; }

    constexpr intptr_t asInt() const
    {
        intptr_t R = 0;
        std::memcpy(&R, Data, sizeof(R));
        return R;
    }

    constexpr operator intptr_t() const { return asInt(); }

    constexpr PunnedPointer& operator=(intptr_t const V)
    {
        std::memcpy(Data, &V, sizeof(Data));
        return *this;
    }

    Ptr* getPointerAddress() { return reinterpret_cast<Ptr*>(Data); }
    Ptr const* getPointerAddress() const { return reinterpret_cast<Ptr const*>(Data); }

private:
    alignas(Ptr) unsigned char Data[sizeof(Ptr)];
};

template<typename PointerT, unsigned IntBits, typename PtrTraits>
struct PointerIntPairInfo;

/// PointerIntPair - This class implements a pair of a pointer and small
/// integer.  It is designed to represent this in the space required by one
/// pointer by bitmangling the integer into the low part of the pointer.  This
/// can only be done for small integers: typically up to 3 bits, but it depends
/// on the number of bits available according to PointerLikeTypeTraits for the
/// type.
///
/// Note that PointerIntPair always puts the IntVal part in the highest bits
/// possible.  For example, PointerIntPair<void*, 1, bool> will put the bit for
/// the bool into bit #2, not bit #0, which allows the low two bits to be used
/// for something else.  For example, this allows:
///   PointerIntPair<PointerIntPair<void*, 1, bool>, 1, bool>
/// ... and the two bools will land in different bits.
template<typename PointerTy, unsigned IntBits, typename IntType = unsigned,
    typename PtrTraits = PointerLikeTypeTraits<PointerTy>,
    typename Info = PointerIntPairInfo<PointerTy, IntBits, PtrTraits>>
class PointerIntPair {
    // Used by MSVC visualizer and generally helpful for debugging/visualizing.
    using InfoTy = Info;
    PunnedPointer<PointerTy> Value;

public:
    constexpr PointerIntPair() = default;

    PointerIntPair(PointerTy PtrVal, IntType IntVal)
    {
        setPointerAndInt(PtrVal, IntVal);
    }

    explicit PointerIntPair(PointerTy PtrVal) { initWithPointer(PtrVal); }

    PointerTy getPointer() const { return Info::getPointer(Value); }

    IntType getInt() const { return static_cast<IntType>(Info::getInt(Value)); }

    void setPointer(PointerTy PtrVal) &
    {
        Value = Info::updatePointer(Value, PtrVal);
    }

    void setInt(IntType IntVal) &
    {
        Value = Info::updateInt(Value, static_cast<intptr_t>(IntVal));
    }

    void initWithPointer(PointerTy PtrVal) &
    {
        Value = Info::updatePointer(0, PtrVal);
    }

    void setPointerAndInt(PointerTy PtrVal, IntType IntVal) &
    {
        Value = Info::updateInt(Info::updatePointer(0, PtrVal),
            static_cast<intptr_t>(IntVal));
    }

    PointerTy const* getAddrOfPointer() const
    {
        return const_cast<PointerIntPair*>(this)->getAddrOfPointer();
    }

    PointerTy* getAddrOfPointer()
    {
        assert(Value == reinterpret_cast<intptr_t>(getPointer()) && "Can only return the address if IntBits is cleared and "
                                                                    "PtrTraits doesn't change the pointer");
        return Value.getPointerAddress();
    }

    void* getOpaqueValue() const
    {
        return reinterpret_cast<void*>(Value.asInt());
    }

    void setFromOpaqueValue(void* Val) &
    {
        Value = reinterpret_cast<intptr_t>(Val);
    }

    static PointerIntPair getFromOpaqueValue(void* V)
    {
        PointerIntPair P;
        P.setFromOpaqueValue(V);
        return P;
    }

    // Allow PointerIntPairs to be created from const void * if and only if the
    // pointer type could be created from a const void *.
    static PointerIntPair getFromOpaqueValue(void const* V)
    {
        (void)PtrTraits::getFromVoidPointer(V);
        return getFromOpaqueValue(const_cast<void*>(V));
    }

    bool operator==(PointerIntPair const& RHS) const
    {
        return Value == RHS.Value;
    }

    bool operator!=(PointerIntPair const& RHS) const
    {
        return Value != RHS.Value;
    }

    bool operator<(PointerIntPair const& RHS) const { return Value < RHS.Value; }
    bool operator>(PointerIntPair const& RHS) const { return Value > RHS.Value; }

    bool operator<=(PointerIntPair const& RHS) const
    {
        return Value <= RHS.Value;
    }

    bool operator>=(PointerIntPair const& RHS) const
    {
        return Value >= RHS.Value;
    }
};

template<typename PointerT, unsigned IntBits, typename PtrTraits>
struct PointerIntPairInfo {
    static_assert(PtrTraits::NumLowBitsAvailable < std::numeric_limits<uintptr_t>::digits,
        "cannot use a pointer type that has all bits free");
    static_assert(IntBits <= PtrTraits::NumLowBitsAvailable,
        "PointerIntPair with integer size too large for pointer");
    enum MaskAndShiftConstants : uintptr_t {
        /// PointerBitMask - The bits that come from the pointer.
        PointerBitMask = ~static_cast<uintptr_t>((static_cast<intptr_t>(1) << PtrTraits::NumLowBitsAvailable) - 1),

        /// IntShift - The number of low bits that we reserve for other uses, and
        /// keep zero.
        IntShift = static_cast<uintptr_t>(PtrTraits::NumLowBitsAvailable) - IntBits,

        /// IntMask - This is the unshifted mask for valid bits of the int type.
        IntMask = static_cast<uintptr_t>((static_cast<intptr_t>(1) << IntBits) - 1),

        // ShiftedIntMask - This is the bits for the integer shifted in place.
        ShiftedIntMask = static_cast<uintptr_t>(IntMask << IntShift)
    };

    static PointerT getPointer(intptr_t Value)
    {
        return PtrTraits::getFromVoidPointer(
            reinterpret_cast<void*>(Value & PointerBitMask));
    }

    static intptr_t getInt(intptr_t Value)
    {
        return Value >> IntShift & IntMask;
    }

    static intptr_t updatePointer(intptr_t OrigValue, PointerT Ptr)
    {
        intptr_t PtrWord = reinterpret_cast<intptr_t>(PtrTraits::getAsVoidPointer(Ptr));
        assert((PtrWord & ~PointerBitMask) == 0 && "Pointer is not sufficiently aligned");
        // Preserve all low bits, just update the pointer.
        return PtrWord | (OrigValue & ~PointerBitMask);
    }

    static intptr_t updateInt(intptr_t OrigValue, intptr_t const Int)
    {
        intptr_t IntWord = static_cast<intptr_t>(Int);
        assert((IntWord & ~IntMask) == 0 && "Integer too large for field");

        // Preserve all bits other than the ones we are updating.
        return (OrigValue & ~ShiftedIntMask) | IntWord << IntShift;
    }
};

template<typename PointerTy, unsigned IntBits, typename IntType,
    typename PtrTraits>
struct PointerLikeTypeTraits<
    PointerIntPair<PointerTy, IntBits, IntType, PtrTraits>> {
    static inline void*
    getAsVoidPointer(PointerIntPair<PointerTy, IntBits, IntType> const& P)
    {
        return P.getOpaqueValue();
    }

    static inline PointerIntPair<PointerTy, IntBits, IntType>
    getFromVoidPointer(void* P)
    {
        return PointerIntPair<PointerTy, IntBits, IntType>::getFromOpaqueValue(P);
    }

    static inline PointerIntPair<PointerTy, IntBits, IntType>
    getFromVoidPointer(void const* P)
    {
        return PointerIntPair<PointerTy, IntBits, IntType>::getFromOpaqueValue(P);
    }

    static constexpr int NumLowBitsAvailable = PtrTraits::NumLowBitsAvailable - IntBits;
};

// Allow structured bindings on PointerIntPair.
template<std::size_t I, typename PointerTy, unsigned IntBits, typename IntType,
    typename PtrTraits, typename Info>
decltype(auto)
get(PointerIntPair<PointerTy, IntBits, IntType, PtrTraits, Info> const& Pair)
{
    static_assert(I < 2);
    if constexpr (I == 0)
        return Pair.getPointer();
    else
        return Pair.getInt();
}

namespace std {
template<typename PointerTy, unsigned IntBits, typename IntType,
    typename PtrTraits, typename Info>
struct tuple_size<
    PointerIntPair<PointerTy, IntBits, IntType, PtrTraits, Info>>
    : std::integral_constant<std::size_t, 2> { };

template<std::size_t I, typename PointerTy, unsigned IntBits, typename IntType,
    typename PtrTraits, typename Info>
struct tuple_element<
    I, PointerIntPair<PointerTy, IntBits, IntType, PtrTraits, Info>>
    : std::conditional<I == 0, PointerTy, IntType> { };
} // namespace std

template<int Size = 64>
class StackString {
public:
    // Default constructor: creates an empty string.
    StackString() noexcept
        : data_ { '\0' }
    {
    }

    StackString(char const* begin, char const* end)
    {
        data_.assign(begin, end);
    }

    StackString(char const* text)
    {
        if (text) {
            data_.assign(text, text + std::strlen(text));
        }
    }

    StackString(char const* text, size_t const size)
    {
        if (text) {
            data_.assign(text, text + size);
        }
    }

    StackString(float const value)
    {
        std::ostringstream oss;
        oss << value;                                      // Convert the float to a string using ostringstream
        std::string str = oss.str();                       // Get the string from the ostringstream
        data_.insert(data_.end(), str.begin(), str.end()); // Insert characters into the vector
    }

    StackString(juce::String const& juceStr)
    {
        // Convert the juce::String to a UTF-8 encoded const char* and insert into vector
        char const* utf8String = juceStr.toRawUTF8();                                // Get the UTF-8 representation
        data_.insert(data_.end(), utf8String, utf8String + std::strlen(utf8String)); // Insert characters into vector
    }

    // Copy constructor.
    StackString(StackString const& other) noexcept
        : data_(other.data_)
    {
    }

    // Move constructor.
    StackString(StackString&& other) noexcept
        : data_(std::move(other.data_))
    {
    }

    // Copy assignment operator.
    StackString& operator=(StackString const& other) noexcept
    {
        if (this != &other) {
            data_ = other.data_;
        }
        return *this;
    }

    // Move assignment operator.
    StackString& operator=(StackString&& other) noexcept
    {
        if (this != &other) {
            data_ = std::move(other.data_);
        }
        return *this;
    }

    // Appends a string to this one.
    StackString& operator+=(StackString const& other)
    {
        data_.insert(data_.end(), other.data_.begin(), other.data_.end());
        return *this;
    }

    // Appends a C-string to this one.
    StackString& operator+=(char const* text)
    {
        if (text) {
            data_.insert(data_.end(), text, text + std::strlen(text));
        }
        return *this;
    }

    bool operator==(StackString const& other) const
    {
        if(data_.size() != other.data_.size())
            return false;
        
        return !std::strncmp(data_.data(), other.data_.data(), data_.size());
    }

    // Returns the number of characters in the string.
    size_t length() const noexcept
    {
        if (data_.empty())
            return 0;
        return data_.back() == '\0' ? data_.size() - 1 : data_.size();
    }

    // Checks if the string is empty.
    bool isEmpty() const noexcept
    {
        return data_.empty();
    }

    // Returns the character at the given index.
    char operator[](size_t index) const noexcept
    {
        return data_[index];
    }

    // Returns a substring from the given start index.
    StackString substring(size_t startIndex) const
    {
        if (startIndex >= data_.size())
            return StackString();
        return StackString(data_.data() + startIndex, data_.size() - startIndex);
    }

    // Returns a substring from the given start index to the end index.
    StackString substring(size_t startIndex, size_t const endIndex) const
    {
        if (startIndex >= data_.size() || endIndex <= startIndex)
            return StackString();
        return StackString(data_.data() + startIndex, endIndex - startIndex);
    }

    // Converts the string to upper case.
    StackString toUpperCase() const
    {
        StackString upper = *this;
        std::transform(upper.data_.begin(), upper.data_.end(), upper.data_.begin(), ::toupper);
        return upper;
    }

    // Converts the string to lower case.
    StackString toLowerCase() const
    {
        StackString lower = *this;
        std::transform(lower.data_.begin(), lower.data_.end(), lower.data_.begin(), ::tolower);
        return lower;
    }

    // Clears the string.
    void clear() noexcept
    {
        data_.clear();
    }

    // Returns true if the string contains a specific character.
    bool containsChar(char ch) const noexcept
    {
        return std::find(data_.begin(), data_.end(), ch) != data_.end();
    }

    // Compare the string with another string.
    int compare(StackString const& other) const noexcept
    {
        return std::strcmp(data_.data(), other.data_.data());
    }

    bool startsWith(StackString const& other) const
    {
        if (other.length() > length())
            return false;

        return strncmp(data_.data(), other.data_.data(), other.length()) == 0;
    }

    // Prints the string.
    void print() const
    {
        std::cout.write(data_.data(), data_.size());
        std::cout << std::endl;
    }

    StackString replace(char const* toReplace, char const* replaceWith) const
    {
        StackString result = *this;
        size_t toReplaceLength = std::strlen(toReplace);
        size_t const replaceWithLength = std::strlen(replaceWith);

        if (toReplaceLength == 0)
            return result; // Don't do anything if the substring to replace is empty

        // Iterate through the data to find all occurrences of toReplace and replace them
        for (size_t i = 0; i <= data_.size() - toReplaceLength;) {
            // Compare substring starting at current position
            if (std::equal(toReplace, toReplace + toReplaceLength, data_.begin() + i)) {
                // Replace the substring
                result.data_.erase(result.data_.begin() + i, result.data_.begin() + i + toReplaceLength);    // Remove the old substring
                result.data_.insert(result.data_.begin() + i, replaceWith, replaceWith + replaceWithLength); // Insert the new substring
                i += replaceWithLength;                                                                      // Move past the inserted replacement
            } else {
                ++i; // Move to the next character
            }
        }

        return result;
    }

    juce::String toString() const
    {
        return juce::String::fromUTF8(data_.data(), data_.size());
    }

    void ensureNullTerminated()
    {
        if (data_.empty() || data_.back() != '\0') {
            data_.add('\0');
        }
    }

    char const* data() const
    {
        // If we return a c-string, make sure it's null-terminated
        const_cast<StackString*>(this)->ensureNullTerminated();

        return data_.data();
    }

    SmallArray<char, Size>& getArray()
    {
        return data_;
    }

    StackString<Size> operator+(StackString<Size> const& rhs)
    {
        auto result = *this; // Copy current object
        result.getArray().insert(result.getArray().end(), rhs.data_.begin(), rhs.data_.end());
        return result; // Return the modified copy
    }

private:
    SmallArray<char, Size> data_;
};

template<int Size>
StackString<Size> operator+(StackString<Size> const& lhs, char const rhs[])
{
    auto result = lhs; // Copy current object
    if (rhs) {
        result.getArray().insert(result.getArray().end(), rhs, rhs + std::strlen(rhs)); // Append characters to the new object
    }
    return result; // Return the modified copy
}

template<int Size>
StackString<Size> operator+(char const lhs[], StackString<Size> const& rhs)
{
    auto result = rhs; // Copy current object
    if (lhs) {
        result.getArray().insert(result.getArray().begin(), lhs, lhs + std::strlen(lhs)); // Append characters to the new object
    }
    return result; // Return the modified copy
}

using SmallString = StackString<128>;
