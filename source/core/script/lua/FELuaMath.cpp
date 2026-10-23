/// FELuaMath 实现:vec2/vec3/vec4/quat/mat3/mat4 的 userdata 绑定
/// 构造:fe.vec3(1,2,3) | fe.vec3({1,2,3}) | fe.vec3() | fe.vec3(s) 等
/// 四元数约定:fe.quat(x,y,z,w) 与 table {x=,y=,z=,w=} 分量顺序一致
///           (注意 glm qua 构造函数是 (w,x,y,z),此处用成员赋值避免混淆)
/// 注册内容:6 个类型元表 + fe 表上的构造函数(要求 fe 表在栈顶)

#include    "FELuaVM.hpp"
#include    "FELuaHelper.hpp"

#include    <glm/gtc/quaternion.hpp>
#include    <glm/gtc/matrix_transform.hpp>

namespace   FE
{
    namespace
    {
        // ==================== vec2/vec3/vec4 通用实现 ====================

        template<class V> struct VecLabel;
        template<> struct VecLabel<real2>   {   static  constexpr const char*   value   =   "vec2"; };
        template<> struct VecLabel<real3>   {   static  constexpr const char*   value   =   "vec3"; };
        template<> struct VecLabel<real4>   {   static  constexpr const char*   value   =   "vec4"; };
        template<> struct VecLabel<quatr>   {   static  constexpr const char*   value   =   "quat"; };

        /// 编译期维度(glm 此版本 V::length() 非 constexpr,不能用于 if constexpr)
        template<class V> struct VecDim;
        template<> struct VecDim<real2>     {   static  constexpr int   value   =   2;  };
        template<> struct VecDim<real3>     {   static  constexpr int   value   =   3;  };
        template<> struct VecDim<real4>     {   static  constexpr int   value   =   4;  };

        template<class V>
        static  int     vecCtor(lua_State* L)
        {
            const   int dim     =   (int)V::length();
            int     argc        =   lua_gettop(L);      // 参数个数(压 userdata 前取,参数位于 1..argc)
            auto*   p   =   pushMath<V>(L);
            if (argc == 0)
                return  1;
            if (argc == 1)
            {
                V   t;
                if (testMath(L,1,t))
                {
                    p->v    =   t;
                    return  1;
                }
                if (lua_istable(L,1))
                {
                    KFValue kv;
                    if (toKFValue(L,1,kv))
                    {
                        if (auto*   t2  =   std::get_if<real2>(&kv))
                        {
                            if constexpr (VecDim<V>::value == 2)
                            {
                                p->v    =   V(t2->x,t2->y);
                                return  1;
                            }
                        }
                        else if (auto*  t3  =   std::get_if<real3>(&kv))
                        {
                            if constexpr (VecDim<V>::value == 3)
                            {
                                p->v    =   *t3;
                                return  1;
                            }
                            else if constexpr (VecDim<V>::value == 4)
                            {
                                p->v    =   V(t3->x,t3->y,t3->z,0);
                                return  1;
                            }
                        }
                        else if (auto*  t4  =   std::get_if<real4>(&kv))
                        {
                            if constexpr (VecDim<V>::value == 4)
                            {
                                p->v    =   *t4;
                                return  1;
                            }
                        }
                    }
                    return  luaL_error(L,"fe.%s: bad table",VecLabel<V>::value);
                }
                if (lua_isnumber(L,1))
                {
                    real    s   =   (real)lua_tonumber(L,1);
                    p->v    =   V(s);
                    return  1;
                }
                return  luaL_error(L,"fe.%s: expect numbers/table/%s",VecLabel<V>::value,VecLabel<V>::value);
            }
            if (argc == dim)
            {
                for (int i = 0; i < dim; ++i)
                    p->v[i] =   (real)luaL_checknumber(L,1 + i);
                return  1;
            }
            return  luaL_error(L,"fe.%s: expect %d components",VecLabel<V>::value,dim);
        }

        template<class V>
        static  int     vecAdd(lua_State* L)
        {
            V   a,b;
            if (!testMath(L,1,a) || !testMath(L,2,b))
                return  luaL_error(L,"%s + %s: type mismatch",VecLabel<V>::value,VecLabel<V>::value);
            pushMathValue(L,a + b);
            return  1;
        }

        template<class V>
        static  int     vecSub(lua_State* L)
        {
            V   a,b;
            if (!testMath(L,1,a) || !testMath(L,2,b))
                return  luaL_error(L,"%s - %s: type mismatch",VecLabel<V>::value,VecLabel<V>::value);
            pushMathValue(L,a - b);
            return  1;
        }

        template<class V>
        static  int     vecMul(lua_State* L)
        {
            V   a,b;
            if (testMath(L,1,a) && testMath(L,2,b))
                pushMathValue(L,a * b);
            else if (testMath(L,1,a) && lua_isnumber(L,2))
                pushMathValue(L,a * (real)lua_tonumber(L,2));
            else if (lua_isnumber(L,1) && testMath(L,2,b))
                pushMathValue(L,(real)lua_tonumber(L,1) * b);
            else
                return  luaL_error(L,"%s *: expect vec/number",VecLabel<V>::value);
            return  1;
        }

        template<class V>
        static  int     vecDiv(lua_State* L)
        {
            V   a,b;
            if (testMath(L,1,a) && testMath(L,2,b))
                pushMathValue(L,a / b);
            else if (testMath(L,1,a) && lua_isnumber(L,2))
                pushMathValue(L,a / (real)lua_tonumber(L,2));
            else
                return  luaL_error(L,"%s /: expect vec/number",VecLabel<V>::value);
            return  1;
        }

        template<class V>
        static  int     vecUnm(lua_State* L)
        {
            pushMathValue(L,-checkMath<V>(L,1)->v);
            return  1;
        }

        template<class V>
        static  int     vecEq(lua_State* L)
        {
            V   a,b;
            if (testMath(L,1,a) && testMath(L,2,b))
                lua_pushboolean(L,a == b ? 1 : 0);
            else
                lua_pushboolean(L,0);
            return  1;
        }

        template<class V>
        static  int     vecLen(lua_State* L)
        {
            lua_pushinteger(L,V::length());
            return  1;
        }

        template<class V>
        static  int     vecToString(lua_State* L)
        {
            auto*   p   =   checkMath<V>(L,1);
            const   int dim =   (int)V::length();
            if (dim == 2)
                lua_pushfstring(L,"%s(%g, %g)",VecLabel<V>::value,(double)p->v[0],(double)p->v[1]);
            else if (dim == 3)
                lua_pushfstring(L,"%s(%g, %g, %g)",VecLabel<V>::value,(double)p->v[0],(double)p->v[1],(double)p->v[2]);
            else
                lua_pushfstring(L,"%s(%g, %g, %g, %g)",VecLabel<V>::value,(double)p->v[0],(double)p->v[1],(double)p->v[2],(double)p->v[3]);
            return  1;
        }

        template<class V>
        static  int     vecIndex(lua_State* L)
        {
            auto*   p   =   checkMath<V>(L,1);
            int     kt  =   lua_type(L,2);
            if (kt == LUA_TNUMBER)
            {
                lua_Integer i   =   lua_tointeger(L,2);
                if (i >= 1 && i <= (lua_Integer)V::length())
                {
                    lua_pushnumber(L,(double)p->v[i - 1]);
                    return  1;
                }
                lua_pushnil(L);
                return  1;
            }
            if (kt == LUA_TSTRING)
            {
                const char* k   =   lua_tostring(L,2);
                int     c   =   mathComponentIndex(k);
                if (c >= 0 && c < (int)V::length())
                {
                    lua_pushnumber(L,(double)p->v[c]);
                    return  1;
                }
                lua_getmetatable(L,1);
                lua_getfield(L,-1,"__methods");
                lua_pushvalue(L,2);
                lua_rawget(L,-2);
                return  1;
            }
            lua_pushnil(L);
            return  1;
        }

        template<class V>
        static  int     vecNewIndex(lua_State* L)
        {
            auto*   p   =   checkMath<V>(L,1);
            real    val =   (real)luaL_checknumber(L,3);
            if (lua_type(L,2) == LUA_TNUMBER)
            {
                lua_Integer i   =   lua_tointeger(L,2);
                if (i >= 1 && i <= (lua_Integer)V::length())
                    p->v[i - 1] =   val;
                return  0;
            }
            if (lua_type(L,2) == LUA_TSTRING)
            {
                int c   =   mathComponentIndex(lua_tostring(L,2));
                if (c >= 0 && c < (int)V::length())
                    p->v[c] =   val;
                return  0;
            }
            return  luaL_error(L,"%s: bad index",VecLabel<V>::value);
        }

        template<class V>
        static  int     vecLength(lua_State* L)
        {
            lua_pushnumber(L,(double)glm::length(checkMath<V>(L,1)->v));
            return  1;
        }

        template<class V>
        static  int     vecDot(lua_State* L)
        {
            V   a,b;
            if (!testMath(L,1,a) || !testMath(L,2,b))
                return  luaL_error(L,"%s:dot expect %s",VecLabel<V>::value,VecLabel<V>::value);
            lua_pushnumber(L,(double)glm::dot(a,b));
            return  1;
        }

        template<class V>
        static  int     vecDistance(lua_State* L)
        {
            V   a,b;
            if (!testMath(L,1,a) || !testMath(L,2,b))
                return  luaL_error(L,"%s:distance expect %s",VecLabel<V>::value,VecLabel<V>::value);
            lua_pushnumber(L,(double)glm::distance(a,b));
            return  1;
        }

        template<class V>
        static  int     vecNormalize(lua_State* L)
        {
            pushMathValue(L,glm::normalize(checkMath<V>(L,1)->v));
            return  1;
        }

        template<class V>
        static  int     vecLerp(lua_State* L)
        {
            V   a,b;
            if (!testMath(L,1,a) || !testMath(L,2,b))
                return  luaL_error(L,"%s:lerp expect %s",VecLabel<V>::value,VecLabel<V>::value);
            real    t   =   (real)luaL_checknumber(L,3);
            pushMathValue(L,glm::mix(a,b,t));
            return  1;
        }

        static  int     vec3Cross(lua_State* L)
        {
            real3   a,b;
            if (!testMath(L,1,a) || !testMath(L,2,b))
                return  luaL_error(L,"vec3:cross expect vec3");
            pushMathValue(L,glm::cross(a,b));
            return  1;
        }

        template<class V>
        static  void    registerVecType(lua_State* L,const luaL_Reg* extra = nullptr)
        {
            if (luaL_newmetatable(L,metaName<V>()))
            {
                static const luaL_Reg  methods[]   =
                {
                    {   "length",       vecLength<V>    },
                    {   "dot",          vecDot<V>       },
                    {   "distance",     vecDistance<V>  },
                    {   "normalize",    vecNormalize<V> },
                    {   "lerp",         vecLerp<V>      },
                    {   nullptr,        nullptr         },
                };
                lua_newtable(L);
                luaL_setfuncs(L,methods,0);
                if (extra)
                    luaL_setfuncs(L,extra,0);
                lua_setfield(L,-2,"__methods");
                lua_pushcfunction(L,vecIndex<V>);       lua_setfield(L,-2,"__index");
                lua_pushcfunction(L,vecNewIndex<V>);    lua_setfield(L,-2,"__newindex");
                lua_pushcfunction(L,vecAdd<V>);         lua_setfield(L,-2,"__add");
                lua_pushcfunction(L,vecSub<V>);         lua_setfield(L,-2,"__sub");
                lua_pushcfunction(L,vecMul<V>);         lua_setfield(L,-2,"__mul");
                lua_pushcfunction(L,vecDiv<V>);         lua_setfield(L,-2,"__div");
                lua_pushcfunction(L,vecUnm<V>);         lua_setfield(L,-2,"__unm");
                lua_pushcfunction(L,vecEq<V>);          lua_setfield(L,-2,"__eq");
                lua_pushcfunction(L,vecLen<V>);         lua_setfield(L,-2,"__len");
                lua_pushcfunction(L,vecToString<V>);    lua_setfield(L,-2,"__tostring");
                lua_pushcfunction(L,gcMath<V>);         lua_setfield(L,-2,"__gc");
            }
            lua_pop(L,1);
        }

        // ==================== quat ====================

        static  int     quatCtor(lua_State* L)
        {
            int     argc    =   lua_gettop(L);           // 参数个数(压 userdata 前取)
            auto*   p   =   pushMath<quatr>(L);
            if (argc == 0)
                return  1;                      // 单位四元数
            if (argc == 1)
            {
                quatr   t;
                if (testMath(L,1,t))
                {
                    p->v    =   t;
                    return  1;
                }
                if (toQuatr(L,1,p->v))
                    return  1;
                return  luaL_error(L,"fe.quat: expect table {x,y,z,w}");
            }
            if (argc == 4)
            {
                p->v.x  =   (real)luaL_checknumber(L,1);
                p->v.y  =   (real)luaL_checknumber(L,2);
                p->v.z  =   (real)luaL_checknumber(L,3);
                p->v.w  =   (real)luaL_checknumber(L,4);
                return  1;
            }
            return  luaL_error(L,"fe.quat: expect (x,y,z,w) or table");
        }

        /// fe.quatAngleAxis(angleRad, axis):绕轴旋转构造四元数
        static  int     quatAngleAxis(lua_State* L)
        {
            real    angle   =   (real)luaL_checknumber(L,1);
            real3   axis;
            if (!toReal3(L,2,axis))
                return  luaL_error(L,"fe.quatAngleAxis: expect (angle, vec3)");
            pushMathValue(L,glm::angleAxis(angle,glm::normalize(axis)));
            return  1;
        }

        static  int     quatMul(lua_State* L)
        {
            quatr   a;
            real3   v3;
            if (testMath(L,1,a))
            {
                quatr   b;
                if (testMath(L,2,b))
                    pushMathValue(L,a * b);
                else if (testMath(L,2,v3))
                    pushMathValue(L,a * v3);
                else if (lua_isnumber(L,2))
                    pushMathValue(L,a * (real)lua_tonumber(L,2));
                else
                    return  luaL_error(L,"quat *: expect quat/vec3/number");
                return  1;
            }
            if (lua_isnumber(L,1) && testMath(L,2,a))
                pushMathValue(L,(real)lua_tonumber(L,1) * a);
            else
                return  luaL_error(L,"quat *: expect quat/number");
            return  1;
        }

        static  int     quatToString(lua_State* L)
        {
            auto*   p   =   checkMath<quatr>(L,1);
            lua_pushfstring(L,"quat(%g, %g, %g, %g)",(double)p->v.x,(double)p->v.y,(double)p->v.z,(double)p->v.w);
            return  1;
        }

        static  int     quatConj(lua_State* L)
        {
            pushMathValue(L,glm::conjugate(checkMath<quatr>(L,1)->v));
            return  1;
        }

        static  int     quatNormalize(lua_State* L)
        {
            pushMathValue(L,glm::normalize(checkMath<quatr>(L,1)->v));
            return  1;
        }

        static  int     quatDot(lua_State* L)
        {
            quatr   a,b;
            if (!testMath(L,1,a) || !testMath(L,2,b))
                return  luaL_error(L,"quat:dot expect quat");
            lua_pushnumber(L,(double)glm::dot(a,b));
            return  1;
        }

        static  int     quatSlerp(lua_State* L)
        {
            quatr   a,b;
            if (!testMath(L,1,a) || !testMath(L,2,b))
                return  luaL_error(L,"quat:slerp expect quat");
            real    t   =   (real)luaL_checknumber(L,3);
            pushMathValue(L,glm::slerp(a,b,t));
            return  1;
        }

        static  int     quatAngle(lua_State* L)
        {
            lua_pushnumber(L,(double)glm::angle(checkMath<quatr>(L,1)->v));
            return  1;
        }

        static  int     quatAxis(lua_State* L)
        {
            pushMathValue(L,glm::axis(checkMath<quatr>(L,1)->v));
            return  1;
        }

        static  int     quatToMat3(lua_State* L)
        {
            pushMathValue(L,glm::mat3_cast(checkMath<quatr>(L,1)->v));
            return  1;
        }

        static  int     quatToMat4(lua_State* L)
        {
            pushMathValue(L,glm::mat4_cast(checkMath<quatr>(L,1)->v));
            return  1;
        }

        static  void    registerQuatType(lua_State* L)
        {
            if (luaL_newmetatable(L,metaName<quatr>()))
            {
                static const luaL_Reg  methods[]   =
                {
                    {   "conj",         quatConj        },
                    {   "normalize",    quatNormalize   },
                    {   "dot",          quatDot         },
                    {   "slerp",        quatSlerp       },
                    {   "angle",        quatAngle       },
                    {   "axis",         quatAxis        },
                    {   "toMat3",       quatToMat3      },
                    {   "toMat4",       quatToMat4      },
                    {   nullptr,        nullptr         },
                };
                lua_newtable(L);
                luaL_setfuncs(L,methods,0);
                lua_setfield(L,-2,"__methods");
                lua_pushcfunction(L,vecIndex<quatr>)    ;lua_setfield(L,-2,"__index");
                lua_pushcfunction(L,vecNewIndex<quatr>) ;lua_setfield(L,-2,"__newindex");
                lua_pushcfunction(L,vecAdd<quatr>)      ;lua_setfield(L,-2,"__add");
                lua_pushcfunction(L,vecSub<quatr>)      ;lua_setfield(L,-2,"__sub");
                lua_pushcfunction(L,quatMul)            ;lua_setfield(L,-2,"__mul");
                lua_pushcfunction(L,vecUnm<quatr>)      ;lua_setfield(L,-2,"__unm");
                lua_pushcfunction(L,vecEq<quatr>)       ;lua_setfield(L,-2,"__eq");
                lua_pushcfunction(L,vecLen<quatr>)      ;lua_setfield(L,-2,"__len");
                lua_pushcfunction(L,quatToString)       ;lua_setfield(L,-2,"__tostring");
                lua_pushcfunction(L,gcMath<quatr>)      ;lua_setfield(L,-2,"__gc");
            }
            lua_pop(L,1);
        }

        // ==================== mat3/mat4 通用实现 ====================

        template<class M> struct MatLabel;
        template<> struct MatLabel<mat3r>   {   static  constexpr const char*   value   =   "mat3"; };
        template<> struct MatLabel<mat4r>   {   static  constexpr const char*   value   =   "mat4"; };

        template<class M> struct MatCol;
        template<> struct MatCol<mat3r>     {   using  type    =   real3;  };
        template<> struct MatCol<mat4r>     {   using  type    =   real4;  };

        template<class M>
        static  int     matCtor(lua_State* L)
        {
            const   int dim =   (int)M::length();
            int     argc    =   lua_gettop(L);          // 参数个数(压 userdata 前取)
            auto*   p   =   pushMath<M>(L);
            *p  =   LuaMath<M>();           // 单位阵由默认构造给出(glm mat(1.0))
            if (argc == 0)
                return  1;
            if (argc == 1)
            {
                M   t;
                if (testMath(L,1,t))
                {
                    p->v    =   t;
                    return  1;
                }
                quatr   q;
                if (testMath(L,1,q))
                {
                    if (dim == 3)
                        p->v    =   glm::mat3_cast(q);
                    else
                        p->v    =   glm::mat4_cast(q);
                    return  1;
                }
                return  luaL_error(L,"fe.%s: expect quat/%s",MatLabel<M>::value,MatLabel<M>::value);
            }
            return  luaL_error(L,"fe.%s: too many arguments",MatLabel<M>::value);
        }

        /// fe.mat4Perspective(fovyRad, aspect, near, far)
        static  int     mat4Perspective(lua_State* L)
        {
            real    fovy    =   (real)luaL_checknumber(L,1);
            real    aspect  =   (real)luaL_checknumber(L,2);
            real    znear   =   (real)luaL_checknumber(L,3);
            real    zfar    =   (real)luaL_checknumber(L,4);
            pushMathValue(L,glm::perspective(fovy,aspect,znear,zfar));
            return  1;
        }

        /// fe.mat4LookAt(eye, center, up)
        static  int     mat4LookAt(lua_State* L)
        {
            real3   eye,center,up;
            if (!toReal3(L,1,eye) || !toReal3(L,2,center) || !toReal3(L,3,up))
                return  luaL_error(L,"fe.mat4LookAt: expect (eye, center, up)");
            pushMathValue(L,glm::lookAt(eye,center,up));
            return  1;
        }

        template<class M>
        static  int     matMul(lua_State* L)
        {
            M   a;
            typename MatCol<M>::type    col;
            if (testMath(L,1,a))
            {
                M   b;
                if (testMath(L,2,b))
                    pushMathValue(L,a * b);
                else if (testMath(L,2,col))
                    pushMathValue(L,a * col);
                else if (lua_isnumber(L,2))
                    pushMathValue(L,a * (real)lua_tonumber(L,2));
                else
                    return  luaL_error(L,"%s *: expect %s/vec/number",MatLabel<M>::value,MatLabel<M>::value);
                return  1;
            }
            if (lua_isnumber(L,1) && testMath(L,2,a))
                pushMathValue(L,(real)lua_tonumber(L,1) * a);
            else
                return  luaL_error(L,"%s *: expect %s/number",MatLabel<M>::value,MatLabel<M>::value);
            return  1;
        }

        template<class M>
        static  int     matEq(lua_State* L)
        {
            M   a,b;
            if (testMath(L,1,a) && testMath(L,2,b))
                lua_pushboolean(L,a == b ? 1 : 0);
            else
                lua_pushboolean(L,0);
            return  1;
        }

        template<class M>
        static  int     matToString(lua_State* L)
        {
            auto*   p   =   checkMath<M>(L,1);
            UNUSED(p);
            lua_pushfstring(L,"%s(%gx%g)",MatLabel<M>::value,(double)M::length(),(double)M::length());
            return  1;
        }

        template<class M>
        static  int     matIndex(lua_State* L)
        {
            auto*   p   =   checkMath<M>(L,1);
            if (lua_type(L,2) == LUA_TNUMBER)
            {
                lua_Integer i   =   lua_tointeger(L,2);
                if (i >= 1 && i <= (lua_Integer)M::length())
                {
                    pushMathValue(L,typename MatCol<M>::type(p->v[i - 1]));
                    return  1;
                }
                lua_pushnil(L);
                return  1;
            }
            if (lua_type(L,2) == LUA_TSTRING)
            {
                lua_getmetatable(L,1);
                lua_getfield(L,-1,"__methods");
                lua_pushvalue(L,2);
                lua_rawget(L,-2);
                return  1;
            }
            lua_pushnil(L);
            return  1;
        }

        template<class M>
        static  int     matNewIndex(lua_State* L)
        {
            auto*   p   =   checkMath<M>(L,1);
            typename MatCol<M>::type    col;
            if (lua_type(L,2) == LUA_TNUMBER && testMath(L,3,col))
            {
                lua_Integer i   =   lua_tointeger(L,2);
                if (i >= 1 && i <= (lua_Integer)M::length())
                    p->v[i - 1] =   col;
                return  0;
            }
            return  luaL_error(L,"%s: expect [%d] = vec",MatLabel<M>::value,(int)M::length());
        }

        template<class M>
        static  int     matTranspose(lua_State* L)
        {
            pushMathValue(L,glm::transpose(checkMath<M>(L,1)->v));
            return  1;
        }

        template<class M>
        static  int     matInverse(lua_State* L)
        {
            pushMathValue(L,glm::inverse(checkMath<M>(L,1)->v));
            return  1;
        }

        template<class M>
        static  int     matDeterminant(lua_State* L)
        {
            lua_pushnumber(L,(double)glm::determinant(checkMath<M>(L,1)->v));
            return  1;
        }

        /// mat4:transformPoint(v3):变换点(平移生效)
        static  int     mat4TransformPoint(lua_State* L)
        {
            auto*   p   =   checkMath<mat4r>(L,1);
            real3   v;
            if (!toReal3(L,2,v))
                return  luaL_error(L,"mat4:transformPoint expect vec3");
            real4   t   =   p->v * real4(v,1.0);
            pushMathValue(L,real3(t.x,t.y,t.z));
            return  1;
        }

        /// mat4:transformVector(v3):变换方向(平移不生效)
        static  int     mat4TransformVector(lua_State* L)
        {
            auto*   p   =   checkMath<mat4r>(L,1);
            real3   v;
            if (!toReal3(L,2,v))
                return  luaL_error(L,"mat4:transformVector expect vec3");
            real4   t   =   p->v * real4(v,0.0);
            pushMathValue(L,real3(t.x,t.y,t.z));
            return  1;
        }

        template<class M>
        static  void    registerMatType(lua_State* L)
        {
            if (luaL_newmetatable(L,metaName<M>()))
            {
                static const luaL_Reg  methods[]   =
                {
                    {   "transpose",        matTranspose<M>     },
                    {   "inverse",          matInverse<M>       },
                    {   "determinant",      matDeterminant<M>   },
                    {   nullptr,            nullptr             },
                };
                lua_newtable(L);
                luaL_setfuncs(L,methods,0);
                if (M::length() == 4)
                {
                    static const luaL_Reg  mat4Methods[]   =
                    {
                        {   "transformPoint",   mat4TransformPoint  },
                        {   "transformVector",  mat4TransformVector },
                        {   nullptr,            nullptr             },
                    };
                    luaL_setfuncs(L,mat4Methods,0);
                }
                lua_setfield(L,-2,"__methods");
                lua_pushcfunction(L,matIndex<M>)        ;lua_setfield(L,-2,"__index");
                lua_pushcfunction(L,matNewIndex<M>)     ;lua_setfield(L,-2,"__newindex");
                lua_pushcfunction(L,matMul<M>)          ;lua_setfield(L,-2,"__mul");
                lua_pushcfunction(L,matEq<M>)           ;lua_setfield(L,-2,"__eq");
                lua_pushcfunction(L,matToString<M>)     ;lua_setfield(L,-2,"__tostring");
                lua_pushcfunction(L,gcMath<M>)          ;lua_setfield(L,-2,"__gc");
            }
            lua_pop(L,1);
        }
    }

    /// 注册数学类型与 fe.* 构造函数(要求 fe 表在栈顶)
    void    FELuaRegisterMath(lua_State* L)
    {
        static  const luaL_Reg  extraCross[]   =
        {
            {   "cross",    vec3Cross   },
            {   nullptr,    nullptr     },
        };
        registerVecType<real2>(L);
        registerVecType<real3>(L,extraCross);
        registerVecType<real4>(L);
        registerQuatType(L);
        registerMatType<mat3r>(L);
        registerMatType<mat4r>(L);

        static  const luaL_Reg  mathFuncs[]   =
        {
            {   "vec2",             vecCtor<real2>      },
            {   "vec3",             vecCtor<real3>      },
            {   "vec4",             vecCtor<real4>      },
            {   "quat",             quatCtor            },
            {   "quatAngleAxis",    quatAngleAxis       },
            {   "mat3",             matCtor<mat3r>      },
            {   "mat4",             matCtor<mat4r>      },
            {   "mat4Perspective",  mat4Perspective     },
            {   "mat4LookAt",       mat4LookAt          },
            {   nullptr,            nullptr             },
        };
        luaL_setfuncs(L,mathFuncs,0);
    }
}
