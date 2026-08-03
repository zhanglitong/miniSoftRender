set(TARGET_NAME vkDemo)

file(GLOB_RECURSE HEADER_FILES *.h *.hpp)
file(GLOB_RECURSE SOURCE_FILES *.c *.cc *.cpp) 

# 2. 自动建立目录树
# ${CMAKE_CURRENT_SOURCE_DIR} 是物理路径的根，生成的 IDE 结构将与之对齐
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${HEADER_FILES})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCE_FILES})


message(STATUS "Using module to find Vulkan")

find_package(Vulkan REQUIRED)


IF (NOT Vulkan_FOUND)
    message(FATAL_ERROR "Could not find Vulkan library!")
ELSE()
    message(STATUS ${Vulkan_LIBRARY})
ENDIF()

# 健壮地获取 glslc 路径
if(Vulkan_GLSLC_EXECUTABLE)
    set(GLSLC ${Vulkan_GLSLC_EXECUTABLE})
elseif(Vulkan_GLSLC)
    set(GLSLC ${Vulkan_GLSLC})
else()
    # 再尝试手动查找（备用）
    find_program(GLSLC glslc PATHS ENV VULKAN_SDK PATH_SUFFIXES bin)
    if(NOT GLSLC)
        message(FATAL_ERROR "glslc not found")
    endif()
endif()

message(STATUS "Using glslc: ${GLSLC}")

IF(WIN32)
    IF (Vulkan_FOUND)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WIN32_KHR")
    ENDIF()
ENDIF(WIN32)

set(SHADER_SRC_DIR      ${CMAKE_SOURCE_DIR}/bin/data/shader/source)
set(SHADER_CACHE_DIR    ${CMAKE_SOURCE_DIR}/bin/data/shader/cache)

set(PIPELINE_DIR        ${CMAKE_SOURCE_DIR}/bin/data/pipeline)

file(GLOB_RECURSE SHADER_SOURCES 
    "${SHADER_SRC_DIR}/*.vert" 
    "${SHADER_SRC_DIR}/*.frag" 
    "${SHADER_SRC_DIR}/*.comp" 
    "${SHADER_SRC_DIR}/*.geom" 
    "${SHADER_SRC_DIR}/*.tesc" 
    "${SHADER_SRC_DIR}/*.tese" 
    "${SHADER_SRC_DIR}/*.mesh" 
    "${SHADER_SRC_DIR}/*.task" 
    "${SHADER_SRC_DIR}/*.rgen" 
    "${SHADER_SRC_DIR}/*.rchit" 
    "${SHADER_SRC_DIR}/*.rmiss" 
    "${SHADER_SRC_DIR}/*.rcall" 
    "${SHADER_SRC_DIR}/*.rahit" 
    "${SHADER_SRC_DIR}/*.rint" 
    "${SHADER_SRC_DIR}/*.glsl"
)

file(GLOB_RECURSE PIPELINE_LIST
    "${PIPELINE_DIR}/*.xml" 
)

source_group(TREE "${SHADER_SRC_DIR}" PREFIX "shader"   FILES ${SHADER_SOURCES})

source_group(TREE "${PIPELINE_DIR}"   PREFIX "pipeline" FILES ${PIPELINE_LIST})


message(STATUS "Shader sources: ${SHADER_SOURCES}")

add_executable(${TARGET_NAME}
        ${HEADER_FILES}
        ${SOURCE_FILES}
        ${SHADER_SOURCES}
        ${PIPELINE_LIST}
)

set(SPV_OUTPUTS "")
foreach(SHADER ${SHADER_SOURCES})
    file(RELATIVE_PATH RELATIVE_PATH ${SHADER_SRC_DIR} ${SHADER})
    
    set(OUTPUT_SPV "${SHADER_CACHE_DIR}/${RELATIVE_PATH}.spv")
    
    get_filename_component(OUTPUT_DIR ${OUTPUT_SPV} DIRECTORY)
    file(MAKE_DIRECTORY ${OUTPUT_DIR})
    
    add_custom_command(
        OUTPUT ${OUTPUT_SPV}
        COMMAND ${GLSLC} -I ${SHADER_SRC_DIR}/common ${SHADER} -o ${OUTPUT_SPV}
        DEPENDS ${SHADER}
        COMMENT "编译着色器 ${RELATIVE_PATH}"
        VERBATIM
    )
    list(APPEND SPV_OUTPUTS ${OUTPUT_SPV})
endforeach()

add_custom_target(CompileShaders ALL DEPENDS ${SPV_OUTPUTS})

add_dependencies(${TARGET_NAME} CompileShaders)

message(STATUS "[Shader Compile] Total shaders to compile: ${SHADER_SOURCES}")


target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/../../../inc)

target_link_libraries(${TARGET_NAME} PUBLIC Vulkan::Headers)
target_link_libraries(${TARGET_NAME} PRIVATE core)
target_link_libraries(${TARGET_NAME} PRIVATE rs.vulkan)
target_link_libraries(${TARGET_NAME} PRIVATE rs.webgpu)

if(UNIX)
    target_compile_definitions(${TARGET_NAME} PUBLIC -D_LINUX_ -D_UNUSE_SIMD)
    target_compile_definitions(${TARGET_NAME} PRIVATE -DPOSIX -DLINUX)
endif()



target_compile_definitions(${TARGET_NAME} PRIVATE -DNOMINMAX -DUNICODE -D_UNICODE -D_CRT_SECURE_NO_WARNINGS)
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "sample/basic/window")

