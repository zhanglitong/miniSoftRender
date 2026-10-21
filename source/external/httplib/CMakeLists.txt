# ===== cpp-httplib 0.62.0:源码内置(HTTP 客户端/服务端,最小拷贝)=====
# header-only 库,仅拷贝 httplib.h;httplib.cpp 为唯一编译单元,
# 链接 core 时由链接器按需拉取所需符号。未定义 CPPHTTPLIB_OPENSSL_SUPPORT /
# CPPHTTPLIB_ZLIB_SUPPORT,自包含无外部依赖(ws2_32 由源内 pragma comment 引入)。
add_library(httplib STATIC
            httplib.cpp
            httplib.h
)

target_include_directories(httplib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
