set(TARGET_NAME tool.animation.editor)

project(${TARGET_NAME} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Vulkan REQUIRED)

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

if(NOT CMAKE_PREFIX_PATH)
    if(EXISTS "D:/Qt/Qt6.2.4/6.2.4/msvc2019_64")
        list(APPEND CMAKE_PREFIX_PATH "D:/Qt/Qt6.2.4/6.2.4/msvc2019_64")
    elseif(EXISTS "$ENV{QTDIR}")
        list(APPEND CMAKE_PREFIX_PATH "$ENV{QTDIR}")
    elseif(EXISTS "$ENV{QTDIR64}")
        list(APPEND CMAKE_PREFIX_PATH "$ENV{QTDIR64}")
    endif()
endif()

find_package(Qt6 COMPONENTS Core Gui Widgets)

if(NOT Qt6_FOUND)
    message(STATUS "Qt6 not found, skipping tool.animation.editor")
    return()
endif()

add_definitions(-DQT_USE_QSTRING_FROM_CHARS)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

file(GLOB_RECURSE HEADER_FILES *.h *.hpp)
file(GLOB_RECURSE SOURCE_FILES *.c *.cc *.cpp)
file(GLOB_RECURSE FORM_FILES *.ui)
file(GLOB_RECURSE RESOURCE_FILES *.qrc)

source_group(Header FILES ${HEADER_FILES})
source_group(Source FILES ${SOURCE_FILES})


source_group(Forms      FILES ${FORM_FILES})
source_group(Resources  FILES ${RESOURCE_FILES})

add_executable(${TARGET_NAME}
    ${HEADER_FILES}
    ${SOURCE_FILES}
    ${FORM_FILES}
    ${RESOURCE_FILES}
)

target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/../..inc)

target_link_libraries(${TARGET_NAME} PUBLIC Vulkan::Headers)
target_link_libraries(${TARGET_NAME} PRIVATE core)
target_link_libraries(${TARGET_NAME} PRIVATE rs.vulkan)
target_link_libraries(${TARGET_NAME} PRIVATE rs.webgpu)

if(UNIX)
    target_compile_definitions(${TARGET_NAME} PRIVATE -DPOSIX -DLINUX)
endif()

target_include_directories(${TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
)
target_include_directories(${TARGET_NAME} PRIVATE
    ${FLEX_INCLUDE_DIRS}
)

target_link_libraries(${TARGET_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

target_compile_definitions(${TARGET_NAME} PRIVATE -DNOMINMAX -DUNICODE -D_UNICODE -D_CRT_SECURE_NO_WARNINGS)

set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "tool")

# -------------------------------------------------------------------------
# Deploy Qt runtime dependencies (DLLs + plugins) into the exe directory
# 调试运行缺 Qt6Core.dll / platforms 插件等都由 windeployqt 自动补齐
# -------------------------------------------------------------------------
if(WIN32)
    # 从 Qt6::Core 反推 windeployqt 路径，保证构建套件 (msvc vs mingw) 一致
    get_target_property(_qt6_core_dll Qt6::Core IMPORTED_LOCATION_RELEASE)
    if(NOT _qt6_core_dll)
        get_target_property(_qt6_core_dll Qt6::Core IMPORTED_LOCATION_DEBUG)
    endif()
    if(_qt6_core_dll)
        get_filename_component(_qt_bin_dir "${_qt6_core_dll}" DIRECTORY)
        find_program(_WINDEPLOYQT_EXECUTABLE
            NAMES windeployqt
            HINTS "${_qt_bin_dir}"
            NO_DEFAULT_PATH
        )
    endif()

    if(_WINDEPLOYQT_EXECUTABLE)
        # ------------------------------------------------------------------
        # 为什么不在 COMMAND 参数里写 $<$<CONFIG:Debug>:--debug> / $<IF:...>：
        #   VS 多配置生成器 + VERBATIM 下，"一个条件性的参数级 genex" 在不命中时
        #   会展开为 "" 空参数，而 "整个 COMMAND 用 ; 拼接成 list 的 genex" 又会
        #   因为内部路径含空格 + genex 与 VERBATIM 的二次转义产生：
        #     ""$"<IF:1,\"...\windeployqt.exe\" ...">>
        #   这样的损坏命令行（文件名、目录名或卷标语法不正确 / 退出码 123）。
        #
        # 本项目只在工具工程里用 windeployqt，配置简单，因此采用最小可行方案：
        #   1) 不向 windeployqt 传 --debug / --release。
        #      Qt5/Qt6 的 windeployqt 默认扫描 exe 的 import 表，检测到链接
        #      Qt6Cored.dll 就部署 Debug 版，检测到 Qt6Core.dll 就部署 Release 版。
        #   2) 用 $<TARGET_FILE_DIR:${TARGET_NAME}> 而不是相对路径，避免
        #      MSBuild 把 $(OutDir) 带空格时解析出错。
        # ------------------------------------------------------------------
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND "${_WINDEPLOYQT_EXECUTABLE}"
                    --no-compiler-runtime
                    --no-opengl-sw
                    --no-system-d3d-compiler
                    --no-translations
                    --no-quick-import
                    --dir "$<TARGET_FILE_DIR:${TARGET_NAME}>"
                    "$<TARGET_FILE:${TARGET_NAME}>"
            COMMENT "[tool.animation.editor] windeployqt: deploying Qt DLLs into $<TARGET_FILE_DIR:${TARGET_NAME}> ..."
            VERBATIM
        )
    else()
        message(WARNING "windeployqt not found for tool.animation.editor. "
                        "Running the executable will fail unless Qt DLLs are on PATH.")
    endif()
endif()