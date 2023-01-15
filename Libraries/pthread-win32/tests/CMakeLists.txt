
include(CTest)

macro(add_testcase name srcs lib)
    set(test_name test-${name}-${lib})
    add_executable(${test_name} ${srcs})
    target_link_libraries(${test_name} PUBLIC ${lib} ws2_32)
    target_compile_definitions(${test_name} PUBLIC _CONSOLE _MBCS __PtW32NoCatchWarn)
    add_test(NAME ${test_name} COMMAND ${test_name})
endmacro()

file(GLOB test_srcs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.c)
list(REMOVE_ITEM test_srcs
    "benchlib.c"
    "context2.c"
    "tryentercs2.c"
    "openmp1.c"
    "wrapper4tests_1.c"
)

foreach(test_src ${test_srcs})
    get_filename_component(test_name ${test_src} NAME_WE)

    if(${test_name} MATCHES "benchtest")
        list(APPEND test_src "benchlib.c")
    endif()
    
    add_testcase(${test_name} "${test_src}" pthreadVCE3)
    add_testcase(${test_name} "${test_src}" pthreadVSE3)
    add_testcase(${test_name} "${test_src}" pthreadVC3)
endforeach()
