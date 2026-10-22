# ===== QuickJS-NG:源码内置(嵌入脚本,最小拷贝)=====
# 仅拷贝解释器库源码(dtoa/libregexp/libunicode/quickjs),
# 排除独立可执行入口 qjs.c/qjsc.c(解释器/字节码编译器 CLI)、quickjs-libc.c
# (std/os 模块,宿主自带 fe.log)及 tests/docs/gen。
# 编译器适配项与官方 CMakeLists 保持一致:
#   - MSVC 的 C11 <stdatomic.h> 需要 /experimental:c11atomics
#   - Windows 定义 WIN32_LEAN_AND_MEAN/_WIN32_WINNT=0x0601
file(GLOB HEADER_FILES *.h)
file(GLOB SOURCE_FILES *.c)

add_library(quickjs STATIC
            ${HEADER_FILES}
            ${SOURCE_FILES}
)

target_include_directories(quickjs PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_compile_definitions(quickjs PRIVATE _GNU_SOURCE _CRT_SECURE_NO_WARNINGS)

if(MSVC)
    # C11 atomics(quickjs-c-atomics.h 在非 GCC 下转 <stdatomic.h>)
    # <stdatomic.h> 要求 C11 语言模式(MSVC .c 默认为旧版 C 模式,须显式指定)
    target_compile_options(quickjs PRIVATE /std:c11)
    # 旧版 MSVC 不识别该选项,先探测再加;不可用时 quickjs.c 自行跳过 CONFIG_ATOMICS
    include(CheckCCompilerFlag)
    check_c_compiler_flag("/experimental:c11atomics" FE_QJS_HAS_C11_ATOMICS)
    if(FE_QJS_HAS_C11_ATOMICS)
        target_compile_options(quickjs PRIVATE /experimental:c11atomics)
    endif()
    # 静默 C 库源的噪声警告(与 quickjs-ng 官方 /wd 列表对齐)
    target_compile_options(quickjs PRIVATE
        /wd4018 /wd4100 /wd4127 /wd4146 /wd4200 /wd4244 /wd4245 /wd4267
        /wd4388 /wd4389 /wd4456 /wd4457 /wd4710 /wd4711 /wd4820 /wd5045
    )
endif()

target_compile_definitions(quickjs PRIVATE WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0601)
