# ===== Lua 5.4.7:源码内置(嵌入脚本,最小拷贝)=====
# 仅拷贝解释器库源码(lapi.c..lvm.c + linit.c 提供 luaL_openlibs),
# 排除独立可执行入口 lua.c(lua 解释器 CLI)、luac.c(字节码编译器 CLI)及 doc/Makefile。
file(GLOB_RECURSE HEADER_FILES src/*.h src/*.hpp)
file(GLOB_RECURSE SOURCE_FILES src/*.c)

add_library(lua STATIC
            ${HEADER_FILES}
            ${SOURCE_FILES}
)

target_include_directories(lua PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
