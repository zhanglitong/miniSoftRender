set(TARGET_NAME core)

file(GLOB_RECURSE HEADER_FILES *.h *.hpp)
file(GLOB_RECURSE SOURCE_FILES *.c *.cc *.cpp) 

file(GLOB_RECURSE INC_FILES "../inc/*.h"  "../inc/*.hpp")

# 2. 自动建立目录树
# ${CMAKE_CURRENT_SOURCE_DIR} 是物理路径的根，生成的 IDE 结构将与之对齐
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${HEADER_FILES})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCE_FILES})

get_filename_component(INC_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../inc" ABSOLUTE)
source_group(TREE "${INC_ROOT}" PREFIX "inc" FILES ${INC_FILES})

add_library(${TARGET_NAME} STATIC
            ${HEADER_FILES}
		    ${SOURCE_FILES}
            ${INC_FILES}
)



message(STATUS "Using module to find Vulkan")
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

target_include_directories(${TARGET_NAME} PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/../inc)
target_link_libraries(${TARGET_NAME} PUBLIC Vulkan::Headers)


if(UNIX)
    target_compile_definitions(${TARGET_NAME} PRIVATE -DPOSIX -DLINUX)
endif()
target_compile_definitions(${TARGET_NAME} PRIVATE -DNOMINMAX -DUNICODE -D_UNICODE -D_CRT_SECURE_NO_WARNINGS)
