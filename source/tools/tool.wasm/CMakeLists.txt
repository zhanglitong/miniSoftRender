
set(TARGET_NAME wasm.api)
project(${TARGET_NAME})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)



###if(DEFINED ENV{EMSDK})
###    set(CMAKE_TOOLCHAIN_FILE $ENV{EMSDK}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake)
###else()
###    message(FATAL_ERROR "EMSDK environment variable not found. Please install Emscripten and set the EMSDK environment variable.")
###endif()

message($ENV{EMSDK})

include_directories(../../external)
include_directories(../../external/glm)
include_directories(../../external/webgpu/include)
include_directories(../../inc)

if(NOT EMSCRIPTEN)
    include_directories($ENV{EMSDK}/upstream/emscripten/system/include)
endif()
set(HEADER_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.h
)

set(SOURCE_FILES
     ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)


add_executable(${TARGET_NAME}
    ${HEADER_FILES}
    ${SOURCE_FILES}
)

if(EMSCRIPTEN)
    target_link_libraries(${TARGET_NAME} embind)
    # 指定输出为 .html 文件
    set_target_properties(${TARGET_NAME} PROPERTIES
        SUFFIX ".html"
    )

    # 编译选项：添加 MEMORY64=1
    # 苹果手机上 用 MEMORY64=1 不行
    target_compile_options(${TARGET_NAME} PRIVATE
        "SHELL:-s MEMORY64=2"
    )

    #  "SHELL:-s INITIAL_MEMORY=16777216"      # 初始 16MB
    #  "SHELL:-s ALLOW_MEMORY_GROWTH=1"        # 允许增长
    #  "SHELL:-s MAXIMUM_MEMORY=2147483648"    # 可选：限制最大 2GB

    target_link_options(${TARGET_NAME} PRIVATE
        "SHELL:-s WASM=1"
        "SHELL:-s MODULARIZE=1"
        "SHELL:-s EXPORT_NAME='MyModule'"
        "SHELL:-s SINGLE_FILE=1"
        "SHELL:--bind"
        "SHELL:-s MEMORY64=2"
   
    )

    # 指定自定义 HTML 模板（注意：必须包含 --shell-file）
    target_link_options(${TARGET_NAME} PRIVATE
        "SHELL:--shell-file ${CMAKE_CURRENT_SOURCE_DIR}/test_template.html"
    )

    target_compile_definitions(${TARGET_NAME} PRIVATE -DNOMINMAX -DIS_EMSCRIPTEN)
else()
    target_compile_definitions(${TARGET_NAME} PRIVATE -DNOMINMAX -DUNICODE -D_UNICODE -D_CRT_SECURE_NO_WARNINGS)
    set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "tool")
endif()

