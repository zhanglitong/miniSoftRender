cmake_minimum_required (VERSION 3.8)

# 如果支持，请为 MSVC 编译器启用热重载。
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

if (MSVC)
    target_compile_options(${TARGET} PRIVATE /W4)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${TARGET} PRIVATE -Wall -Wextra)  # 对应 /W4 的级别
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project ("miniSoftRender")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(CMAKE_GENERATOR_PLATFORM)
    set(PLATFORM_SUFFIX "${CMAKE_GENERATOR_PLATFORM}")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(PLATFORM_SUFFIX "x64")
else()
    set(PLATFORM_SUFFIX "x86")
endif()


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY  ${CMAKE_SOURCE_DIR}/bin/$<PLATFORM_ID>$<CONFIG>${PLATFORM_SUFFIX})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY  ${CMAKE_SOURCE_DIR}/lib/$<PLATFORM_ID>$<CONFIG>${PLATFORM_SUFFIX})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY  ${CMAKE_SOURCE_DIR}/lib/$<PLATFORM_ID>$<CONFIG>${PLATFORM_SUFFIX})

# 包含子项目。
add_subdirectory ("source")




