set(TARGET_NAME test.js.quickjs)

file(GLOB_RECURSE HEADER_FILES *.h *.hpp)
file(GLOB_RECURSE SOURCE_FILES *.c *.cc *.cpp)

# 自动建立目录树
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

# JS 引擎栈检查上限(1MB)与 MSVC 默认主线程栈(1MB)过于接近,深递归会先崩宿主栈;
# 与 quickjs-ng 官方做法一致,把测试 exe 栈提至 8MB
if(MSVC)
    target_link_options(${TARGET_NAME} PRIVATE /STACK:8388608)
endif()

# core 对 quickjs 为 PRIVATE 链接且不导出其符号,测试直接链接 quickjs 静态库;
# 运行时 core.dll 与 exe 各持一份 quickjs 代码,但仅 core.dll 侧(FEQuickJS)创建
# runtime/context,exe 侧只调用 JS_Eval/JS_ToInt32 等纯数据操作函数,无类注册等
# 全局态操作,两份拷贝一致安全
target_link_libraries(${TARGET_NAME} core quickjs)

# 测试直接用 JS_Eval 断言求值结果,需 quickjs 头文件
target_include_directories(${TARGET_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../external/quickjs")

# 测试脚本拷贝到可执行目录,main.cpp 从 exe 所在目录加载
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/script.js
            $<TARGET_FILE_DIR:${TARGET_NAME}>/script.js
)
