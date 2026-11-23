cmake_minimum_required (VERSION 3.15)

if (NOT DEFINED CMAKE_BUILD_TYPE)
  set (CMAKE_BUILD_TYPE Release CACHE STRING "Build type")
endif ()

project (fftw)

if (POLICY CMP0042)
  cmake_policy (SET CMP0042 NEW)
endif ()

option (BUILD_SHARED_LIBS "Build shared libraries" OFF)
option (BUILD_TESTS "Build tests" ON)

option (ENABLE_OPENMP "Use OpenMP for multithreading" OFF)
option (ENABLE_THREADS "Use pthread for multithreading" OFF)
option (WITH_COMBINED_THREADS "Merge thread library" OFF)

option (ENABLE_FLOAT "single-precision" ON)
option (ENABLE_LONG_DOUBLE "long-double precision" OFF)
option (ENABLE_QUAD_PRECISION "quadruple-precision" OFF)

option (ENABLE_SIMD "Compile SSE2/AVX/AVX2 (x86_64) and NEON (arm64) codelets" ON)

option (DISABLE_FORTRAN "Disable Fortran wrapper routines" OFF)

include(GNUInstallDirs)


include (CheckIncludeFile)
check_include_file (alloca.h         HAVE_ALLOCA_H)
check_include_file (altivec.h        HAVE_ALTIVEC_H)
check_include_file (c_asm.h          HAVE_C_ASM_H)
check_include_file (dlfcn.h          HAVE_DLFCN_H)
check_include_file (intrinsics.h     HAVE_INTRINSICS_H)
check_include_file (inttypes.h       HAVE_INTTYPES_H)
check_include_file (libintl.h        HAVE_LIBINTL_H)
check_include_file (limits.h         HAVE_LIMITS_H)
check_include_file (mach/mach_time.h HAVE_MACH_MACH_TIME_H)
check_include_file (malloc.h         HAVE_MALLOC_H)
check_include_file (memory.h         HAVE_MEMORY_H)
check_include_file (stddef.h         HAVE_STDDEF_H)
check_include_file (stdint.h         HAVE_STDINT_H)
check_include_file (stdlib.h         HAVE_STDLIB_H)
check_include_file (string.h         HAVE_STRING_H)
check_include_file (strings.h        HAVE_STRINGS_H)
check_include_file (sys/types.h      HAVE_SYS_TYPES_H)
check_include_file (sys/time.h       HAVE_SYS_TIME_H)
check_include_file (sys/stat.h       HAVE_SYS_STAT_H)
check_include_file (sys/sysctl.h     HAVE_SYS_SYSCTL_H)
check_include_file (time.h           HAVE_TIME_H)
check_include_file (uintptr.h        HAVE_UINTPTR_H)
check_include_file (unistd.h         HAVE_UNISTD_H)
if (HAVE_TIME_H AND HAVE_SYS_TIME_H)
  set (TIME_WITH_SYS_TIME TRUE)
endif ()

include (CheckPrototypeDefinition)
check_prototype_definition (drand48 "double drand48 (void)" "0" stdlib.h HAVE_DECL_DRAND48)
check_prototype_definition (srand48 "void srand48(long int seedval)" "0" stdlib.h HAVE_DECL_SRAND48)
check_prototype_definition (cosl "long double cosl( long double arg )" "0" math.h HAVE_DECL_COSL)
check_prototype_definition (sinl "long double sinl( long double arg )" "0" math.h HAVE_DECL_SINL)
check_prototype_definition (memalign "void *memalign(size_t alignment, size_t size)" "0" malloc.h HAVE_DECL_MEMALIGN)
check_prototype_definition (posix_memalign "int posix_memalign(void **memptr, size_t alignment, size_t size)" "0" stdlib.h HAVE_DECL_POSIX_MEMALIGN)

include (CheckSymbolExists)
check_symbol_exists (clock_gettime time.h HAVE_CLOCK_GETTIME)
check_symbol_exists (gettimeofday sys/time.h HAVE_GETTIMEOFDAY)
check_symbol_exists (getpagesize unistd.h HAVE_GETPAGESIZE)
check_symbol_exists (drand48 stdlib.h HAVE_DRAND48)
check_symbol_exists (srand48 stdlib.h HAVE_SRAND48)
check_symbol_exists (memalign malloc.h HAVE_MEMALIGN)
check_symbol_exists (posix_memalign stdlib.h HAVE_POSIX_MEMALIGN)
check_symbol_exists (mach_absolute_time mach/mach_time.h HAVE_MACH_ABSOLUTE_TIME)

# plugdata: FFTW_MEASURE can only benchmark plans when kernel/cycle.h finds a
# tick counter.  x86 uses rdtsc (selected by compiler macros), macOS uses
# mach_absolute_time() when both HAVE_MACH_ABSOLUTE_TIME and
# HAVE_MACH_MACH_TIME_H are defined, and AArch64 uses CNTVCT_EL0 when
# HAVE_ARMV8_CNTVCT_EL0 is defined.
#
# The CMake port was missing the #cmakedefine for HAVE_MACH_MACH_TIME_H even
# though it checks for the header, and it never enabled the AArch64 counter
# (only autoconf's --enable-armv8-cntvct-el0 does), so FFTW_MEASURE silently
# measured nothing on arm64.  Without a timer the planner falls back to a
# different, for SIMD codelets worse, heuristic than FFTW_ESTIMATE.
#
# CNTVCT_EL0 is readable from user space on Linux unless the kernel disables
# EL0 access, so probe it at configure time; skip the probe when
# cross-compiling because the result could not be verified.
include (CheckCSourceRuns)
string (TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" _fftw_system_processor)
if (_fftw_system_processor MATCHES "^(aarch64|arm64)$"
    AND NOT APPLE AND NOT EMSCRIPTEN AND NOT CMAKE_CROSSCOMPILING
    AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  check_c_source_runs ("
    #include <stdint.h>
    int main(void)
    {
      uint64_t t;
      __asm__ __volatile__(\"mrs %0, CNTVCT_EL0\" : \"=r\"(t));
      return t == 0;
    }" HAVE_ARMV8_CNTVCT_EL0)
endif ()

check_symbol_exists (alloca alloca.h HAVE_ALLOCA)
if (NOT HAVE_ALLOCA)
  unset (HAVE_ALLOCA CACHE)
  check_symbol_exists (alloca malloc.h HAVE_ALLOCA)
endif ()
check_symbol_exists (isnan math.h HAVE_ISNAN)
check_symbol_exists (snprintf stdio.h HAVE_SNPRINTF)
check_symbol_exists (strchr string.h HAVE_STRCHR)
check_symbol_exists (sysctl unistd.h HAVE_SYSCTL)

if (UNIX)
  set (CMAKE_REQUIRED_LIBRARIES m)
endif ()
check_symbol_exists (cosl math.h HAVE_COSL)
check_symbol_exists (sinl math.h HAVE_SINL)

include (CheckTypeSize)
check_type_size ("float" SIZEOF_FLOAT)
check_type_size ("double" SIZEOF_DOUBLE)
check_type_size ("int" SIZEOF_INT)
check_type_size ("long" SIZEOF_LONG)
check_type_size ("long long" SIZEOF_LONG_LONG)
check_type_size ("unsigned int" SIZEOF_UNSIGNED_INT)
check_type_size ("unsigned long" SIZEOF_UNSIGNED_LONG)
check_type_size ("unsigned long long" SIZEOF_UNSIGNED_LONG_LONG)
check_type_size ("size_t" SIZEOF_SIZE_T)
check_type_size ("ptrdiff_t" SIZEOF_PTRDIFF_T)
math (EXPR SIZEOF_INT_BITS "8 * ${SIZEOF_INT}")
set (C_FFTW_R2R_KIND "C_INT${SIZEOF_INT_BITS}_T")

find_library (LIBM_LIBRARY NAMES m)
if (LIBM_LIBRARY)
  set (HAVE_LIBM TRUE)
endif ()


if (ENABLE_THREADS)
  find_package (Threads)
endif ()
if (Threads_FOUND)
  if(CMAKE_USE_PTHREADS_INIT)
    set (USING_POSIX_THREADS 1)
  endif ()
  set (HAVE_THREADS TRUE)
endif ()

if (ENABLE_OPENMP)
  find_package (OpenMP)
endif ()
if (OPENMP_FOUND)
  set (HAVE_OPENMP TRUE)
endif ()

# plugdata: SIMD codelets.
#
# Upstream's CMake adds -mavx/-mavx2 (or /arch:AVX) to the *whole* library, so
# the compiler may emit AVX anywhere and the library crashes on CPUs without it.
# Its "/arch:SSE2" check also fails on MSVC x64, where SSE2 is implicit.
# Instead, only the AVX/AVX2 codelet files get those flags; FFTW checks the CPU
# at runtime (X(have_simd_avx) etc.) before it uses them. Every SIMD source is
# compiled through a small generated wrapper that is guarded by the target
# architecture, and config.h enables each SIMD family per architecture, so a
# multi-arch build (macOS arm64;x86_64) gets the right codelets in each slice.
set (FFTW_SIMD_X86 OFF)
set (FFTW_SIMD_NEON OFF)
if (ENABLE_SIMD AND NOT EMSCRIPTEN)
  if (APPLE AND CMAKE_OSX_ARCHITECTURES)
    set (_fftw_target_archs ${CMAKE_OSX_ARCHITECTURES})
  elseif (MSVC AND CMAKE_C_COMPILER_ARCHITECTURE_ID)
    set (_fftw_target_archs ${CMAKE_C_COMPILER_ARCHITECTURE_ID})
  else ()
    set (_fftw_target_archs ${CMAKE_SYSTEM_PROCESSOR})
  endif ()
  foreach (_fftw_arch IN LISTS _fftw_target_archs)
    string (TOLOWER "${_fftw_arch}" _fftw_arch)
    if (_fftw_arch MATCHES "^(x86_64|amd64|x64)$")
      set (FFTW_SIMD_X86 ON)
    elseif (_fftw_arch MATCHES "^(arm64|aarch64)$" AND NOT MSVC)
      # simd-neon.h initialises vectors with brace lists, which MSVC's
      # arm_neon.h types don't support, so NEON is GCC/Clang only
      set (FFTW_SIMD_NEON ON)
    endif ()
  endforeach ()
endif ()

if (MSVC)
  set (FFTW_AVX_FLAGS /arch:AVX)
  set (FFTW_AVX2_FLAGS /arch:AVX2)
elseif (APPLE)
  # -Xarch_x86_64 keeps these away from the arm64 slice of universal builds
  set (FFTW_AVX_FLAGS -Xarch_x86_64 -mavx)
  set (FFTW_AVX2_FLAGS -Xarch_x86_64 -mavx2 -Xarch_x86_64 -mfma)
else ()
  set (FFTW_AVX_FLAGS -mavx)
  set (FFTW_AVX2_FLAGS -mavx2 -mfma)
endif ()

set (FFTW_X86_GUARD "defined(__x86_64__) || defined(_M_X64) || defined(_M_AMD64)")
set (FFTW_NEON_GUARD "defined(__aarch64__) || defined(_M_ARM64)")

# Appends a guarded wrapper for each .c file in SOURCES to SOURCEFILES
function (fftw_add_simd_sources guard)
  cmake_parse_arguments (ARG "" "" "FLAGS;SOURCES" ${ARGN})
  foreach (src IN LISTS ARG_SOURCES)
    if (NOT src MATCHES "\\.c$")
      continue ()
    endif ()
    file (RELATIVE_PATH rel "${CMAKE_CURRENT_SOURCE_DIR}" "${src}")
    set (wrapper "${CMAKE_CURRENT_BINARY_DIR}/simd-wrappers/${rel}")
    set (FFTW_SIMD_GUARD "${guard}")
    set (FFTW_SIMD_SOURCE "${src}")
    configure_file ("${CMAKE_CURRENT_SOURCE_DIR}/cmake.simd-wrapper.c.in" "${wrapper}" @ONLY)
    if (ARG_FLAGS)
      set_source_files_properties ("${wrapper}" PROPERTIES COMPILE_OPTIONS "${ARG_FLAGS}")
    endif ()
    list (APPEND SOURCEFILES "${wrapper}")
  endforeach ()
  set (SOURCEFILES ${SOURCEFILES} PARENT_SCOPE)
endfunction ()

file(GLOB           fftw_api_SOURCE                 api/*.c             api/*.h)
file(GLOB           fftw_dft_SOURCE                 dft/*.c             dft/*.h)
file(GLOB           fftw_dft_scalar_SOURCE          dft/scalar/*.c      dft/scalar/*.h)
file(GLOB           fftw_dft_scalar_codelets_SOURCE dft/scalar/codelets/*.c     dft/scalar/codelets/*.h)
file(GLOB           fftw_dft_simd_SOURCE            dft/simd/*.c        dft/simd/*.h)

file(GLOB           fftw_dft_simd_sse2_SOURCE       dft/simd/sse2/*.c   dft/simd/sse2/*.h)
file(GLOB           fftw_dft_simd_avx_SOURCE        dft/simd/avx/*.c    dft/simd/avx/*.h)
file(GLOB           fftw_dft_simd_avx2_SOURCE       dft/simd/avx2/*.c   dft/simd/avx2/*.h dft/simd/avx2-128/*.c   dft/simd/avx2-128/*.h)
file(GLOB           fftw_dft_simd_neon_SOURCE       dft/simd/neon/*.c   dft/simd/neon/*.h)
file(GLOB           fftw_kernel_SOURCE              kernel/*.c          kernel/*.h)
file(GLOB           fftw_rdft_SOURCE                rdft/*.c            rdft/*.h)
file(GLOB           fftw_rdft_scalar_SOURCE         rdft/scalar/*.c     rdft/scalar/*.h)

file(GLOB           fftw_rdft_scalar_r2cb_SOURCE    rdft/scalar/r2cb/*.c
                                                    rdft/scalar/r2cb/*.h)
file(GLOB           fftw_rdft_scalar_r2cf_SOURCE    rdft/scalar/r2cf/*.c
                                                    rdft/scalar/r2cf/*.h)
file(GLOB           fftw_rdft_scalar_r2r_SOURCE     rdft/scalar/r2r/*.c
                                                    rdft/scalar/r2r/*.h)

file(GLOB           fftw_rdft_simd_SOURCE           rdft/simd/*.c       rdft/simd/*.h)
file(GLOB           fftw_rdft_simd_sse2_SOURCE      rdft/simd/sse2/*.c  rdft/simd/sse2/*.h)
file(GLOB           fftw_rdft_simd_avx_SOURCE       rdft/simd/avx/*.c   rdft/simd/avx/*.h)
file(GLOB           fftw_rdft_simd_avx2_SOURCE      rdft/simd/avx2/*.c  rdft/simd/avx2/*.h rdft/simd/avx2-128/*.c  rdft/simd/avx2-128/*.h)
file(GLOB           fftw_rdft_simd_neon_SOURCE      rdft/simd/neon/*.c  rdft/simd/neon/*.h)

file(GLOB           fftw_reodft_SOURCE              reodft/*.c          reodft/*.h)
file(GLOB           fftw_simd_support_SOURCE        simd-support/*.c    simd-support/*.h)
file(GLOB           fftw_libbench2_SOURCE           libbench2/*.c       libbench2/*.h)
list (REMOVE_ITEM   fftw_libbench2_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/libbench2/useropt.c)

set(SOURCEFILES
    ${fftw_api_SOURCE}
    ${fftw_dft_SOURCE}
    ${fftw_dft_scalar_SOURCE}
    ${fftw_dft_scalar_codelets_SOURCE}
    ${fftw_dft_simd_SOURCE}
    ${fftw_kernel_SOURCE}
    ${fftw_rdft_SOURCE}
    ${fftw_rdft_scalar_SOURCE}

    ${fftw_rdft_scalar_r2cb_SOURCE}
    ${fftw_rdft_scalar_r2cf_SOURCE}
    ${fftw_rdft_scalar_r2r_SOURCE}

    ${fftw_rdft_simd_SOURCE}
    ${fftw_reodft_SOURCE}
    ${fftw_simd_support_SOURCE}
    ${fftw_threads_SOURCE}
)

set(fftw_par_SOURCE
    threads/api.c
    threads/conf.c
    threads/ct.c
    threads/dft-vrank-geq1.c
    threads/f77api.c
    threads/hc2hc.c
    threads/rdft-vrank-geq1.c
    threads/vrank-geq1-rdft2.c)

set (fftw_threads_SOURCE ${fftw_par_SOURCE} threads/threads.c)
set (fftw_omp_SOURCE ${fftw_par_SOURCE} threads/openmp.c)


include_directories (.)


if (WITH_COMBINED_THREADS)
  list (APPEND SOURCEFILES ${fftw_threads_SOURCE})
endif ()


if (FFTW_SIMD_X86)
  # SSE2 is part of the x86_64 baseline, so those codelets need no flags
  fftw_add_simd_sources ("${FFTW_X86_GUARD}"
    SOURCES ${fftw_dft_simd_sse2_SOURCE} ${fftw_rdft_simd_sse2_SOURCE})
  fftw_add_simd_sources ("${FFTW_X86_GUARD}" FLAGS ${FFTW_AVX_FLAGS}
    SOURCES ${fftw_dft_simd_avx_SOURCE} ${fftw_rdft_simd_avx_SOURCE})
  fftw_add_simd_sources ("${FFTW_X86_GUARD}" FLAGS ${FFTW_AVX2_FLAGS}
    SOURCES ${fftw_dft_simd_avx2_SOURCE} ${fftw_rdft_simd_avx2_SOURCE})
endif ()

if (FFTW_SIMD_NEON)
  # NEON is part of the AArch64 baseline, so no flags are needed
  fftw_add_simd_sources ("${FFTW_NEON_GUARD}"
    SOURCES ${fftw_dft_simd_neon_SOURCE} ${fftw_rdft_simd_neon_SOURCE})
endif ()

set (FFTW_VERSION 3.3.9)

set (PREC_SUFFIX)
if (ENABLE_FLOAT)
  set (FFTW_SINGLE TRUE)
  set (BENCHFFT_SINGLE TRUE)
  set (PREC_SUFFIX f)
endif ()

if (ENABLE_LONG_DOUBLE)
  set (FFTW_LDOUBLE TRUE)
  set (BENCHFFT_LDOUBLE TRUE)
  set (PREC_SUFFIX l)
endif ()

if (ENABLE_QUAD_PRECISION)
  set (FFTW_QUAD TRUE)
  set (BENCHFFT_QUAD TRUE)
  set (PREC_SUFFIX q)
endif ()
set (fftw3_lib fftw3${PREC_SUFFIX})

configure_file (cmake.config.h.in config.h @ONLY)
include_directories (${CMAKE_CURRENT_BINARY_DIR})

if (BUILD_SHARED_LIBS)
  add_definitions (-DFFTW_DLL)
endif ()

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# plugdata: keep LTO static archives intact with CMake < 4.0.
#
# CMake's IPO (LTO) archive rules for GNU and Clang used "ar cr" / "ar r"
# before CMake 4.0, and these *replace* members that already have the same
# name.  FFTW has many sources that share a basename across directories: every
# SIMD flavor has its own codlist.c and genus.c, the codelet files repeat in
# every flavor directory, and dft/ and rdft/ both have conf.c, plan.c, ... .
# For large object lists CMake splits the archive command into a create pass
# and an append pass, so the append pass silently replaces objects from the
# create pass and the resulting libfftw3f.a is missing members (on Linux
# x86-64 this showed up as unresolved fftwf_dft_*_genus_sse2 symbols when
# linking with CMake 3.28 + LTO).
#
# CMake >= 4.0 fixed this by switching the IPO rules to the non-replacing
# "qc" / "q" modes.  Do the same here so older CMake versions (or toolchains
# that still use the old rules) produce a complete archive.
if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  if (APPLE OR ANDROID)
    set (_fftw_arxiv "${CMAKE_AR}")
  else ()
    set (_fftw_arxiv "${CMAKE_C_COMPILER_AR}")
  endif ()
  if (NOT _fftw_arxiv)
    set (_fftw_arxiv "${CMAKE_AR}")
  endif ()
  set (CMAKE_C_ARCHIVE_CREATE_IPO
    "\"${_fftw_arxiv}\" qc <TARGET> <LINK_FLAGS> <OBJECTS>")
  set (CMAKE_C_ARCHIVE_APPEND_IPO
    "\"${_fftw_arxiv}\" q <TARGET> <LINK_FLAGS> <OBJECTS>")
endif ()

add_library (${fftw3_lib} ${SOURCEFILES})
target_include_directories (${fftw3_lib} INTERFACE $<INSTALL_INTERFACE:include>)
if (MSVC AND NOT (CMAKE_C_COMPILER_ID STREQUAL "Intel"))
  target_compile_definitions (${fftw3_lib} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/bigobj> HAVE_UINTPTR_T=1)
endif ()
if (HAVE_LIBM)
  target_link_libraries (${fftw3_lib} m)
endif ()

set (subtargets ${fftw3_lib})

if (Threads_FOUND)
  if (WITH_COMBINED_THREADS)
    target_link_libraries (${fftw3_lib} ${CMAKE_THREAD_LIBS_INIT})
  else ()
    add_library (${fftw3_lib}_threads ${fftw_threads_SOURCE})
    target_include_directories (${fftw3_lib}_threads INTERFACE $<INSTALL_INTERFACE:include>)
    target_link_libraries (${fftw3_lib}_threads ${fftw3_lib})
    target_link_libraries (${fftw3_lib}_threads ${CMAKE_THREAD_LIBS_INIT})
    list (APPEND subtargets ${fftw3_lib}_threads)
  endif ()
endif ()

if (OPENMP_FOUND)
  add_library (${fftw3_lib}_omp ${fftw_omp_SOURCE})
  target_include_directories (${fftw3_lib}_omp INTERFACE $<INSTALL_INTERFACE:include>)
  target_link_libraries (${fftw3_lib}_omp ${fftw3_lib})
  target_link_libraries (${fftw3_lib}_omp ${CMAKE_THREAD_LIBS_INIT})
  list (APPEND subtargets ${fftw3_lib}_omp)
  target_compile_options (${fftw3_lib}_omp PRIVATE ${OpenMP_C_FLAGS})
endif ()

foreach(subtarget ${subtargets})
  set_target_properties (${subtarget} PROPERTIES SOVERSION 3.6.9 VERSION 3)
  install (TARGETS ${subtarget}
	  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
endforeach ()
install(TARGETS ${fftw3_lib}
          EXPORT FFTW3LibraryDepends
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install (FILES api/fftw3.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
if (EXISTS ${CMAKE_SOURCE_DIR}/api/fftw3.f)
  install (FILES api/fftw3.f api/fftw3l.f03 api/fftw3q.f03 DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif ()
if (EXISTS ${CMAKE_SOURCE_DIR}/api/fftw3.f03.in)
  file (READ api/fftw3.f03.in FFTW3_F03_IN OFFSET 42)
  file (WRITE ${CMAKE_CURRENT_BINARY_DIR}/fftw3.f03 "! Generated automatically.  DO NOT EDIT!\n\n")
  file (APPEND ${CMAKE_CURRENT_BINARY_DIR}/fftw3.f03 "  integer, parameter :: C_FFTW_R2R_KIND = ${C_FFTW_R2R_KIND}\n\n")
  file (APPEND ${CMAKE_CURRENT_BINARY_DIR}/fftw3.f03 "${FFTW3_F03_IN}")
  install (FILES ${CMAKE_CURRENT_BINARY_DIR}/fftw3.f03 DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif ()

if (BUILD_TESTS)

  add_executable (bench ${fftw_libbench2_SOURCE} tests/bench.c tests/hook.c tests/fftw-bench.c)

  if (ENABLE_THREADS AND NOT WITH_COMBINED_THREADS)
    target_link_libraries (bench ${fftw3_lib}_threads)
  else ()
    target_link_libraries (bench ${fftw3_lib})
  endif ()


  enable_testing ()

  if (Threads_FOUND)

    macro (fftw_add_test problem)
      add_test (NAME ${problem} COMMAND bench -s ${problem})
    endmacro ()

    fftw_add_test (32x64)
    fftw_add_test (ib256)

  endif ()
endif ()

# pkgconfig file
set (prefix ${CMAKE_INSTALL_PREFIX})
set (exec_prefix ${CMAKE_INSTALL_PREFIX})
set (libdir ${CMAKE_INSTALL_FULL_LIBDIR})
set (includedir ${CMAKE_INSTALL_FULL_INCLUDEDIR})
set (VERSION ${FFTW_VERSION})
configure_file (fftw.pc.in fftw3${PREC_SUFFIX}.pc @ONLY)
install (FILES
          ${CMAKE_CURRENT_BINARY_DIR}/fftw3${PREC_SUFFIX}.pc
         DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
         COMPONENT Development)

# cmake file
set (FFTW3_LIBRARIES "FFTW3::${fftw3_lib}")
configure_file (FFTW3Config.cmake.in FFTW3${PREC_SUFFIX}Config.cmake @ONLY)
configure_file (FFTW3ConfigVersion.cmake.in FFTW3${PREC_SUFFIX}ConfigVersion.cmake @ONLY)
install (FILES
          ${CMAKE_CURRENT_BINARY_DIR}/FFTW3${PREC_SUFFIX}Config.cmake
          ${CMAKE_CURRENT_BINARY_DIR}/FFTW3${PREC_SUFFIX}ConfigVersion.cmake
	  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/fftw3${PREC_SUFFIX}
         COMPONENT Development)

export (TARGETS ${fftw3_lib} NAMESPACE FFTW3:: FILE ${PROJECT_BINARY_DIR}/FFTW3LibraryDepends.cmake)
install(EXPORT FFTW3LibraryDepends
        NAMESPACE FFTW3::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/fftw3${PREC_SUFFIX}
        COMPONENT Development)
