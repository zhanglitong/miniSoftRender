#pragma     once
/// FEQuickJS
/// 引擎 QuickJS(NG) 脚本宿主:
///   1. 创建/销毁 JSRuntime/JSContext,JS_NewContext 装载全部内置对象
///   2. 注册 fe 全局表(fe.log/findNode/createNode/dispatchNode)
///   3. 注册数学(fe.vec2/vec3/vec4/quat/mat3/mat4)、几何(fe.geo + FE.Mesh)、
///      动画(fe.anim + FE.Animation 等 5 类)绑定模块
///   4. 提供 doFile/doString 脚本执行入口,出错写引擎日志
/// 典型用法:
///   FEQuickJS js(ctx);
///   js.setScene(scene.get());
///   js.doFile("main.js");

#include    "../../inc/FEDefine.h"

struct  JSRuntime;
struct  JSContext;

namespace   FE
{
    class   FEContext;
    class   FEScene;

    class   FE_API  FEQuickJS
    {
    public:
        FEQuickJS(FEContext& ctx);
        ~FEQuickJS();
        FEQuickJS(const FEQuickJS&)             =   delete;
        FEQuickJS&    operator=(const FEQuickJS&) = delete;
    public:
        /// <summary>
        /// 关联场景,供 fe.findNode / fe.anim.action 等使用
        /// 可在 VM 存活期间随时切换
        /// </summary>
        void        setScene(FEScene* scene);
        FEScene*    scene() const;
        /// <summary>
        /// JS 上下文,宿主可自行扩展绑定
        /// </summary>
        JSContext*  context() const;
        /// <summary>
        /// 执行脚本文件,出错返回 false 并写日志
        /// </summary>
        bool        doFile(const char* file);
        /// <summary>
        /// 执行脚本代码片段,出错返回 false 并写日志
        /// </summary>
        bool        doString(const char* code);
    private:
        /// <summary>
        /// 初始化:内置对象 + fe 全局表 + 各绑定模块
        /// </summary>
        void        init();
    private:
        FEContext*  _ctx    =   nullptr;
        FEScene*    _scene  =   nullptr;
        JSRuntime*  _rt     =   nullptr;
        JSContext*  _js     =   nullptr;
    };
}
