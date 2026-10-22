#pragma once
/// FELuaMath
/// 数学类型 userdata: fe.vec2/vec3/vec4/quat/mat3/mat4(double,与引擎 real 精度一致)
/// 值语义:userdata 内直接存 glm 值,__gc 只做析构,无资源释放
/// 布局依赖:必须在本文件 include 之前已声明 metaName 模板(FELuaHelper.hpp 中),
///          且本文件位于 namespace FE 内部被 include(本文件不能再包一层 namespace FE)
/// 注册实现:FELuaMath.cpp

/// userdata 包装:按值持有数学对象
    template<class V>
    struct  LuaMath
    {
        V   v;
    };

    template<> constexpr const char*    metaName<real2>()   {   return  "FE.Vec2";  }
    template<> constexpr const char*    metaName<real3>()   {   return  "FE.Vec3";  }
    template<> constexpr const char*    metaName<real4>()   {   return  "FE.Vec4";  }
    template<> constexpr const char*    metaName<quatr>()   {   return  "FE.Quat";  }
    template<> constexpr const char*    metaName<mat3r>()   {   return  "FE.Mat3";  }
    template<> constexpr const char*    metaName<mat4r>()   {   return  "FE.Mat4";  }

    /// x/y/z/w -> 0-based 分量下标,不匹配返回 -1
    inline  int     mathComponentIndex(const char* k)
    {
        if (k[1] != 0)
            return  -1;
        switch (k[0])
        {
        case 'x':       return  0;
        case 'y':       return  1;
        case 'z':       return  2;
        case 'w':       return  3;
        default:        return  -1;
        }
    }

    /// 新建一个零值数学对象压栈,返回其 userdata 指针
    template<class V>
    inline  LuaMath<V>* pushMath(lua_State* L)
    {
        auto*   p   =   (LuaMath<V>*)lua_newuserdatauv(L,sizeof(LuaMath<V>),0);
        new (p) LuaMath<V>();
        luaL_setmetatable(L,metaName<V>());
        return  p;
    }

    /// 拷贝一个数学值压栈
    template<class V>
    inline  void    pushMathValue(lua_State* L,const V& v)
    {
        pushMath<V>(L)->v   =   v;
    }

    /// 栈上对象是否为指定数学类型,是则取出值
    template<class V>
    inline  bool    testMath(lua_State* L,int idx,V& out)
    {
        auto*   p   =   (LuaMath<V>*)luaL_testudata(L,idx,metaName<V>());
        if (p == nullptr)
            return  false;
        out =   p->v;
        return  true;
    }

    /// 取指定数学类型,类型不符抛 Lua 错误
    template<class V>
    inline  LuaMath<V>* checkMath(lua_State* L,int idx)
    {
        return  (LuaMath<V>*)luaL_checkudata(L,idx,metaName<V>());
    }

    template<class V>
    inline  int     gcMath(lua_State* L)
    {
        auto*   p   =   (LuaMath<V>*)lua_touserdata(L,1);
        if (p)
            p->~LuaMath<V>();
        return  0;
    }

    inline  void    pushReal2(lua_State* L,const real2& v)  {   pushMathValue(L,v); }
    inline  void    pushReal3(lua_State* L,const real3& v)  {   pushMathValue(L,v); }
    inline  void    pushReal4(lua_State* L,const real4& v)  {   pushMathValue(L,v); }
    inline  void    pushQuatr(lua_State* L,const quatr& q)  {   pushMathValue(L,q); }
    inline  void    pushMat3(lua_State* L,const mat3r& m)   {   pushMathValue(L,m); }
    inline  void    pushMat4(lua_State* L,const mat4r& m)   {   pushMathValue(L,m); }

    /// 注册数学类型元表与 fe.vec2/vec3/vec4/quat/mat3/mat4 等构造函数
    /// 要求:调用时 fe 表位于栈顶
    void    FELuaRegisterMath(lua_State* L);
