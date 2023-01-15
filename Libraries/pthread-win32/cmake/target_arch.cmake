
set(TARGET_ARCH_DETECT_CODE "
    int main() {
        #if defined(_M_ARM)
            return 2;
        #elif defined(_M_ARM64)
            return 3;
        #elif defined(_M_AMD64)
            return 4;
        #elif defined(_M_X64)
            return 5;
        #elif defined(_M_IX86)
            return 6;
        #else
            return 0;
    #endif
}
")

function(get_target_arch out)

    file(WRITE 
        "${CMAKE_BINARY_DIR}/target_arch_detect.c"
        "${TARGET_ARCH_DETECT_CODE}")

    try_run(
        run_result compile_result_unused
        "${CMAKE_BINARY_DIR}" "${CMAKE_BINARY_DIR}/target_arch_detect.c")

    if (run_result STREQUAL 2)
        set(${out} "ARM" PARENT_SCOPE)
    elseif (run_result STREQUAL 3)
        set(${out} "ARM64" PARENT_SCOPE)
    elseif (run_result STREQUAL 4)
        set(${out} "x86_64" PARENT_SCOPE)
    elseif (run_result STREQUAL 5)
        set(${out} "x64" PARENT_SCOPE)
    elseif (run_result STREQUAL 6)
        set(${out} "x86" PARENT_SCOPE)
    else()
        set(${out} "unknown" PARENT_SCOPE)
    endif()
endfunction()
