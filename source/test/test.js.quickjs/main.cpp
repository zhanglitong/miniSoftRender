/// test.js.quickjs
/// QuickJS-NG 无窗口断言式测试:
///   1. VM 生命周期 + 内置对象(Math/JSON/RegExp/Array/BigInt/Map)
///   2. doString 求值结果断言(int/double/string)
///   3. doFile 执行脚本文件 + fe.log 绑定调用
///   4. 异常(throw)与语法错误路径返回 false
///   5. 递归深函数验证栈余量(/STACK:8388608)
///   6. 场景绑定:scene 未设置错误路径 + 无窗口 FEScene 的
///      fe.findNode/createNode/dispatchNode + script.js 内
///      数学/几何/节点/动画绑定全覆盖断言(globalThis.__t)
/// 用法: test.js.quickjs.exe

#include    <iostream>
#include    <cstring>
#include    <cmath>
#include    "../../inc/FEContext.hpp"
#include    "../../inc/graphic/FEScene.h"
#include    "../../core/script/quickjs/FEQuickJS.hpp"

#include    <quickjs.h>

using   namespace   FE;

static  int     g_failed    =   0;
static  int     g_total     =   0;

static  void    report(const char* name,bool pass,const std::string& detail = "")
{
    g_total++;
    if (pass)
        std::cout << "[PASS] " << name << std::endl;
    else
    {
        std::cout << "[FAIL] " << name << (detail.empty() ? "" : (": " + detail)) << std::endl;
        g_failed++;
    }
}

/// eval 一段代码,异常时返回 JS_EXCEPTION(调用方负责 FreeValue)
static  JSValue evalCode(JSContext* jc,const char* code)
{
    return  JS_Eval(jc,code,strlen(code),"<test>",JS_EVAL_TYPE_GLOBAL);
}

static  void    expectEvalInt(FEQuickJS& js,const char* name,const char* code,int want)
{
    JSContext*  jc  =   js.context();
    JSValue     ret =   evalCode(jc,code);
    int32_t     got =   0;
    bool        ok  =   !JS_IsException(ret) && JS_ToInt32(jc,&got,ret) == 0 && got == want;
    report(name,ok,ok ? "" : ("eval '" + std::string(code) + "' got " + std::to_string(got) + " want " + std::to_string(want)));
    JS_FreeValue(jc,ret);
}

static  void    expectEvalDouble(FEQuickJS& js,const char* name,const char* code,double want)
{
    JSContext*  jc  =   js.context();
    JSValue     ret =   evalCode(jc,code);
    double      got =   0;
    bool        ok  =   !JS_IsException(ret) && JS_ToFloat64(jc,&got,ret) == 0 && fabs(got - want) <= 1e-9;
    report(name,ok,ok ? "" : ("eval '" + std::string(code) + "' got " + std::to_string(got)));
    JS_FreeValue(jc,ret);
}

static  void    expectEvalString(FEQuickJS& js,const char* name,const char* code,const char* want)
{
    JSContext*  jc  =   js.context();
    JSValue     ret =   evalCode(jc,code);
    const char* got =   (!JS_IsException(ret)) ? JS_ToCString(jc,ret) : nullptr;
    bool        ok  =   got != nullptr && strcmp(got,want) == 0;
    report(name,ok,ok ? "" : ("eval '" + std::string(code) + "' got '" + (got ? got : "<exception>") + "' want '" + want + "'"));
    if (got != nullptr)
        JS_FreeCString(jc,got);
    JS_FreeValue(jc,ret);
}

static  void    expectDoString(FEQuickJS& js,const char* name,const char* code,bool wantOk)
{
    bool    ok  =   js.doString(code);
    report(name,ok == wantOk,ok == wantOk ? "" : ("doString '" + std::string(code) + "' returned " + (ok ? "true" : "false")));
}

int     main(int argc,char** argv)
{
    std::cout << "==== FEQuickJS minimal integration tests ====\n";
    std::cout << "quickjs-ng version: " << JS_GetVersion() << "\n";

    FEContext&  ctx =   FEContext::instance();
    FEQuickJS   js(ctx);

    /// JS 栈检查上限默认 1MB,宿主 exe 栈已提至 8MB;放宽到 6MB 供深递归测试
    JS_SetMaxStackSize(JS_GetRuntime(js.context()),6 * 1024 * 1024);

    /// 1. 基础算术与运算符
    expectEvalInt(js,"arithmetic precedence","1 + 2 * 3",7);
    expectEvalInt(js,"integer division floor","-7 / 2 | 0",-3);
    expectEvalDouble(js,"ieee754 double","0.1 + 0.2",0.30000000000000004);
    expectEvalInt(js,"bitwise","(0xF0 | 0x0F) & 0xFF",255);

    /// 2. 函数/闭包/数组方法
    expectEvalInt(js,"closure counter","\
        (function(){\
            let n = 0;\
            const inc = () => ++n;\
            inc(); inc(); inc();\
            return n;\
        })()",3);
    expectEvalInt(js,"array map reduce","[1,2,3].map(x => x * x).reduce((a,b) => a + b,0)",14);
    expectEvalInt(js,"array spread/rest","(function(...a){ return a.length; })(1,2,3,4,5)",5);
    expectEvalInt(js,"BigInt","Number(2n ** 10n)",1024);

    /// 3. 内置对象
    expectEvalDouble(js,"Math.hypot","Math.hypot(3,4)",5);
    expectEvalString(js,"JSON roundtrip","JSON.stringify(JSON.parse('{\"a\":[1,2.5,true,\"s\"]}'))","{\"a\":[1,2.5,true,\"s\"]}");
    expectEvalInt(js,"RegExp","/^a\\.b$/i.test('A.B') ? 1 : 0",1);
    expectEvalInt(js,"Map + Set","\
        (function(){\
            const m = new Map([[1,'one']]);\
            const s = new Set([1,1,2]);\
            return m.get(1) === 'one' && s.size === 2 ? 1 : 0;\
        })()",1);
    expectEvalString(js,"template literal","`x=${1 + 2}, y='${'ab'.repeat(2)}'`","x=3, y='abab'");

    /// 4. 内置对象可用性探测(信息输出,不计断言)
    {
        JSContext*  jc  =   js.context();
        const char* probes[]    =   { "typeof Atomics","typeof structuredClone","typeof WeakRef" };
        for (auto* p : probes)
        {
            JSValue ret =   evalCode(jc,p);
            const char* got =   JS_IsException(ret) ? "<exception>" : JS_ToCString(jc,ret);
            std::cout << "[INFO] " << p << " = " << (got ? got : "?") << "\n";
            if (got != nullptr && !JS_IsException(ret))
                JS_FreeCString(jc,got);
            JS_FreeValue(jc,ret);
        }
    }

    /// 5. fe.log 绑定:不抛异常即为通过(输出见引擎日志)
    expectDoString(js,"fe.log single","fe.log('hello from js')",true);
    expectDoString(js,"fe.log multi-args","fe.log('n =',123,'pi =',Math.round(Math.PI * 100) / 100)",true);
    expectDoString(js,"fe.log object","fe.log('obj =',{a:1},'arr =',[1,2])",true);

    /// 6. 场景绑定:scene 未设置时 fe.findNode/createNode/dispatchNode 必须报错
    expectDoString(js,"findNode without scene throws","fe.findNode('x')",false);
    expectDoString(js,"createNode without scene throws","fe.createNode('x')",false);
    expectDoString(js,"dispatchNode without scene throws","fe.dispatchNode(null)",false);

    /// 无窗口 FEScene:nodeTree 纯数据可用(findNode/createNode/变换/层级),
    /// 动画系统未注册 -> fe.anim.action 走错误路径(由 script.js 断言);
    /// dispatchNode 仅对无组件节点调用,避开无系统时的 Debug assert
    FEScene scene(ctx);
    js.setScene(&scene);
    report("setScene",js.scene() == &scene);

    /// 6.5 绑定快速探针(doFile 前逐类定位崩溃/异常点)
    expectEvalInt(js,"probe fe.vec3","fe.vec3(1,2,3).z",3);
    expectEvalInt(js,"probe fe.quat","fe.quat(1,2,3,4).w",4);
    expectEvalInt(js,"probe fe.mat4","fe.mat4().c3.w",1);
    expectEvalInt(js,"probe fe.geo.box","fe.geo.box().vertexCount() > 0 ? 1 : 0",1);
    expectEvalInt(js,"probe fe.anim.createAnimation","fe.anim.createAnimation() !== null ? 1 : 0",1);
    expectEvalInt(js,"probe fe.createNode","fe.createNode('probe').name() === 'probe' ? 1 : 0",1);

    /// 7. 异常与语法错误路径
    expectDoString(js,"throw Error returns false","throw new Error('boom')",false);
    expectDoString(js,"syntax error returns false","1 +",false);
    expectDoString(js,"ReferenceError returns false","__noSuchGlobal__.foo",false);
    expectEvalInt(js,"try/catch still works","\
        (function(){\
            try { null.foo; } catch (e) { return e instanceof TypeError ? 1 : 0; }\
            return 0;\
        })()",1);

    /// 8. doFile:执行 exe 目录下的 script.js 并断言其副作用
    {
        /// 从 argv[0] 解析 exe 所在目录,拼脚本路径
        std::string exe     =   argc > 0 ? argv[0] : "";
        std::string dir     =   ".";
        size_t      slash   =   exe.find_last_of("/\\");
        if (slash != std::string::npos)
            dir     =   exe.substr(0,slash);
        std::string script  =   dir + "/script.js";
        ctx.log().infor("[main] before doFile: %s",script.c_str());
        bool    ok  =   js.doFile(script.c_str());
        ctx.log().infor("[main] after doFile ok=%d",ok ? 1 : 0);
        report("doFile script.js",ok,ok ? "" : ("doFile '" + script + "' failed, see engine log"));
        if (ok)
        {
            expectEvalInt(js,"doFile side effect","globalThis.__marker",42);
            expectEvalInt(js,"script.js binding assertions","globalThis.__t === true ? 1 : 0",1);
            /// 全过时 __fails 为空串,PASS;失败时 FAIL detail 直接列出断言信息
            expectEvalString(js,"script.js failure detail","globalThis.__fails","");
        }
    }

    /// 9. 递归深度(验证 /STACK:8388608 + JS_SetMaxStackSize(6MB) 配合不崩宿主栈;
    ///    Debug 帧更大,8000 层可能触发 RangeError —— 完成或干净抛 RangeError 均算通过)
    expectEvalInt(js,"deep recursion","\
        (function(){\
            function f(n){ return n <= 0 ? 0 : 1 + f(n - 1); }\
            try { return f(8000) === 8000 ? 1 : 0; } catch (e) { return e instanceof RangeError ? 1 : 0; }\
        })()",1);

    std::cout << "==== " << (g_total - g_failed) << "/" << g_total << " passed ====\n";
    return  g_failed > 0 ? 1 : 0;
}
