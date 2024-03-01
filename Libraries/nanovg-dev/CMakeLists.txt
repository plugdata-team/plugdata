project(nanovg VERSION 0.0.0 LANGUAGES C CXX)

#add_library(${PROJECT_NAME}
#        src/nanovg.c
#        )

include_directories("src")

file(GLOB SOURCES "src/*.c")

#target_include_directories(${PROJECT_NAME} PUBLIC .)
#target_include_directories(${PROJECT_NAME} PRIVATE ./src)

add_library(${PROJECT_NAME} STATIC ${SOURCES})
target_compile_options(${PROJECT_NAME} PRIVATE -O3)
