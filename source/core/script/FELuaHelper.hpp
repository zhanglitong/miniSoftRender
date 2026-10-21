#pragma     once
/// FELuaHelper
/// Lua 绑定内部辅助:
///   1. LuaEnv            宿主环境(ctx/scene 指针),存注册表供各绑定模块取回
///   2. LuaPtr<T>/pushObj/checkObj    引擎对象 <-> userdata(持 SharedPtr,__gc 减引用)
///   3. math 转换         Lua table <-> real2/real3/quat/uint8x4/KFValue
/// 仅被 FELuaVM.cpp / FELuaAnim.cpp 包含,不对外导出

#include    <lua.hpp>

#include    "../inc/FEContext.hpp"
#include    "../inc/node/FENode.hpp"
#include    "../inc/mesh/FEMesh.hpp"
#include    "../inc/graphic/FEScene.h"
#include    "../inc/animation/FEAnimationHelper.hpp"
#include    "../inc/animation/FEAnimationSys.hpp"

namespace   FE
{
    class   FEScene;

    /// <summary>
    /// 脚本宿主环境,生命周期由 FELuaVM 管理
    /// scene 由宿主(setScene)持有,绑定代码不负责其生命周期
    /// </summary>
    struct  LuaEnv
    {
        FEContext*  ctx     =   nullptr;
        FEScene*    scene   =   nullptr;
    };

    /// <summary>
    /// 宿主环境的注册表键
    /// getEnv/setEnv 必须共用同一把键(各写一份 static 会得到两个不同地址,
    /// 导致 setEnv 写入的环境 getEnv 读不到,返回 nullptr)
    /// </summary>
    inline  void*   envKey()
    {
        static  char    key;
        return  (void*)&key;
    }

    /// <summary>
    /// 从注册表取回宿主环境
    /// </summary>
    inline  LuaEnv* getEnv(lua_State* L)
    {
        lua_pushlightuserdata(L,envKey());
        lua_rawget(L,LUA_REGISTRYINDEX);
        auto*   env     =   (LuaEnv*)lua_touserdata(L,-1);
        lua_pop(L,1);
        return  env;
    }

    /// <summary>
    /// 写入宿主环境到注册表
    /// </summary>
    inline  void    setEnv(lua_State* L,LuaEnv* env)
    {
        lua_pushlightuserdata(L,envKey());
        lua_pushlightuserdata(L,(void*)env);
        lua_rawset(L,LUA_REGISTRYINDEX);
    }

    /// <summary>
    /// userdata 包装:持有一份 SharedPtr,保证对象在 Lua 侧存活
    /// __gc 时析构,引用计数自动减一
    /// </summary>
    template<class T>
    struct  LuaPtr
    {
        SharedPtr<T>    obj;
    };

    /// <summary>
    /// 各绑定类型的注册表metatable名
    /// </summary>
    template<class T>   constexpr const char*   metaName()              {   return  "FE.Unknown";   }
    template<> constexpr const char*    metaName<FENode>()          {   return  "FE.Node";          }
    template<> constexpr const char*    metaName<FEMesh>()          {   return  "FE.Mesh";          }
    template<> constexpr const char*    metaName<FEAnimation>()     {   return  "FE.Animation";     }
    template<> constexpr const char*    metaName<FEAnimClip>()      {   return  "FE.AnimClip";      }
    template<> constexpr const char*    metaName<FEKeyFrameTrack>() {   return  "FE.KeyFrameTrack"; }
    template<> constexpr const char*    metaName<FEPlayback>()      {   return  "FE.Playback";      }
    template<> constexpr const char*    metaName<FEAnimationSys>()  {   return  "FE.AnimationSys";  }

    /// 数学类型 userdata(vec/quat/mat)注册与推送辅助
    /// 依赖上方 metaName 模板,注册实现在 FELuaMath.cpp
    #include    "FELuaMath.hpp"

    /// <summary>
    /// 引擎对象压栈(nullptr 压 nil)
    /// </summary>
    template<class T>
    void    pushObj(lua_State* L,const SharedPtr<T>& obj)
    {
        if (obj == nullptr)
        {
            lua_pushnil(L);
            return;
        }
        auto*   p   =   (LuaPtr<T>*)lua_newuserdatauv(L,sizeof(LuaPtr<T>),0);
        new (p) LuaPtr<T>{obj};
        luaL_setmetatable(L,metaName<T>());
    }

    /// <summary>
    /// 从栈上取引擎对象,类型不符抛 Lua 错误
    /// </summary>
    template<class T>
    SharedPtr<T>    checkObj(lua_State* L,int idx)
    {
        auto*   p   =   (LuaPtr<T>*)luaL_checkudata(L,idx,metaName<T>());
        return  p->obj;
    }

    template<class T>
    int     gcObj(lua_State* L)
    {
        auto*   p   =   (LuaPtr<T>*)lua_touserdata(L,1);
        if (p)
            p->~LuaPtr<T>();
        return  0;
    }

    template<class T>
    int     toStringObj(lua_State* L)
    {
        auto*   p   =   (LuaPtr<T>*)luaL_checkudata(L,1,metaName<T>());
        lua_pushfstring(L,"%s: %p",metaName<T>(),(void*)p->obj.get());
        return  1;
    }

    /// <summary>
    /// 注册一个绑定类型:注册表metatable + __index方法表 + __gc + __tostring
    /// </summary>
    template<class T>
    void    registerType(lua_State* L,const luaL_Reg* methods)
    {
        if (luaL_newmetatable(L,metaName<T>()))
        {
            lua_newtable(L);
            luaL_setfuncs(L,methods,0);
            lua_setfield(L,-2,"__index");
            lua_pushcfunction(L,gcObj<T>);
            lua_setfield(L,-2,"__gc");
            lua_pushcfunction(L,toStringObj<T>);
            lua_setfield(L,-2,"__tostring");
        }
        lua_pop(L,1);
    }

    /// <summary>
    /// 从 Lua table 读 real2,兼容 {x=,y=} 与 {1,2}
    /// </summary>
    inline  bool    toReal2(lua_State* L,int idx,real2& out)
    {
        real2   t;
        if (testMath(L,idx,t))
        {
            out =   t;
            return  true;
        }
        if (!lua_istable(L,idx))
            return  false;
        auto    read    =   [&](const char* key,int pos,real& v)
        {
            lua_getfield(L,idx,key);
            if (lua_isnumber(L,-1))
                v   =   lua_tonumber(L,-1);
            else
            {
                lua_pop(L,1);
                lua_geti(L,idx,pos);
                if (!lua_isnumber(L,-1))
                {
                    lua_pop(L,1);
                    return  false;
                }
                v   =   lua_tonumber(L,-1);
            }
            lua_pop(L,1);
            return  true;
        };
        return  read("x",1,out.x) && read("y",2,out.y);
    }

    /// <summary>
    /// 从 Lua table 读 real3,兼容 {x=,y=,z=} 与 {1,2,3}
    /// </summary>
    inline  bool    toReal3(lua_State* L,int idx,real3& out)
    {
        real3   t;
        if (testMath(L,idx,t))
        {
            out =   t;
            return  true;
        }
        if (!lua_istable(L,idx))
            return  false;
        auto    read    =   [&](const char* key,int pos,real& v)
        {
            lua_getfield(L,idx,key);
            if (lua_isnumber(L,-1))
                v   =   lua_tonumber(L,-1);
            else
            {
                lua_pop(L,1);
                lua_geti(L,idx,pos);
                if (!lua_isnumber(L,-1))
                {
                    lua_pop(L,1);
                    return  false;
                }
                v   =   lua_tonumber(L,-1);
            }
            lua_pop(L,1);
            return  true;
        };
        return  read("x",1,out.x) && read("y",2,out.y) && read("z",3,out.z);
    }

    /// <summary>
    /// 从 Lua table 读四元数 {x=,y=,z=,w=} 或 {1,2,3,4}
    /// </summary>
    inline  bool    toQuatr(lua_State* L,int idx,quatr& out)
    {
        quatr   q;
        if (testMath(L,idx,q))
        {
            out =   q;
            return  true;
        }
        real3   v3;
        if (!toReal3(L,idx,v3))
            return  false;
        real    w   =   0;
        lua_getfield(L,idx,"w");
        if (lua_isnumber(L,-1))
            w   =   lua_tonumber(L,-1);
        else
        {
            lua_pop(L,1);
            lua_geti(L,idx,4);
            if (!lua_isnumber(L,-1))
            {
                lua_pop(L,1);
                return  false;
            }
            w   =   lua_tonumber(L,-1);
        }
        lua_pop(L,1);
        out =   quatr(v3.x,v3.y,v3.z,w);
        return  true;
    }

    /// <summary>
    /// 从 Lua table 读颜色 {r=,g=,b=,a=} 或 {1,2,3,4},分量 0-255
    /// </summary>
    inline  bool    toUint8x4(lua_State* L,int idx,uint8x4& out)
    {
        if (!lua_istable(L,idx))
            return  false;
        auto    read    =   [&](const char* key,int pos,uint8& v)
        {
            lua_getfield(L,idx,key);
            if (lua_isnumber(L,-1))
                v   =   (uint8)lua_tointeger(L,-1);
            else
            {
                lua_pop(L,1);
                lua_geti(L,idx,pos);
                if (!lua_isnumber(L,-1))
                {
                    lua_pop(L,1);
                    return  false;
                }
                v   =   (uint8)lua_tointeger(L,-1);
            }
            lua_pop(L,1);
            return  true;
        };
        return  read("r",1,out.x) && read("g",2,out.y) && read("b",3,out.z) && read("a",4,out.w);
    }

    /// <summary>
    /// table -> KFValue 启发式:
    ///   boolean -> uint8(0/1);  number -> real
    ///   {r,g,b,a} -> uint8x4;   {x,y} -> real2;  {x,y,z} -> real3;  {x,y,z,w} -> real4
    /// </summary>
    inline  bool    toKFValue(lua_State* L,int idx,KFValue& out)
    {
        real2   v2;
        if (testMath(L,idx,v2))
        {
            out =   KFValue(v2);
            return  true;
        }
        real3   v3;
        if (testMath(L,idx,v3))
        {
            out =   KFValue(v3);
            return  true;
        }
        real4   v4;
        if (testMath(L,idx,v4))
        {
            out =   KFValue(v4);
            return  true;
        }
        quatr   q;
        if (testMath(L,idx,q))
        {
            out =   KFValue(q);
            return  true;
        }
        if (lua_isboolean(L,idx))
        {
            out =   KFValue((uint8)(lua_toboolean(L,idx) ? 1 : 0));
            return  true;
        }
        if (lua_isnumber(L,idx))
        {
            out =   KFValue((real)lua_tonumber(L,idx));
            return  true;
        }
        if (!lua_istable(L,idx))
            return  false;

        uint8x4 rgba;
        if (toUint8x4(L,idx,rgba))
        {
            out =   KFValue(rgba);
            return  true;
        }
        real3   tv3;
        if (!toReal3(L,idx,tv3))
            return  false;
        lua_getfield(L,idx,"w");
        bool    bHasW   =   lua_isnumber(L,-1);
        real    w       =   0;
        if (bHasW)
            w   =   lua_tonumber(L,-1);
        lua_pop(L,1);
        if (!bHasW)
        {
            lua_geti(L,idx,4);
            bHasW   =   lua_isnumber(L,-1);
            if (bHasW)
                w   =   lua_tonumber(L,-1);
            lua_pop(L,1);
        }
        if (bHasW)
            out =   KFValue(real4(tv3.x,tv3.y,tv3.z,w));
        else
            out =   KFValue(tv3);
        return  true;
    }

    /// pushReal2/pushReal3/pushQuatr/pushMat3/pushMat4 由 FELuaMath.hpp 提供(输出 math userdata)
    inline  void    pushUint8x4(lua_State* L,const uint8x4& v)
    {
        lua_createtable(L,0,4);
        lua_pushinteger(L,v.x); lua_setfield(L,-2,"r");
        lua_pushinteger(L,v.y); lua_setfield(L,-2,"g");
        lua_pushinteger(L,v.z); lua_setfield(L,-2,"b");
        lua_pushinteger(L,v.w); lua_setfield(L,-2,"a");
    }

    inline  void    pushReal2Range(lua_State* L,const real2& v)
    {
        lua_createtable(L,0,2);
        lua_pushnumber(L,v.x);  lua_setfield(L,-2,"min");
        lua_pushnumber(L,v.y);  lua_setfield(L,-2,"max");
    }
}
