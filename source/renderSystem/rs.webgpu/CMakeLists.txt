set(TARGET_NAME rs.webgpu)

file(GLOB HEADER_FILES *.h *.hpp)
file(GLOB SOURCE_FILES *.c *.cc *.cpp)

source_group("inc" FILES ${HEADER_FILES})
source_group("src" FILES ${SOURCE_FILES})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../external)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../external/glm)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../external/webgpu/include)

set(WGPU_NATIVE_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../external/webgpu/lib/x64)

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
