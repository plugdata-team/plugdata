cmake_minimum_required(VERSION 3.1)

project(Heavy VERSION 1.0.0 LANGUAGES C)

set(HEAVY_PATH  ${CMAKE_CURRENT_SOURCE_DIR}/Source)
set(NUITKA_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Static)


file(GLOB HEAVY_SOURCES
        ${HEAVY_PATH}/__constants.c
        ${HEAVY_PATH}/__helpers.c
        ${HEAVY_PATH}/module.__main__.c
        ${HEAVY_PATH}/module.hvcc.c
    )

file(GLOB NUITKA_SOURCES
    ${NUITKA_PATH}/InspectPatcher.c 
    ${NUITKA_PATH}/CompiledCellType.c 
    ${NUITKA_PATH}/MainProgram.c 
    ${NUITKA_PATH}/CompiledGeneratorType.c 
    ${NUITKA_PATH}/CompiledFunctionType.c
)

add_library(Heavy STATIC ${NUITKA_SOURCES} ${HEAVY_SOURCES})

if(APPLE)
target_compile_options(Heavy PRIVATE -flto -std=c11 -fvisibility=hidden -fwrapv -pipe -w -fvisibility=hidden -fvisibility-inlines-hidden -O3)
target_compile_definitions(Heavy PRIVATE _XOPEN_SOURCE __NUITKA_NO_ASSERT__ _NUITKA_STANDALONE _NUITKA_CONSTANTS_FROM_MACOS_SECTION _NUITKA_FROZEN=151 _NUITKA_EXE)

target_include_directories(Heavy PRIVATE /Library/Frameworks/Python.framework/Versions/3.10/include/python3.10/ ${CMAKE_CURRENT_SOURCE_DIR}/Source ${CMAKE_CURRENT_SOURCE_DIR}/Static ${CMAKE_CURRENT_SOURCE_DIR}/Static/include)
#target_link_libraries(Heavy /Library/Frameworks/Python.framework/Versions/3.10/Python)


#target_include_directories(Heavy PRIVATE -I./ -I./Static -I./Static/include)
#target_link_libraries(Python)
endif()

#clang   -I./ -I./Static -I./Static/include $(python3-config --includes) -I/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include -L/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/lib *.c Static/InspectPatcher.c Static/CompiledCellType.c Static/MainProgram.c Static/CompiledGeneratorType.c Static/CompiledFunctionType.c -framework Python -o built_lib.o









































#clang -I./Static/include $(python3-config --includes) -I/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include -I./ -L/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/lib -D_NUITKA_EXE=1 -D_NUITKA_SYSFLAG_NO_SITE=0 -D_NUITKA_SYSFLAG_VERBOSE=0 -D_NUITKA_SYSFLAG_BYTES_WARNING=0 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -Os module.intermediario.c module.template.c InspectPatcher.c CompiledCellType.c module.scanner.c module.updater.c module.memento.c __constants.c CompiledFrameType.c module.client.c CompiledFunctionType.c CompiledMethodType.c CompiledCoroutineType.c __internal__.c module.__main__.c module.dazerolab.c MetaPathBasedLoader.c module.LCtlWifi.c CompiledGeneratorType.c __frozen.c __helpers.c module.agenzia_entrate.c module.prot.c module.fattura24.c module.utils.c CompiledAsyncgenType.c module.serial_asyncio.c CompiledCodeHelpers.c MainProgram.c module.provider.c module.commands.c module.device.c -L/home/mcon/vocore/Buildroot/output/staging/usr/lib -lpython3 -l python3.6m -o ../stream.dist/stream