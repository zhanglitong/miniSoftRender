cmake_minimum_required(VERSION 3.15)

set(TARGET_NAME tool.resource.cc)

file(GLOB_RECURSE HEADER_FILES *.h *.hpp)
file(GLOB_RECURSE SOURCE_FILES *.c *.cc *.cpp) 

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${HEADER_FILES})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCE_FILES})

add_executable(${TARGET_NAME}
	${HEADER_FILES}
	${SOURCE_FILES}
)

if(UNIX)
    target_compile_definitions(${TARGET_NAME} PRIVATE -DPOSIX -DLINUX)
endif()

target_include_directories(${TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
)
target_include_directories(${TARGET_NAME} PRIVATE 
    ${FLEX_INCLUDE_DIRS}
)

target_compile_definitions(${TARGET_NAME} PRIVATE -DNOMINMAX -DUNICODE -D_UNICODE -D_CRT_SECURE_NO_WARNINGS)

set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "tool")

# -------------------------------------------------------------------------
# 静态链接 C/C++ 运行时，生成不依赖运行时 DLL 的独立可执行文件
#   MSVC  : /MT (Release) /MTd (Debug)  →  静态 CRT，不依赖 vcruntime/msvcp/ucrt DLL
#   GCC   : -static 静态链接所有库（含 libstdc++/libgcc/libwinpthread 等）
# -------------------------------------------------------------------------
if(MSVC)
    # 将默认的动态 CRT (/MD /MDd) 替换为静态 CRT (/MT /MTd)
    foreach(_cfg "" "_DEBUG" "_RELEASE" "_MINSIZEREL" "_RELWITHDEBINFO")
        string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS${_cfg} "${CMAKE_CXX_FLAGS${_cfg}}")
        string(REPLACE "/MD" "/MT" CMAKE_C_FLAGS${_cfg}   "${CMAKE_C_FLAGS${_cfg}}")
    endforeach()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_link_options(${TARGET_NAME} PRIVATE
        -static
        -static-libgcc
        -static-libstdc++
    )
endif()