set(TARGET_NAME core)

file(GLOB_RECURSE HEADER_FILES *.h *.hpp)
file(GLOB_RECURSE SOURCE_FILES *.c *.cc *.cpp)
list(FILTER SOURCE_FILES EXCLUDE REGEX "\\.fers\\.cpp$")


if(WIN32)
    set(PRRS_COMPILE      ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tool.resource.cc.exe)
else()
    set(PRRS_COMPILE      ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tool.resource.cc)
endif()
set(PRRS_DATA_ROOT        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/../data)
set(PRRS_BIN_ROOT         ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/../data)

# 获取引擎的数据目录，编译器使用它来计算相对目录
get_filename_component(ABS_PATH			${PRRS_DATA_ROOT}	ABSOLUTE)
get_filename_component(ABS_BIN_PATH		${PRRS_BIN_ROOT}	ABSOLUTE)

# 收集用户自定义的fers文件
file(GLOB_RECURSE fers_files "*.fers")
  
set(DIR_LIST "pipeline" "shader")

foreach(cDir ${DIR_LIST})
    set(FERS_INPUT   ${ABS_PATH}/${cDir})
    set(FERS_OUTPUT  ${CMAKE_CURRENT_LIST_DIR}/${cDir}.fers)

    add_custom_command(
            PRE_BUILD
            WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
            OUTPUT  ${FERS_OUTPUT}
            COMMAND ${PRRS_COMPILE} -w ${ABS_BIN_PATH} -m dir -i ${FERS_INPUT} -f ${FERS_OUTPUT}
            DEPENDS ${FERS_INPUT}
   
            COMMENT "compile sub dir to .fers ->${FERS_INPUT}/${cDir}"
        )
        list(APPEND PRRS_OUTPUTS ${FERS_OUTPUT})
endforeach()



foreach(userFERS ${fers_files})
    list(APPEND FERS_OUTPUT ${userFERS})
endforeach()

foreach(prrs_file ${PRRS_OUTPUTS})
   
    set(FERS_INPUT   ${prrs_file})
    set(FERS_OUTPUT  ${prrs_file}.cpp)

    add_custom_command(
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        OUTPUT  ${FERS_OUTPUT}
        COMMAND ${PRRS_COMPILE} -w ${ABS_PATH} -m fers -i ${FERS_INPUT} -o ${FERS_OUTPUT}
        DEPENDS ${FERS_INPUT}
   
        COMMENT "compile resource file ${FERS_INPUT}"
    )
  list(APPEND FERS_CPPS ${FERS_OUTPUT})
endforeach()



file(GLOB_RECURSE INC_FILES "../inc/*.h"  "../inc/*.hpp")

# 2. 自动建立目录树
# ${CMAKE_CURRENT_SOURCE_DIR} 是物理路径的根，生成的 IDE 结构将与之对齐
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${HEADER_FILES})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCE_FILES})

get_filename_component(INC_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../inc" ABSOLUTE)
source_group(TREE "${INC_ROOT}" PREFIX "inc" FILES ${INC_FILES})


source_group(fers       FILES ${fers_files})
source_group(fers_cxx   FILES ${FERS_CPPS})


add_library(${TARGET_NAME} STATIC
            ${HEADER_FILES}
		    ${SOURCE_FILES}
            ${INC_FILES}
            ${fers_files}
            ${FERS_CPPS}
)

add_dependencies(${TARGET_NAME} tool.resource.cc)


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
