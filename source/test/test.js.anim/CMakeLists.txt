set(TARGET_NAME test.js.anim)

file(GLOB_RECURSE HEADER_FILES *.h *.hpp)
file(GLOB_RECURSE SOURCE_FILES *.c *.cc *.cpp)

# 自动建立 IDE 目录树
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${HEADER_FILES})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCE_FILES})

add_executable(${TARGET_NAME}
        ${HEADER_FILES}
        ${SOURCE_FILES}
)

if(UNIX)
    target_compile_definitions(${TARGET_NAME} PUBLIC -D_PR_LINUX_ -DWIZ_UNUSE_SIMD)
    target_compile_definitions(${TARGET_NAME} PRIVATE -DPOSIX -DLINUX)
endif()

target_compile_definitions(${TARGET_NAME} PRIVATE -DNOMINMAX -DUNICODE -D_UNICODE -D_CRT_SECURE_NO_WARNINGS)

set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "test")

target_link_libraries(${TARGET_NAME} core rs.vulkan rs.webgpu)

# 动画脚本拷贝到可执行目录,main.cpp 从 exe 所在目录加载
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/anim.js
            $<TARGET_FILE_DIR:${TARGET_NAME}>/anim.js
)
