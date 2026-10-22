#pragma     once
/// FELuaVM
/// 引擎 Lua 脚本宿主:
///   1. 创建/销毁 lua_State,装载标准库
///   2. 注册 fe 全局表(fe.findNode / fe.createNode / fe.dispatchNode / fe.log)
///   3. 注册 fe.anim 动画绑定模块(实现于 FELuaAnim.cpp)
/// 典型用法:
///   FELuaVM lua(ctx);
///   lua.setScene(scene.get());
///   lua.doFile("anim.lua");

#include    "../../inc/FEDefine.h"

struct  lua_State;

namespace   FE
{
    class   FEContext;
    class   FEScene;

    class   FE_API  FELuaVM
    {
    public:
        FELuaVM(FEContext& ctx);
        ~FELuaVM();
        FELuaVM(const FELuaVM&)             =   delete;
        FELuaVM&    operator=(const FELuaVM&) = delete;
    public:
        /// <summary>
        /// 关联场景,供 fe.findNode / fe.anim.action 等使用
        /// 可在 VM 存活期间随时切换
        /// </summary>
        void        setScene(FEScene* scene);
        FEScene*    scene() const;
        /// <summary>
        /// Lua 状态机,宿主可自行扩展绑定
        /// </summary>
        lua_State*  state() const;
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
        /// 初始化:标准库 + fe 全局表 + 各绑定模块
        /// </summary>
        void        init();
    private:
        FEContext*  _ctx    =   nullptr;
        FEScene*    _scene  =   nullptr;
        lua_State*  _L      =   nullptr;
    };
}
