# ===== Dear ImGui:源码内置(引擎内调试 GUI,仅核心库,不含任何 backend) =====
# 静态库直接编入 core;官方 backend(如 imgui_impl_vulkan)依赖 VkRenderPass,
# 与本引擎的动态渲染路径不兼容,故由 rs.vulkan 自写最小渲染器。
set(TARGET_NAME imgui)

file(GLOB IMGUI_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui_draw.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui_widgets.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/imgui_demo.cpp"
)

add_library(${TARGET_NAME} STATIC ${IMGUI_SOURCES})

# 头文件目录(imgui.h 等)对使用者可见
target_include_directories(${TARGET_NAME} PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_compile_definitions(${TARGET_NAME} PRIVATE
    -D_CRT_SECURE_NO_WARNINGS
    -DNOMINMAX
)

if(WIN32)
    # Win32 默认剪贴板/IME 处理需要 imm32
    target_link_libraries(${TARGET_NAME} PRIVATE imm32)
endif()

set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "external")
