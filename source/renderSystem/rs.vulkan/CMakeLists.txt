set(TARGET_NAME rs.vulkan)

file(GLOB HEADER_FILES *.h *.hpp)
file(GLOB SOURCE_FILES *.c *.cc *.cpp)
file(GLOB SPIRV_SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../inc/graphic/spirv-reflect/*.c *.cc *.cpp)

source_group("inc" FILES ${HEADER_FILES})
source_group("src" FILES ${SOURCE_FILES})
source_group("src/spirv-reflect" FILES ${SPIRV_SOURCE_FILES})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../external)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../external/glm)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../external/webgpu/include)

set(WGPU_NATIVE_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../external/webgpu/lib/x64)

find_library(
    WGPU_LIBRARY NAMES wgpu_native.lib
    HINTS ${WGPU_NATIVE_LIB_DIR}
    REQUIRED
)
message(STATUS ${WGPU_LIBRARY})

message(STATUS "Using module to find Vulkan")

find_package(Vulkan REQUIRED COMPONENTS shaderc_combined)

IF (NOT Vulkan_FOUND)
	message(FATAL_ERROR "Could not find Vulkan library!")
ELSE()
	message(STATUS ${Vulkan_LIBRARY})
ENDIF()

IF(WIN32)
	IF (Vulkan_FOUND)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WIN32_KHR")
	ENDIF()
ENDIF(WIN32)

add_library(${TARGET_NAME} STATIC
			${HEADER_FILES}
			${SOURCE_FILES}
			${SPIRV_SOURCE_FILES}
)
target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../inc)

target_link_libraries(${TARGET_NAME} PRIVATE Vulkan::Headers)
target_link_libraries(${TARGET_NAME} PRIVATE Vulkan::Vulkan)

target_link_libraries(${TARGET_NAME} PRIVATE ${WGPU_LIBRARY})

target_link_libraries(${TARGET_NAME} PRIVATE Vulkan::shaderc_combined)

target_link_libraries(${TARGET_NAME} PRIVATE core)

if(UNIX)
    target_compile_definitions(${TARGET_NAME} PUBLIC -D_LINUX_ -D_UNUSE_SIMD)
    target_compile_definitions(${TARGET_NAME} PRIVATE -DPOSIX -DLINUX)
endif()

target_compile_definitions(${TARGET_NAME} PRIVATE -DNOMINMAX -DUNICODE -D_UNICODE -D_CRT_SECURE_NO_WARNINGS)
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "renderSystem")

set(INPUT_FILE		${CMAKE_CURRENT_SOURCE_DIR}/../../inc/graphic/FEShaderDefine.h)
set(OUTPUT_FILE		${CMAKE_SOURCE_DIR}/bin/data/shader/source/common/FEShaderDefine.h)

add_custom_target(CopyResources ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/bin/data/shader/source/common
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${INPUT_FILE} ${OUTPUT_FILE}
    DEPENDS ${INPUT_FILE}
    COMMENT "Copying ${INPUT_FILE} to ${OUTPUT_FILE} if changed"
)

add_dependencies(${TARGET_NAME} CopyResources)
