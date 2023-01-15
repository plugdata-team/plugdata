# The exported targets as aliases, to use FluidLite in another project 
# for instance as a git submodule
if (TARGET fluidlite)
    add_library(fluidlite::fluidlite ALIAS fluidlite)
endif()
if (TARGET fluidlite-static)
    add_library(fluidlite::fluidlite-static ALIAS fluidlite-static)
endif()
