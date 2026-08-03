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