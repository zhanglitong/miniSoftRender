
add_subdirectory ("tools")
add_subdirectory ("core")
add_subdirectory ("sample")
add_subdirectory ("test")
add_subdirectory ("renderSystem")

# ===== Shader 编译自动化（公共位置）=====
# 所有可执行程序经 core 传递依赖 CompileShaders，构建前自动把 
# bin/data/shader/source 下的着色器编译为 cache 中的 SPIR-V，
# 避免 cache 被清后运行时缺 shader 崩溃（VKCPipeline 空指针）。
set(SHADER_SRC_DIR      ${CMAKE_SOURCE_DIR}/bin/data/shader/source)
set(SHADER_CACHE_DIR    ${CMAKE_SOURCE_DIR}/bin/data/shader/cache)

# 健壮地获取 glslc 路径
if(Vulkan_GLSLC_EXECUTABLE)
    set(GLSLC ${Vulkan_GLSLC_EXECUTABLE})
elseif(Vulkan_GLSLC)
    set(GLSLC ${Vulkan_GLSLC})
else()
    find_program(GLSLC glslc PATHS ENV VULKAN_SDK PATH_SUFFIXES bin)
endif()

file(GLOB_RECURSE SHADER_SOURCES CONFIGURE_DEPENDS
    "${SHADER_SRC_DIR}/*.vert"  "${SHADER_SRC_DIR}/*.frag"
    "${SHADER_SRC_DIR}/*.comp"  "${SHADER_SRC_DIR}/*.geom"
    "${SHADER_SRC_DIR}/*.tesc"  "${SHADER_SRC_DIR}/*.tese"
    "${SHADER_SRC_DIR}/*.mesh"  "${SHADER_SRC_DIR}/*.task"
    "${SHADER_SRC_DIR}/*.rgen"  "${SHADER_SRC_DIR}/*.rchit"
    "${SHADER_SRC_DIR}/*.rmiss" "${SHADER_SRC_DIR}/*.rcall"
    "${SHADER_SRC_DIR}/*.rahit" "${SHADER_SRC_DIR}/*.rint")

if(GLSLC AND SHADER_SOURCES)
    set(SPV_OUTPUTS "")
    foreach(SHADER ${SHADER_SOURCES})
        file(RELATIVE_PATH RELATIVE_PATH ${SHADER_SRC_DIR} ${SHADER})
        set(OUTPUT_SPV "${SHADER_CACHE_DIR}/${RELATIVE_PATH}.spv")
        get_filename_component(OUTPUT_DIR ${OUTPUT_SPV} DIRECTORY)
        file(MAKE_DIRECTORY ${OUTPUT_DIR})

        # 按子目录附加编译定义:oit 几何着色器需要 OIT_MODE 宏（写入 PPLL 链表）
        set(SHADER_DEFS "")
        if(RELATIVE_PATH MATCHES "^oit/")
            list(APPEND SHADER_DEFS -DOIT_MODE)
        endif()

        add_custom_command(
            OUTPUT ${OUTPUT_SPV}
            COMMAND ${GLSLC} -I ${SHADER_SRC_DIR}/common ${SHADER_DEFS} ${SHADER} -o ${OUTPUT_SPV}
            DEPENDS ${SHADER}
            COMMENT "编译着色器 ${RELATIVE_PATH}"
            VERBATIM
        )
        list(APPEND SPV_OUTPUTS ${OUTPUT_SPV})
    endforeach()

    add_custom_target(CompileShaders ALL DEPENDS ${SPV_OUTPUTS})
    # FEShaderDefine.h 副本必须在编译 shader 前更新（目标定义在 rs.vulkan）
    add_dependencies(CompileShaders CopyResources)
    # 所有链接 core 的 sample/test/tool 构建前先编译 shader
    add_dependencies(core CompileShaders)

    list(LENGTH SHADER_SOURCES N_SHADERS)
    message(STATUS "[Shader Compile] glslc: ${GLSLC}, shaders: ${N_SHADERS}, cache: ${SHADER_CACHE_DIR}")
else()
    message(WARNING "[Shader Compile] glslc not found or no shader sources — skip (use existing cache)")
endif()