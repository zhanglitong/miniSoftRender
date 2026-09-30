set(TARGET_NAME sample.cylinder)

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

set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "sample/cylinder")

target_link_libraries(${TARGET_NAME} core rs.vulkan rs.webgpu)
