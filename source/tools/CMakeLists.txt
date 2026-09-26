add_subdirectory ("tool.resource.cc")
if(DEFINED ENV{EMSDK})
    add_subdirectory ("tool.wasm")
else()
    message(STATUS "EMSDK environment variable not set, skipping tool.wasm")
endif()
add_subdirectory ("tool.animation.editor")

