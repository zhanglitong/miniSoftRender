set(TARGET_NAME FEViewer)

file(GLOB_RECURSE HEADER_FILES *.h *.hpp)
file(GLOB_RECURSE SOURCE_FILES *.c *.cc *.cpp)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${HEADER_FILES})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCE_FILES})

add_executable(${TARGET_NAME}
        ${HEADER_FILES}
        ${SOURCE_FILES}
)

target_compile_definitions(${TARGET_NAME} PRIVATE -DNOMINMAX -DUNICODE -D_UNICODE -D_CRT_SECURE_NO_WARNINGS)

set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "tools")

if(WIN32)
    target_link_libraries(${TARGET_NAME} core rs.vulkan rs.webgpu shell32 advapi32)
else()
    target_link_libraries(${TARGET_NAME} core rs.vulkan rs.webgpu)
endif()
