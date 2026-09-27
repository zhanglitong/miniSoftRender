set(TARGET_NAME rs.webgpu)

file(GLOB HEADER_FILES *.h *.hpp)
file(GLOB SOURCE_FILES *.c *.cc *.cpp)

source_group("inc" FILES ${HEADER_FILES})
source_group("src" FILES ${SOURCE_FILES})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../external)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../external/glm)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../external/webgpu/include)

set(WGPU_NATIVE_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../external/webgpu/lib/x64)
set(WGPU_NATIVE_BIN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../external/webgpu/bin/x64)

find_library(
    WGPU_LIBRARY NAMES wgpu_native.lib
    HINTS ${WGPU_NATIVE_LIB_DIR}
    REQUIRED
)
message(STATUS "WebGPU Library: ${WGPU_LIBRARY}")

add_library(${TARGET_NAME} STATIC
			${HEADER_FILES}
			${SOURCE_FILES}
)
target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../inc)
target_link_libraries(${TARGET_NAME} PRIVATE ${WGPU_LIBRARY})
target_link_libraries(${TARGET_NAME} PRIVATE core)

if(UNIX)
    target_compile_definitions(${TARGET_NAME} PUBLIC -D_LINUX_ -D_UNUSE_SIMD)
    target_compile_definitions(${TARGET_NAME} PRIVATE -DPOSIX -DLINUX)
endif()

target_compile_definitions(${TARGET_NAME} PRIVATE -DNOMINMAX -DUNICODE -D_UNICODE -D_CRT_SECURE_NO_WARNINGS)
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "renderSystem")

# 把 wgpu_native.dll 拷贝到可执行文件输出目录（exe 与 rs.webgpu 在同一 CMAKE_RUNTIME_OUTPUT_DIRECTORY）
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${WGPU_NATIVE_BIN_DIR}/wgpu_native.dll
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/wgpu_native.dll
    COMMENT "Copy wgpu_native.dll to runtime output directory"
)
