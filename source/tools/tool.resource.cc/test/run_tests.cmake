# =========================================================================
# tool.resource.cc 自动化测试
#   由 CMakeLists.txt 以脚本模式调用:
#     cmake -DTOOL=<tool.resource.cc 可执行文件>
#           -DFIXTURE=<测试数据目录>
#           -DOUT=<输出目录>
#           -P run_tests.cmake
# =========================================================================

if(NOT TOOL OR NOT FIXTURE OR NOT OUT)
    message(FATAL_ERROR "必须指定 TOOL / FIXTURE / OUT")
endif()

file(TO_CMAKE_PATH "${TOOL}"    TOOL)
file(TO_CMAKE_PATH "${FIXTURE}" FIXTURE)
file(TO_CMAKE_PATH "${OUT}"     OUT)

set_property(GLOBAL PROPERTY TEST_PASSED 0)
set_property(GLOBAL PROPERTY TEST_FAILED 0)

# ---------------------------------------------------------------- 断言工具
function(pass desc)
    message(STATUS "[ PASS ] ${desc}")
    get_property(_count GLOBAL PROPERTY TEST_PASSED)
    math(EXPR _count "${_count} + 1")
    set_property(GLOBAL PROPERTY TEST_PASSED ${_count})
endfunction()

function(fail desc)
    message(STATUS "[ FAIL ] ${desc}")
    get_property(_count GLOBAL PROPERTY TEST_FAILED)
    math(EXPR _count "${_count} + 1")
    set_property(GLOBAL PROPERTY TEST_FAILED ${_count})
endfunction()

# 运行一次工具,ARGN 为完整命令行参数
function(run_tool)
    execute_process(COMMAND "${TOOL}" ${ARGN}
                    RESULT_VARIABLE _rc
                    OUTPUT_VARIABLE _out
                    ERROR_VARIABLE  _err)
    set(RUN_RC  "${_rc}"        PARENT_SCOPE)
    set(RUN_OUT "${_out}${_err}" PARENT_SCOPE)
endfunction()

function(expect_rc expected desc)
    if("${RUN_RC}" STREQUAL "${expected}")
        pass("${desc}(退出码=${expected})")
    else()
        fail("${desc}: 期望退出码=${expected}, 实际=${RUN_RC}")
    endif()
endfunction()

function(expect_text text needle desc)
    string(FIND "${text}" "${needle}" _pos)
    if(_pos LESS 0)
        fail("${desc}: 未找到 '${needle}'")
    else()
        pass("${desc}")
    endif()
endfunction()

# 统计文本中 needle 的出现次数,与期望值比较
function(expect_count text needle expected desc)
    string(REGEX MATCHALL "${needle}" _items "${text}")
    list(LENGTH _items _count)
    if(_count EQUAL expected)
        pass("${desc}(数量=${_count})")
    else()
        fail("${desc}: 实际=${_count}, 期望=${expected}")
    endif()
endfunction()

# ---------------------------------------------------------------- 准备
file(REMOVE_RECURSE "${OUT}")
file(MAKE_DIRECTORY "${OUT}")

set(DATA_DIR   "${FIXTURE}/data")
set(RES_DIR    "${FIXTURE}/res")
set(FERS_FILE  "${OUT}/data.fers")
set(FERS_COPY  "${OUT}/data_copy.fers")
set(CPP_FILE   "${OUT}/data.fers.cpp")

# ---------------------------------------------------------------- 用例1: dir 模式
message(STATUS "---- 用例1: -m dir 生成 .fers ----")
run_tool(-w "${FIXTURE}" -m dir -i "${DATA_DIR}" -f "${FERS_FILE}")
expect_rc(0 "dir 模式生成 .fers")

if(EXISTS "${FERS_FILE}")
    pass("dir 模式输出文件存在")
    file(READ "${FERS_FILE}" FERS_TEXT)
    expect_text("${FERS_TEXT}" "<root dataPath=" "root 节点与 dataPath 属性")
    expect_text("${FERS_TEXT}" "singleOut=\"1\"" "root 节点 singleOut 属性")
    expect_text("${FERS_TEXT}" "<folder path=\"data\">" "顶层 folder 节点")
    expect_text("${FERS_TEXT}" "<file name=\"data/alpha.bin\" input=\"alpha.bin\"/>" "文件节点 alpha.bin")
    expect_text("${FERS_TEXT}" "<file name=\"data/alpha.txt\" input=\"alpha.txt\"/>" "文件节点 alpha.txt")
    expect_text("${FERS_TEXT}" "<file name=\"data/notes.txt\" input=\"notes.txt\"/>" "文件节点 notes.txt")
    expect_text("${FERS_TEXT}" "<folder path=\"sub\">" "子目录 folder 节点")
    expect_text("${FERS_TEXT}" "<file name=\"data/sub/beta.txt\" input=\"beta.txt\"/>" "子目录文件节点 beta.txt")

    # 目录内容按名称排序(alpha.bin < alpha.txt < notes.txt < sub)
    string(FIND "${FERS_TEXT}" "data/alpha.bin"  _pos1)
    string(FIND "${FERS_TEXT}" "data/alpha.txt"  _pos2)
    string(FIND "${FERS_TEXT}" "data/notes.txt"  _pos3)
    string(FIND "${FERS_TEXT}" "path=\"sub\""    _pos4)
    if(_pos1 GREATER -1 AND _pos2 GREATER _pos1 AND _pos3 GREATER _pos2 AND _pos4 GREATER _pos3)
        pass("目录遍历结果按名称稳定排序")
    else()
        fail("目录遍历结果未按名称排序(${_pos1},${_pos2},${_pos3},${_pos4})")
    endif()
else()
    fail("dir 模式输出文件不存在")
endif()

# ---------------------------------------------------------------- 用例2: 可复现
message(STATUS "---- 用例2: 重复生成一致 ----")
set(FERS_TEXT "")
if(EXISTS "${FERS_FILE}")
    file(READ "${FERS_FILE}" FERS_TEXT)
endif()
run_tool(-w "${FIXTURE}" -m dir -i "${DATA_DIR}" -f "${FERS_COPY}")
expect_rc(0 "dir 模式二次生成")
if(EXISTS "${FERS_COPY}")
    file(READ "${FERS_COPY}" FERS_COPY_TEXT)
    if("${FERS_TEXT}" STREQUAL "${FERS_COPY_TEXT}")
        pass("两次生成的 .fers 完全一致(可复现)")
    else()
        fail("两次生成的 .fers 不一致")
    endif()
else()
    fail("二次生成输出文件不存在")
endif()

# ---------------------------------------------------------------- 用例3: fers 模式
message(STATUS "---- 用例3: -m fers 生成 .cpp ----")
run_tool(-w "${FIXTURE}" -m fers -i "${FERS_FILE}" -o "${CPP_FILE}")
expect_rc(0 "fers 模式生成 .cpp")

if(EXISTS "${CPP_FILE}")
    file(READ "${CPP_FILE}" CPP_TEXT)
    expect_text("${CPP_TEXT}" "namespace  FE" "生成 namespace FE")
    expect_text("${CPP_TEXT}" "FE::FEAssetsMgr::instance().addBuffer(\"data/alpha.bin\"," "注册 data/alpha.bin")
    expect_text("${CPP_TEXT}" "FE::FEAssetsMgr::instance().addBuffer(\"data/alpha.txt\"," "注册 data/alpha.txt")
    expect_text("${CPP_TEXT}" "FE::FEAssetsMgr::instance().addBuffer(\"data/notes.txt\"," "注册 data/notes.txt")
    expect_text("${CPP_TEXT}" "FE::FEAssetsMgr::instance().addBuffer(\"data/sub/beta.txt\"," "注册 data/sub/beta.txt")
    expect_count("${CPP_TEXT}" "addBuffer\\(" 4 "addBuffer 注册数量")

    # 数据长度变量
    foreach(_rel IN ITEMS "data/alpha.bin" "data/alpha.txt" "data/notes.txt" "data/sub/beta.txt")
        file(READ "${FIXTURE}/${_rel}" _hex HEX)
        string(LENGTH "${_hex}" _hexLen)
        math(EXPR _size "${_hexLen} / 2")
        expect_text("${CPP_TEXT}" "_len      =   ${_size};" "${_rel} 长度=${_size}")
    endforeach()

    # 字节数组逐字节校验:所有 0xXX 拼接后应与 4 个源文件内容(+ 结尾 0x00)完全一致
    string(REGEX MATCHALL "0x[0-9a-fA-F][0-9a-fA-F]" BYTE_TOKENS "${CPP_TEXT}")
    list(JOIN BYTE_TOKENS "" ACTUAL_HEX)
    string(REGEX REPLACE "0x" "" ACTUAL_HEX "${ACTUAL_HEX}")
    string(TOLOWER "${ACTUAL_HEX}" ACTUAL_HEX)

    set(EXPECT_HEX "")
    foreach(_rel IN ITEMS "data/alpha.bin" "data/alpha.txt" "data/notes.txt" "data/sub/beta.txt")
        file(READ "${FIXTURE}/${_rel}" _hex HEX)
        string(TOLOWER "${_hex}" _hex)
        string(APPEND EXPECT_HEX "${_hex}00")
    endforeach()

    list(LENGTH BYTE_TOKENS _tokenCount)
    string(LENGTH "${EXPECT_HEX}" _expectLen)
    math(EXPR _expectCount "${_expectLen} / 2")
    if(_tokenCount EQUAL _expectCount AND "${ACTUAL_HEX}" STREQUAL "${EXPECT_HEX}")
        pass("字节数组与源文件逐字节一致(${_expectCount} 字节含结尾 0x00)")
    else()
        fail("字节数组不一致: 字节数=${_tokenCount}, 期望=${_expectCount}")
    endif()
else()
    fail("fers 模式输出文件不存在")
endif()

# ---------------------------------------------------------------- 用例4: qrc 模式
message(STATUS "---- 用例4: -m qrc 生成 .cpp ----")
run_tool(-w "${FIXTURE}" -m qrc -i "${RES_DIR}/test.qrc" -o "${OUT}/test.qrc.cpp")
expect_rc(0 "qrc 模式生成 .cpp")

if(EXISTS "${OUT}/test.qrc.cpp")
    file(READ "${OUT}/test.qrc.cpp" QRC_TEXT)
    # prefix 与资源 key 规范: :/prefix/name
    expect_text("${QRC_TEXT}" "addBuffer(\":/EditorRes/alpha.txt\"," "prefix + 相对路径 -> :/EditorRes/alpha.txt")
    expect_text("${QRC_TEXT}" "addBuffer(\":/EditorRes/renamed.txt\"," "alias 覆盖资源名 -> :/EditorRes/renamed.txt")
    expect_text("${QRC_TEXT}" "addBuffer(\":/Lang/alpha.txt@fr\"," "qresource 级 lang 追加 @fr")
    expect_text("${QRC_TEXT}" "addBuffer(\":/Trim/sub/gamma.txt\"," "跨行 <file> 文本被 trim")
    expect_count("${QRC_TEXT}" "addBuffer\\(" 4 "qrc addBuffer 注册数量")

    # alias 指向 beta.txt,内容应来自 beta.txt
    file(READ "${RES_DIR}/beta.txt" BETA_HEX HEX)
    string(TOLOWER "${BETA_HEX}" BETA_HEX)
    string(REGEX MATCHALL "0x[0-9a-fA-F][0-9a-fA-F]" QRC_BYTES "${QRC_TEXT}")
    list(JOIN QRC_BYTES "" QRC_HEX)
    string(REGEX REPLACE "0x" "" QRC_HEX "${QRC_HEX}")
    string(TOLOWER "${QRC_HEX}" QRC_HEX)
    if(QRC_HEX MATCHES "${BETA_HEX}00")
        pass("alias 资源内容来自被引用文件 beta.txt")
    else()
        fail("alias 资源内容与 beta.txt 不一致")
    endif()
else()
    fail("qrc 模式输出文件不存在")
endif()

# ---------------------------------------------------------------- 用例5: 错误路径
message(STATUS "---- 用例5: 错误路径返回非 0 ----")
run_tool(-w "${FIXTURE}" -m unknown -i "${DATA_DIR}")
if("${RUN_RC}" STREQUAL "0")
    fail("未知 mode 未返回非 0")
else()
    pass("未知 mode 返回非 0(${RUN_RC})")
endif()
expect_text("${RUN_OUT}" "unknown mode" "未知 mode 有错误提示")

run_tool(-m fers -w "${FIXTURE}")
if("${RUN_RC}" STREQUAL "0")
    fail("缺少 -i 参数未返回非 0")
else()
    pass("缺少必填参数返回非 0(${RUN_RC})")
endif()

run_tool(-w "${FIXTURE}" -m fers -i "${OUT}/not_exists.fers" -o "${OUT}/not_exists.cpp")
if("${RUN_RC}" STREQUAL "0")
    fail("输入文件不存在未返回非 0")
else()
    pass("输入文件不存在返回非 0(${RUN_RC})")
endif()
if(EXISTS "${OUT}/not_exists.cpp")
    fail("输入文件不存在时仍生成了残留 .cpp")
else()
    pass("输入文件不存在时无残留 .cpp")
endif()

# 重复资源 key: 应返回非 0 并提示 duplicate resource key
run_tool(-w "${FIXTURE}" -m qrc -i "${RES_DIR}/test_dup.qrc" -o "${OUT}/test_dup.qrc.cpp")
if("${RUN_RC}" STREQUAL "0")
    fail("重复资源 key 未返回非 0")
else()
    pass("重复资源 key 返回非 0(${RUN_RC})")
endif()
expect_text("${RUN_OUT}" "duplicate resource key" "重复资源 key 有错误提示")

# 根标签不是 RCC: 应返回非 0 并提示 root element must be <RCC>
run_tool(-w "${FIXTURE}" -m qrc -i "${RES_DIR}/test_badroot.qrc" -o "${OUT}/test_badroot.qrc.cpp")
if("${RUN_RC}" STREQUAL "0")
    fail("错误的根标签未返回非 0")
else()
    pass("错误的根标签返回非 0(${RUN_RC})")
endif()
expect_text("${RUN_OUT}" "root element must be <RCC>" "错误的根标签有错误提示")

# ---------------------------------------------------------------- 汇总
get_property(PASSED GLOBAL PROPERTY TEST_PASSED)
get_property(FAILED GLOBAL PROPERTY TEST_FAILED)
if(FAILED GREATER 0)
    message(FATAL_ERROR "测试结束: 通过 ${PASSED} 项, 失败 ${FAILED} 项")
endif()
message(STATUS "测试结束: 全部通过 ${PASSED} 项")
