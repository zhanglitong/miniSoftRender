/// FEQuickJSMath 实现:vec2/vec3/vec4/quat/mat3/mat4 绑定
/// 构造:fe.vec3(1,2,3) | fe.vec3([1,2,3]) | fe.vec3({x,y,z}) | fe.vec3() | fe.vec3(s)
/// 四元数约定:fe.quat(x,y,z,w) 与 {x,y,z,w} 分量顺序一致(成员赋值,规避 glm 构造歧义)
/// 运算以方法提供(add/sub/mul/div/neg/equals),对应 Lua 版的运算符绑定
/// 注册内容:6 个数学原型 + fe 表上的构造函数

#include    "FEQuickJS.hpp"
#include    "FEQuickJSHelper.hpp"

#include    <glm/gtc/quaternion.hpp>
#include    <glm/gtc/matrix_transform.hpp>

namespace   FE
{
    namespace
    {
        /// <summary>
        /// 读参数数值(严格 number,不做字符串转换)
        /// </summary>
        static  bool    argNum(JSContext* ctx,JSValueConst v,real& out)
        {
            double  d;
            if (!JS_IsNumber(v) || JS_ToFloat64(ctx,&d,v) != 0)
                return  false;
            out =   (real)d;
            return  true;
        }

        // ==================== 向量 traits(按维度分发) ====================

        template<int DIM>   struct  VecOf;
        template<> struct VecOf<2>
        {
            using   type    =   real2;
            static  constexpr const char*   name    =   "vec2";
            static  bool    read(JSContext* ctx,JSValueConst v,type& out)
            {
                return  toReal2(ctx,v,out);
            }
            static  bool    readStrict(JSContext* ctx,JSEnv* env,JSValueConst v,type& out)
            {
                return  isProtoOf(ctx,v,env->protoVec2) && toReal2(ctx,v,out);
            }
            static  JSValue push(JSContext* ctx,JSEnv* env,const type& v)
            {
                return  pushReal2(ctx,env,v);
            }
        };
        template<> struct VecOf<3>
        {
            using   type    =   real3;
            static  constexpr const char*   name    =   "vec3";
            static  bool    read(JSContext* ctx,JSValueConst v,type& out)
            {
                return  toReal3(ctx,v,out);
            }
            static  bool    readStrict(JSContext* ctx,JSEnv* env,JSValueConst v,type& out)
            {
                return  isProtoOf(ctx,v,env->protoVec3) && toReal3(ctx,v,out);
            }
            static  JSValue push(JSContext* ctx,JSEnv* env,const type& v)
            {
                return  pushReal3(ctx,env,v);
            }
        };
        template<> struct VecOf<4>
        {
            using   type    =   real4;
            static  constexpr const char*   name    =   "vec4";
            static  bool    read(JSContext* ctx,JSValueConst v,type& out)
            {
                return  toReal4(ctx,v,out);
            }
            static  bool    readStrict(JSContext* ctx,JSEnv* env,JSValueConst v,type& out)
            {
                return  isProtoOf(ctx,v,env->protoVec4) && toReal4(ctx,v,out);
            }
            static  JSValue push(JSContext* ctx,JSEnv* env,const type& v)
            {
                return  pushReal4(ctx,env,v);
            }
        };

        // ==================== 向量构造与方法 ====================

        template<int DIM>
        static  JSValue vecCtor(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(this_val);
            using   Vec    =   typename VecOf<DIM>::type;
            JSEnv*  env    =   getEnv(ctx);
            if (argc == 0)
                return  VecOf<DIM>::push(ctx,env,Vec(0));
            if (argc == 1)
            {
                real    s;
                if (argNum(ctx,argv[0],s))
                    return  VecOf<DIM>::push(ctx,env,Vec(s));
                KFValue kv;
                if (!toKFValue(ctx,env,argv[0],kv))
                    return  JS_ThrowTypeError(ctx,"fe.%s: expect numbers/object",VecOf<DIM>::name);
                if (auto*   t2  =   std::get_if<real2>(&kv))
                {
                    if constexpr (DIM == 2)
                        return  VecOf<DIM>::push(ctx,env,*t2);
                }
                else if (auto*  t3  =   std::get_if<real3>(&kv))
                {
                    if constexpr (DIM == 3)
                        return  VecOf<DIM>::push(ctx,env,*t3);
                    else if constexpr (DIM == 4)
                        return  VecOf<DIM>::push(ctx,env,Vec(t3->x,t3->y,t3->z,0));
                }
                else if (auto*  t4  =   std::get_if<real4>(&kv))
                {
                    if constexpr (DIM == 4)
                        return  VecOf<DIM>::push(ctx,env,*t4);
                }
                return  JS_ThrowTypeError(ctx,"fe.%s: bad value shape",VecOf<DIM>::name);
            }
            if (argc == DIM)
            {
                Vec     t(0);
                for (int i = 0; i < DIM; ++i)
                {
                    if (!argNum(ctx,argv[i],t[i]))
                        return  JS_ThrowTypeError(ctx,"fe.%s: expect %d number components",VecOf<DIM>::name,DIM);
                }
                return  VecOf<DIM>::push(ctx,env,t);
            }
            return  JS_ThrowTypeError(ctx,"fe.%s: expect %d components",VecOf<DIM>::name,DIM);
        }

        /// this 与参数均要求严格同类型(对应 Lua 运算符的 testMath 类型检查)
        template<int DIM>
        static  bool    vecOperands(JSContext* ctx,JSEnv* env,JSValueConst this_val,JSValueConst arg,
                                    typename VecOf<DIM>::type& a,typename VecOf<DIM>::type& b)
        {
            return  VecOf<DIM>::readStrict(ctx,env,this_val,a) && VecOf<DIM>::readStrict(ctx,env,arg,b);
        }

        template<int DIM>
        static  JSValue vecAdd(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(argc);
            JSEnv*  env =   getEnv(ctx);
            typename VecOf<DIM>::type   a,b;
            if (!vecOperands<DIM>(ctx,env,this_val,argv[0],a,b))
                return  JS_ThrowTypeError(ctx,"%s.add: expect %s",VecOf<DIM>::name,VecOf<DIM>::name);
            return  VecOf<DIM>::push(ctx,env,a + b);
        }

        template<int DIM>
        static  JSValue vecSub(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(argc);
            JSEnv*  env =   getEnv(ctx);
            typename VecOf<DIM>::type   a,b;
            if (!vecOperands<DIM>(ctx,env,this_val,argv[0],a,b))
                return  JS_ThrowTypeError(ctx,"%s.sub: expect %s",VecOf<DIM>::name,VecOf<DIM>::name);
            return  VecOf<DIM>::push(ctx,env,a - b);
        }

        template<int DIM>
        static  JSValue vecMul(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(argc);
            using   Vec    =   typename VecOf<DIM>::type;
            JSEnv*  env    =   getEnv(ctx);
            Vec     a;
            if (!VecOf<DIM>::readStrict(ctx,env,this_val,a))
                return  JS_ThrowTypeError(ctx,"%s.mul: bad this",VecOf<DIM>::name);
            Vec     b;
            if (VecOf<DIM>::readStrict(ctx,env,argv[0],b))
                return  VecOf<DIM>::push(ctx,env,a * b);
            real    s;
            if (argNum(ctx,argv[0],s))
                return  VecOf<DIM>::push(ctx,env,a * s);
            return  JS_ThrowTypeError(ctx,"%s.mul: expect %s/number",VecOf<DIM>::name,VecOf<DIM>::name);
        }

        template<int DIM>
        static  JSValue vecDiv(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(argc);
            using   Vec    =   typename VecOf<DIM>::type;
            JSEnv*  env    =   getEnv(ctx);
            Vec     a;
            if (!VecOf<DIM>::readStrict(ctx,env,this_val,a))
                return  JS_ThrowTypeError(ctx,"%s.div: bad this",VecOf<DIM>::name);
            Vec     b;
            if (VecOf<DIM>::readStrict(ctx,env,argv[0],b))
                return  VecOf<DIM>::push(ctx,env,a / b);
            real    s;
            if (argNum(ctx,argv[0],s))
                return  VecOf<DIM>::push(ctx,env,a / s);
            return  JS_ThrowTypeError(ctx,"%s.div: expect %s/number",VecOf<DIM>::name,VecOf<DIM>::name);
        }

        template<int DIM>
        static  JSValue vecNeg(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(argc);
            UNUSED(argv);
            JSEnv*  env =   getEnv(ctx);
            typename VecOf<DIM>::type   a;
            if (!VecOf<DIM>::readStrict(ctx,env,this_val,a))
                return  JS_ThrowTypeError(ctx,"%s.neg: bad this",VecOf<DIM>::name);
            return  VecOf<DIM>::push(ctx,env,-a);
        }

        template<int DIM>
        static  JSValue vecEquals(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(argc);
            JSEnv*  env =   getEnv(ctx);
            typename VecOf<DIM>::type   a,b;
            if (!vecOperands<DIM>(ctx,env,this_val,argv[0],a,b))
                return  JS_FALSE;
            return  JS_NewBool(ctx,a == b);
        }

        template<int DIM>
        static  JSValue vecClone(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(argc);
            UNUSED(argv);
            JSEnv*  env =   getEnv(ctx);
            typename VecOf<DIM>::type   a;
            if (!VecOf<DIM>::readStrict(ctx,env,this_val,a))
                return  JS_ThrowTypeError(ctx,"%s.clone: bad this",VecOf<DIM>::name);
            return  VecOf<DIM>::push(ctx,env,a);
        }

        template<int DIM>
        static  JSValue vecAt(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(argc);
            JSEnv*  env =   getEnv(ctx);
            typename VecOf<DIM>::type   a;
            if (!VecOf<DIM>::readStrict(ctx,env,this_val,a))
                return  JS_ThrowTypeError(ctx,"%s.at: bad this",VecOf<DIM>::name);
            int64_t idx;
            if (!JS_IsNumber(argv[0]) || JS_ToInt64(ctx,&idx,argv[0]) != 0 || idx < 0 || idx >= DIM)
                return  JS_UNDEFINED;
            return  JS_NewFloat64(ctx,(double)a[(int)idx]);
        }

        template<int DIM>
        static  JSValue vecLength(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(argc);
            UNUSED(argv);
            JSEnv*  env =   getEnv(ctx);
            typename VecOf<DIM>::type   a;
            if (!VecOf<DIM>::readStrict(ctx,env,this_val,a))
                return  JS_ThrowTypeError(ctx,"%s.length: bad this",VecOf<DIM>::name);
            return  JS_NewFloat64(ctx,(double)glm::length(a));
        }

        template<int DIM>
        static  JSValue vecDot(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(argc);
            JSEnv*  env =   getEnv(ctx);
            typename VecOf<DIM>::type   a,b;
            if (!vecOperands<DIM>(ctx,env,this_val,argv[0],a,b))
                return  JS_ThrowTypeError(ctx,"%s.dot expect %s",VecOf<DIM>::name,VecOf<DIM>::name);
            return  JS_NewFloat64(ctx,(double)glm::dot(a,b));
        }

        template<int DIM>
        static  JSValue vecDistance(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(argc);
            JSEnv*  env =   getEnv(ctx);
            typename VecOf<DIM>::type   a,b;
            if (!vecOperands<DIM>(ctx,env,this_val,argv[0],a,b))
                return  JS_ThrowTypeError(ctx,"%s.distance expect %s",VecOf<DIM>::name,VecOf<DIM>::name);
            return  JS_NewFloat64(ctx,(double)glm::distance(a,b));
        }

        template<int DIM>
        static  JSValue vecNormalize(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(argc);
            UNUSED(argv);
            JSEnv*  env =   getEnv(ctx);
            typename VecOf<DIM>::type   a;
            if (!VecOf<DIM>::readStrict(ctx,env,this_val,a))
                return  JS_ThrowTypeError(ctx,"%s.normalize: bad this",VecOf<DIM>::name);
            return  VecOf<DIM>::push(ctx,env,glm::normalize(a));
        }

        template<int DIM>
        static  JSValue vecLerp(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(argc);
            JSEnv*  env =   getEnv(ctx);
            typename VecOf<DIM>::type   a,b;
            if (!vecOperands<DIM>(ctx,env,this_val,argv[0],a,b))
                return  JS_ThrowTypeError(ctx,"%s.lerp expect %s",VecOf<DIM>::name,VecOf<DIM>::name);
            real    t;
            if (!argNum(ctx,argv[1],t))
                return  JS_ThrowTypeError(ctx,"%s.lerp: expect number t",VecOf<DIM>::name);
            return  VecOf<DIM>::push(ctx,env,glm::mix(a,b,t));
        }

        static  JSValue vec3Cross(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(argc);
            JSEnv*  env =   getEnv(ctx);
            real3   a,b;
            if (!VecOf<3>::readStrict(ctx,env,this_val,a) || !VecOf<3>::readStrict(ctx,env,argv[0],b))
                return  JS_ThrowTypeError(ctx,"vec3.cross expect vec3");
            return  pushReal3(ctx,env,glm::cross(a,b));
        }

        template<int DIM>
        static  JSValue vecToString(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(argc);
            UNUSED(argv);
            JSEnv*  env =   getEnv(ctx);
            typename VecOf<DIM>::type   a;
            if (!VecOf<DIM>::readStrict(ctx,env,this_val,a))
                return  JS_ThrowTypeError(ctx,"%s.toString: bad this",VecOf<DIM>::name);
            char    buf[128];
            if constexpr (DIM == 2)
                snprintf(buf,sizeof(buf),"%s(%g, %g)",VecOf<DIM>::name,(double)a[0],(double)a[1]);
            else if constexpr (DIM == 3)
                snprintf(buf,sizeof(buf),"%s(%g, %g, %g)",VecOf<DIM>::name,(double)a[0],(double)a[1],(double)a[2]);
            else
                snprintf(buf,sizeof(buf),"%s(%g, %g, %g, %g)",VecOf<DIM>::name,(double)a[0],(double)a[1],(double)a[2],(double)a[3]);
            return  JS_NewString(ctx,buf);
        }

        template<int DIM>
        static  void    setFn(JSContext* ctx,JSValue obj,const char* name,JSCFunction* fn,int length)
        {
            JS_SetPropertyStr(ctx,obj,name,JS_NewCFunction(ctx,fn,name,length));
        }

        template<int DIM>
        static  void    registerVecProto(JSContext* ctx,JSEnv* env,JSValue fe,const char* ctorName)
        {
            JSValue proto   =   JS_NewObject(ctx);
            setFn<DIM>(ctx,proto,"length",vecLength<DIM>,0);
            setFn<DIM>(ctx,proto,"dot",vecDot<DIM>,1);
            setFn<DIM>(ctx,proto,"distance",vecDistance<DIM>,1);
            setFn<DIM>(ctx,proto,"normalize",vecNormalize<DIM>,0);
            setFn<DIM>(ctx,proto,"lerp",vecLerp<DIM>,2);
            setFn<DIM>(ctx,proto,"add",vecAdd<DIM>,1);
            setFn<DIM>(ctx,proto,"sub",vecSub<DIM>,1);
            setFn<DIM>(ctx,proto,"mul",vecMul<DIM>,1);
            setFn<DIM>(ctx,proto,"div",vecDiv<DIM>,1);
            setFn<DIM>(ctx,proto,"neg",vecNeg<DIM>,0);
            setFn<DIM>(ctx,proto,"equals",vecEquals<DIM>,1);
            setFn<DIM>(ctx,proto,"clone",vecClone<DIM>,0);
            setFn<DIM>(ctx,proto,"at",vecAt<DIM>,1);
            setFn<DIM>(ctx,proto,"toString",vecToString<DIM>,0);
            if constexpr (DIM == 3)
                setFn<DIM>(ctx,proto,"cross",vec3Cross,1);
            switch (DIM)
            {
            case 2:
                env->protoVec2  =   proto;
                break;
            case 3:
                env->protoVec3  =   proto;
                break;
            case 4:
                env->protoVec4  =   proto;
                break;
            default:
                break;
            }
            JS_SetPropertyStr(ctx,fe,ctorName,JS_NewCFunction(ctx,vecCtor<DIM>,ctorName,0));
        }

        // ==================== quat ====================

        /// quat 逐分量运算:KFValue(variant,可从 quatr 隐式构造)的自由
        /// operator± 会与 glm qua 二元运算符产生重载决议歧义,显式分量实现规避
        static  quatr   quatAddV(const quatr& a,const quatr& b)
        {
            quatr   r;
            r.x =   a.x + b.x;
            r.y =   a.y + b.y;
            r.z =   a.z + b.z;
            r.w =   a.w + b.w;
            return  r;
        }

        static  quatr   quatSubV(const quatr& a,const quatr& b)
        {
            quatr   r;
            r.x =   a.x - b.x;
            r.y =   a.y - b.y;
            r.z =   a.z - b.z;
            r.w =   a.w - b.w;
            return  r;
        }

        /// 四元数乘法(w,x,y,z 分量公式,绕开 glm 二元 * 的重载歧义)
        static  quatr   quatMulV(const quatr& a,const quatr& b)
        {
            quatr   r;
            r.w =   a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z;
            r.x =   a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y;
            r.y =   a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x;
            r.z =   a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w;
            return  r;
        }

        /// 四元数旋转向量:v + 2*cross(q.xyz, cross(q.xyz, v) + w*q.xyz)
        static  real3   quatRotV(const quatr& q,const real3& v)
        {
            real3   qv(q.x,q.y,q.z);
            real3   t   =   real3(2.0 * (qv.y * v.z - qv.z * v.y),
                                  2.0 * (qv.z * v.x - qv.x * v.z),
                                  2.0 * (qv.x * v.y - qv.y * v.x));
            return  real3(v.x + q.w * t.x + (qv.y * t.z - qv.z * t.y),
                          v.y + q.w * t.y + (qv.z * t.x - qv.x * t.z),
                          v.z + q.w * t.z + (qv.x * t.y - qv.y * t.x));
        }

        struct  QuatOf
        {
            using   type    =   quatr;
            static  constexpr const char*   name    =   "quat";
            static  bool    read(JSContext* ctx,JSValueConst v,type& out)
            {
                return  toQuatr(ctx,v,out);
            }
            static  bool    readStrict(JSContext* ctx,JSEnv* env,JSValueConst v,type& out)
            {
                return  isProtoOf(ctx,v,env->protoQuat) && toQuatr(ctx,v,out);
            }
            static  JSValue push(JSContext* ctx,JSEnv* env,const type& v)
            {
                return  pushQuatr(ctx,env,v);
            }
        };

        static  JSValue quatCtor(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(this_val);
            JSEnv*  env =   getEnv(ctx);
            quatr   q(1,0,0,0);
            if (argc == 0)
                return  pushQuatr(ctx,env,q);
            if (argc == 1)
            {
                if (!toQuatr(ctx,argv[0],q))
                    return  JS_ThrowTypeError(ctx,"fe.quat: expect {x,y,z,w}");
                return  pushQuatr(ctx,env,q);
            }
            if (argc == 4)
            {
                real    c[4];
                for (int i = 0; i < 4; ++i)
                {
                    if (!argNum(ctx,argv[i],c[i]))
                        return  JS_ThrowTypeError(ctx,"fe.quat: expect (x,y,z,w) numbers");
                }
                /// 成员赋值,分量顺序 (x,y,z,w)
                q.x =   c[0];
                q.y =   c[1];
                q.z =   c[2];
                q.w =   c[3];
                return  pushQuatr(ctx,env,q);
            }
            return  JS_ThrowTypeError(ctx,"fe.quat: expect (x,y,z,w) or {x,y,z,w}");
        }

        /// fe.quatAngleAxis(angleRad, axis):绕轴旋转构造四元数(axis 自动归一化)
        static  JSValue quatAngleAxis(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(this_val);
            UNUSED(argc);
            JSEnv*  env =   getEnv(ctx);
            real    angle;
            real3   axis;
            if (!argNum(ctx,argv[0],angle) || !toReal3(ctx,argv[1],axis))
                return  JS_ThrowTypeError(ctx,"fe.quatAngleAxis: expect (angle, vec3)");
            return  pushQuatr(ctx,env,glm::angleAxis(angle,glm::normalize(axis)));
        }

        static  JSValue quatMul(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(argc);
            JSEnv*  env =   getEnv(ctx);
            quatr   a;
            if (!QuatOf::readStrict(ctx,env,this_val,a))
                return  JS_ThrowTypeError(ctx,"quat.mul: bad this");
            quatr   b;
            if (QuatOf::readStrict(ctx,env,argv[0],b))
                return  pushQuatr(ctx,env,quatMulV(a,b));
            real3   v3;
            if (isProtoOf(ctx,argv[0],env->protoVec3) && toReal3(ctx,argv[0],v3))
                return  pushReal3(ctx,env,quatRotV(a,v3));
            real    s;
            if (argNum(ctx,argv[0],s))
            {
                quatr   r;
                r.x =   a.x * s;
                r.y =   a.y * s;
                r.z =   a.z * s;
                r.w =   a.w * s;
                return  pushQuatr(ctx,env,r);
            }
            return  JS_ThrowTypeError(ctx,"quat.mul: expect quat/vec3/number");
        }

        static  JSValue quatAdd(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(argc);
            JSEnv*  env =   getEnv(ctx);
            quatr   a,b;
            if (!QuatOf::readStrict(ctx,env,this_val,a) || !QuatOf::readStrict(ctx,env,argv[0],b))
                return  JS_ThrowTypeError(ctx,"quat.add: expect quat");
            return  pushQuatr(ctx,env,quatAddV(a,b));
        }

        static  JSValue quatSub(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(argc);
            JSEnv*  env =   getEnv(ctx);
            quatr   a,b;
            if (!QuatOf::readStrict(ctx,env,this_val,a) || !QuatOf::readStrict(ctx,env,argv[0],b))
                return  JS_ThrowTypeError(ctx,"quat.sub: expect quat");
            return  pushQuatr(ctx,env,quatSubV(a,b));
        }

        static  JSValue quatNeg(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(argc);
            UNUSED(argv);
            JSEnv*  env =   getEnv(ctx);
            quatr   a;
            if (!QuatOf::readStrict(ctx,env,this_val,a))
                return  JS_ThrowTypeError(ctx,"quat.neg: bad this");
            quatr   r;
            r.x =   -a.x;
            r.y =   -a.y;
            r.z =   -a.z;
            r.w =   -a.w;
            return  pushQuatr(ctx,env,r);
        }

        static  JSValue quatEquals(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(argc);
            JSEnv*  env =   getEnv(ctx);
            quatr   a,b;
            if (!QuatOf::readStrict(ctx,env,this_val,a) || !QuatOf::readStrict(ctx,env,argv[0],b))
                return  JS_FALSE;
            return  JS_NewBool(ctx,a == b);
        }

        static  JSValue quatClone(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(argc);
            UNUSED(argv);
            JSEnv*  env =   getEnv(ctx);
            quatr   a;
            if (!QuatOf::readStrict(ctx,env,this_val,a))
                return  JS_ThrowTypeError(ctx,"quat.clone: bad this");
            return  pushQuatr(ctx,env,a);
        }

        static  JSValue quatAt(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(argc);
            JSEnv*  env =   getEnv(ctx);
            quatr   a;
            if (!QuatOf::readStrict(ctx,env,this_val,a))
                return  JS_ThrowTypeError(ctx,"quat.at: bad this");
            int64_t idx;
            if (!JS_IsNumber(argv[0]) || JS_ToInt64(ctx,&idx,argv[0]) != 0 || idx < 0 || idx >= 4)
                return  JS_UNDEFINED;
            return  JS_NewFloat64(ctx,(double)a[(int)idx]);
        }

        static  JSValue quatConj(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(argc);
            UNUSED(argv);
            JSEnv*  env =   getEnv(ctx);
            quatr   a;
            if (!QuatOf::readStrict(ctx,env,this_val,a))
                return  JS_ThrowTypeError(ctx,"quat.conj: bad this");
            return  pushQuatr(ctx,env,glm::conjugate(a));
        }

        static  JSValue quatNormalize(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(argc);
            UNUSED(argv);
            JSEnv*  env =   getEnv(ctx);
            quatr   a;
            if (!QuatOf::readStrict(ctx,env,this_val,a))
                return  JS_ThrowTypeError(ctx,"quat.normalize: bad this");
            return  pushQuatr(ctx,env,glm::normalize(a));
        }

        static  JSValue quatDot(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(argc);
            JSEnv*  env =   getEnv(ctx);
            quatr   a,b;
            if (!QuatOf::readStrict(ctx,env,this_val,a) || !QuatOf::readStrict(ctx,env,argv[0],b))
                return  JS_ThrowTypeError(ctx,"quat.dot expect quat");
            return  JS_NewFloat64(ctx,(double)glm::dot(a,b));
        }

        static  JSValue quatSlerp(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(argc);
            JSEnv*  env =   getEnv(ctx);
            quatr   a,b;
            if (!QuatOf::readStrict(ctx,env,this_val,a) || !QuatOf::readStrict(ctx,env,argv[0],b))
                return  JS_ThrowTypeError(ctx,"quat.slerp expect quat");
            real    t;
            if (!argNum(ctx,argv[1],t))
                return  JS_ThrowTypeError(ctx,"quat.slerp: expect number t");
            return  pushQuatr(ctx,env,glm::slerp(a,b,t));
        }

        static  JSValue quatAngle(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(argc);
            UNUSED(argv);
            JSEnv*  env =   getEnv(ctx);
            quatr   a;
            if (!QuatOf::readStrict(ctx,env,this_val,a))
                return  JS_ThrowTypeError(ctx,"quat.angle: bad this");
            return  JS_NewFloat64(ctx,(double)glm::angle(a));
        }

        static  JSValue quatAxis(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(argc);
            UNUSED(argv);
            JSEnv*  env =   getEnv(ctx);
            quatr   a;
            if (!QuatOf::readStrict(ctx,env,this_val,a))
                return  JS_ThrowTypeError(ctx,"quat.axis: bad this");
            return  pushReal3(ctx,env,glm::axis(a));
        }

        static  JSValue quatToMat3(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(argc);
            UNUSED(argv);
            JSEnv*  env =   getEnv(ctx);
            quatr   a;
            if (!QuatOf::readStrict(ctx,env,this_val,a))
                return  JS_ThrowTypeError(ctx,"quat.toMat3: bad this");
            return  pushMat3(ctx,env,glm::mat3_cast(a));
        }

        static  JSValue quatToMat4(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(argc);
            UNUSED(argv);
            JSEnv*  env =   getEnv(ctx);
            quatr   a;
            if (!QuatOf::readStrict(ctx,env,this_val,a))
                return  JS_ThrowTypeError(ctx,"quat.toMat4: bad this");
            return  pushMat4(ctx,env,glm::mat4_cast(a));
        }

        static  JSValue quatToString(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(argc);
            UNUSED(argv);
            JSEnv*  env =   getEnv(ctx);
            quatr   a;
            if (!QuatOf::readStrict(ctx,env,this_val,a))
                return  JS_ThrowTypeError(ctx,"quat.toString: bad this");
            char    buf[128];
            snprintf(buf,sizeof(buf),"quat(%g, %g, %g, %g)",(double)a.x,(double)a.y,(double)a.z,(double)a.w);
            return  JS_NewString(ctx,buf);
        }

        static  void    registerQuatProto(JSContext* ctx,JSEnv* env,JSValue fe)
        {
            JSValue proto   =   JS_NewObject(ctx);
            setFn<4>(ctx,proto,"conj",quatConj,0);
            setFn<4>(ctx,proto,"normalize",quatNormalize,0);
            setFn<4>(ctx,proto,"dot",quatDot,1);
            setFn<4>(ctx,proto,"slerp",quatSlerp,2);
            setFn<4>(ctx,proto,"angle",quatAngle,0);
            setFn<4>(ctx,proto,"axis",quatAxis,0);
            setFn<4>(ctx,proto,"toMat3",quatToMat3,0);
            setFn<4>(ctx,proto,"toMat4",quatToMat4,0);
            setFn<4>(ctx,proto,"add",quatAdd,1);
            setFn<4>(ctx,proto,"sub",quatSub,1);
            setFn<4>(ctx,proto,"mul",quatMul,1);
            setFn<4>(ctx,proto,"neg",quatNeg,0);
            setFn<4>(ctx,proto,"equals",quatEquals,1);
            setFn<4>(ctx,proto,"clone",quatClone,0);
            setFn<4>(ctx,proto,"at",quatAt,1);
            setFn<4>(ctx,proto,"toString",quatToString,0);
            env->protoQuat  =   proto;
            JS_SetPropertyStr(ctx,fe,"quat",JS_NewCFunction(ctx,quatCtor,"quat",0));
            JS_SetPropertyStr(ctx,fe,"quatAngleAxis",JS_NewCFunction(ctx,quatAngleAxis,"quatAngleAxis",2));
        }

        // ==================== mat3/mat4 ====================

        /// 读矩阵列属性 c0..cN-1(宽松,接受任意 {c0,c1,...} 对象)
        /// 先声明重载再定义 MatOf,供 MatOf::read 调用
        static  bool    readMat(JSContext* ctx,JSValueConst v,mat3r& out);
        static  bool    readMat(JSContext* ctx,JSValueConst v,mat4r& out);

        template<int DIM>   struct  MatOf;
        template<> struct MatOf<3>
        {
            using   type    =   mat3r;
            using   colType =   real3;
            static  constexpr const char*   name    =   "mat3";
            static  bool    read(JSContext* ctx,JSValueConst v,type& out)
            {
                return  readMat(ctx,v,out);
            }
            static  bool    readStrict(JSContext* ctx,JSEnv* env,JSValueConst v,type& out)
            {
                return  isProtoOf(ctx,v,env->protoMat3) && readMat(ctx,v,out);
            }
            static  bool    readCol(JSContext* ctx,JSValueConst v,colType& out)
            {
                return  toReal3(ctx,v,out);
            }
            static  bool    readColStrict(JSContext* ctx,JSEnv* env,JSValueConst v,colType& out)
            {
                return  isProtoOf(ctx,v,env->protoVec3) && toReal3(ctx,v,out);
            }
            static  JSValue push(JSContext* ctx,JSEnv* env,const type& v)
            {
                return  pushMat3(ctx,env,v);
            }
            static  JSValue colPush(JSContext* ctx,JSEnv* env,const colType& v)
            {
                return  pushReal3(ctx,env,v);
            }
        };
        template<> struct MatOf<4>
        {
            using   type    =   mat4r;
            using   colType =   real4;
            static  constexpr const char*   name    =   "mat4";
            static  bool    read(JSContext* ctx,JSValueConst v,type& out)
            {
                return  readMat(ctx,v,out);
            }
            static  bool    readStrict(JSContext* ctx,JSEnv* env,JSValueConst v,type& out)
            {
                return  isProtoOf(ctx,v,env->protoMat4) && readMat(ctx,v,out);
            }
            static  bool    readCol(JSContext* ctx,JSValueConst v,colType& out)
            {
                return  toReal4(ctx,v,out);
            }
            static  bool    readColStrict(JSContext* ctx,JSEnv* env,JSValueConst v,colType& out)
            {
                return  isProtoOf(ctx,v,env->protoVec4) && toReal4(ctx,v,out);
            }
            static  JSValue push(JSContext* ctx,JSEnv* env,const type& v)
            {
                return  pushMat4(ctx,env,v);
            }
            static  JSValue colPush(JSContext* ctx,JSEnv* env,const colType& v)
            {
                return  pushReal4(ctx,env,v);
            }
        };

        /// 读矩阵列属性 c0..cN-1(宽松,接受任意 {c0,c1,...} 对象)
        static  bool    readMat(JSContext* ctx,JSValueConst v,mat3r& out)
        {
            if (!JS_IsObject(v))
                return  false;
            for (int i = 0; i < 3; ++i)
            {
                char    key[8];
                snprintf(key,sizeof(key),"c%d",i);
                JSValue col =   JS_GetPropertyStr(ctx,v,key);
                real3   c;
                bool    ok  =   toReal3(ctx,col,c);
                JS_FreeValue(ctx,col);
                if (!ok)
                    return  false;
                out[i]  =   c;
            }
            return  true;
        }
        static  bool    readMat(JSContext* ctx,JSValueConst v,mat4r& out)
        {
            if (!JS_IsObject(v))
                return  false;
            for (int i = 0; i < 4; ++i)
            {
                char    key[8];
                snprintf(key,sizeof(key),"c%d",i);
                JSValue col =   JS_GetPropertyStr(ctx,v,key);
                real4   c;
                bool    ok  =   toReal4(ctx,col,c);
                JS_FreeValue(ctx,col);
                if (!ok)
                    return  false;
                out[i]  =   c;
            }
            return  true;
        }

        template<int DIM>
        static  JSValue matCtor(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(this_val);
            using   Mat    =   typename MatOf<DIM>::type;
            JSEnv*  env    =   getEnv(ctx);
            Mat     m(1.0);     /// 单位阵
            if (argc == 0)
                return  MatOf<DIM>::push(ctx,env,m);
            if (argc == 1)
            {
                Mat     t;
                if (MatOf<DIM>::read(ctx,argv[0],t))
                    return  MatOf<DIM>::push(ctx,env,t);
                quatr   q;
                if (toQuatr(ctx,argv[0],q))
                {
                    if constexpr (DIM == 3)
                        m   =   glm::mat3_cast(q);
                    else
                        m   =   glm::mat4_cast(q);
                    return  MatOf<DIM>::push(ctx,env,m);
                }
                return  JS_ThrowTypeError(ctx,"fe.%s: expect quat/%s",MatOf<DIM>::name,MatOf<DIM>::name);
            }
            return  JS_ThrowTypeError(ctx,"fe.%s: too many arguments",MatOf<DIM>::name);
        }

        template<int DIM>
        static  JSValue matMul(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(argc);
            using   Mat    =   typename MatOf<DIM>::type;
            using   Col    =   typename MatOf<DIM>::colType;
            JSEnv*  env    =   getEnv(ctx);
            Mat     a;
            if (!MatOf<DIM>::readStrict(ctx,env,this_val,a))
                return  JS_ThrowTypeError(ctx,"%s.mul: bad this",MatOf<DIM>::name);
            Mat     b;
            if (MatOf<DIM>::readStrict(ctx,env,argv[0],b))
                return  MatOf<DIM>::push(ctx,env,a * b);
            Col     c;
            if (MatOf<DIM>::readColStrict(ctx,env,argv[0],c))
                return  MatOf<DIM>::colPush(ctx,env,a * c);
            real    s;
            if (argNum(ctx,argv[0],s))
                return  MatOf<DIM>::push(ctx,env,a * s);
            return  JS_ThrowTypeError(ctx,"%s.mul: expect %s/vec/number",MatOf<DIM>::name,MatOf<DIM>::name);
        }

        template<int DIM>
        static  JSValue matEquals(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(argc);
            JSEnv*  env =   getEnv(ctx);
            typename MatOf<DIM>::type   a,b;
            if (!MatOf<DIM>::readStrict(ctx,env,this_val,a) || !MatOf<DIM>::readStrict(ctx,env,argv[0],b))
                return  JS_FALSE;
            return  JS_NewBool(ctx,a == b);
        }

        template<int DIM>
        static  JSValue matClone(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(argc);
            UNUSED(argv);
            JSEnv*  env =   getEnv(ctx);
            typename MatOf<DIM>::type   a;
            if (!MatOf<DIM>::readStrict(ctx,env,this_val,a))
                return  JS_ThrowTypeError(ctx,"%s.clone: bad this",MatOf<DIM>::name);
            return  MatOf<DIM>::push(ctx,env,a);
        }

        /// m.col(i):返回第 i 列(列向量对象拷贝)
        template<int DIM>
        static  JSValue matCol(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(argc);
            using   Mat    =   typename MatOf<DIM>::type;
            using   Col    =   typename MatOf<DIM>::colType;
            JSEnv*  env    =   getEnv(ctx);
            Mat     m;
            if (!MatOf<DIM>::readStrict(ctx,env,this_val,m))
                return  JS_ThrowTypeError(ctx,"%s.col: bad this",MatOf<DIM>::name);
            int64_t idx;
            if (!JS_IsNumber(argv[0]) || JS_ToInt64(ctx,&idx,argv[0]) != 0 || idx < 0 || idx >= DIM)
                return  JS_UNDEFINED;
            return  MatOf<DIM>::colPush(ctx,env,m[(int)idx]);
        }

        /// m.setCol(i, v):就地修改第 i 列,返回自身(支持链式)
        template<int DIM>
        static  JSValue matSetCol(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(argc);
            using   Col    =   typename MatOf<DIM>::colType;
            JSEnv*  env    =   getEnv(ctx);
            typename MatOf<DIM>::type   m;
            if (!MatOf<DIM>::readStrict(ctx,env,this_val,m))
                return  JS_ThrowTypeError(ctx,"%s.setCol: bad this",MatOf<DIM>::name);
            int64_t idx;
            if (!JS_IsNumber(argv[0]) || JS_ToInt64(ctx,&idx,argv[0]) != 0 || idx < 0 || idx >= DIM)
                return  JS_ThrowRangeError(ctx,"%s.setCol: column index out of range",MatOf<DIM>::name);
            Col     c;
            if (!MatOf<DIM>::readCol(ctx,argv[1],c))
                return  JS_ThrowTypeError(ctx,"%s.setCol: expect vec%d",MatOf<DIM>::name,DIM);
            char    key[8];
            snprintf(key,sizeof(key),"c%d",(int)idx);
            JS_SetPropertyStr(ctx,this_val,key,MatOf<DIM>::colPush(ctx,env,c));
            return  JS_DupValue(ctx,this_val);
        }

        template<int DIM>
        static  JSValue matTranspose(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(argc);
            UNUSED(argv);
            JSEnv*  env =   getEnv(ctx);
            typename MatOf<DIM>::type   m;
            if (!MatOf<DIM>::readStrict(ctx,env,this_val,m))
                return  JS_ThrowTypeError(ctx,"%s.transpose: bad this",MatOf<DIM>::name);
            return  MatOf<DIM>::push(ctx,env,glm::transpose(m));
        }

        template<int DIM>
        static  JSValue matInverse(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(argc);
            UNUSED(argv);
            JSEnv*  env =   getEnv(ctx);
            typename MatOf<DIM>::type   m;
            if (!MatOf<DIM>::readStrict(ctx,env,this_val,m))
                return  JS_ThrowTypeError(ctx,"%s.inverse: bad this",MatOf<DIM>::name);
            return  MatOf<DIM>::push(ctx,env,glm::inverse(m));
        }

        template<int DIM>
        static  JSValue matDeterminant(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(argc);
            UNUSED(argv);
            JSEnv*  env =   getEnv(ctx);
            typename MatOf<DIM>::type   m;
            if (!MatOf<DIM>::readStrict(ctx,env,this_val,m))
                return  JS_ThrowTypeError(ctx,"%s.determinant: bad this",MatOf<DIM>::name);
            return  JS_NewFloat64(ctx,(double)glm::determinant(m));
        }

        /// mat4:transformPoint(v3):变换点(平移生效)
        static  JSValue mat4TransformPoint(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(argc);
            JSEnv*  env =   getEnv(ctx);
            mat4r   m;
            if (!MatOf<4>::readStrict(ctx,env,this_val,m))
                return  JS_ThrowTypeError(ctx,"mat4.transformPoint: bad this");
            real3   v;
            if (!toReal3(ctx,argv[0],v))
                return  JS_ThrowTypeError(ctx,"mat4.transformPoint expect vec3");
            real4   t   =   m * real4(v,1.0);
            return  pushReal3(ctx,env,real3(t.x,t.y,t.z));
        }

        /// mat4:transformVector(v3):变换方向(平移不生效)
        static  JSValue mat4TransformVector(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(argc);
            JSEnv*  env =   getEnv(ctx);
            mat4r   m;
            if (!MatOf<4>::readStrict(ctx,env,this_val,m))
                return  JS_ThrowTypeError(ctx,"mat4.transformVector: bad this");
            real3   v;
            if (!toReal3(ctx,argv[0],v))
                return  JS_ThrowTypeError(ctx,"mat4.transformVector expect vec3");
            real4   t   =   m * real4(v,0.0);
            return  pushReal3(ctx,env,real3(t.x,t.y,t.z));
        }

        /// fe.mat4Perspective(fovyRad, aspect, near, far)
        static  JSValue mat4Perspective(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(this_val);
            UNUSED(argc);
            real    fovy,aspect,znear,zfar;
            if (!argNum(ctx,argv[0],fovy) || !argNum(ctx,argv[1],aspect) ||
                !argNum(ctx,argv[2],znear) || !argNum(ctx,argv[3],zfar))
                return  JS_ThrowTypeError(ctx,"fe.mat4Perspective: expect (fovy, aspect, near, far)");
            return  pushMat4(ctx,getEnv(ctx),glm::perspective(fovy,aspect,znear,zfar));
        }

        /// fe.mat4LookAt(eye, center, up)
        static  JSValue mat4LookAt(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(this_val);
            UNUSED(argc);
            real3   eye,center,up;
            if (!toReal3(ctx,argv[0],eye) || !toReal3(ctx,argv[1],center) || !toReal3(ctx,argv[2],up))
                return  JS_ThrowTypeError(ctx,"fe.mat4LookAt: expect (eye, center, up)");
            return  pushMat4(ctx,getEnv(ctx),glm::lookAt(eye,center,up));
        }

        template<int DIM>
        static  JSValue matToString(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            UNUSED(argc);
            UNUSED(argv);
            JSEnv*  env =   getEnv(ctx);
            typename MatOf<DIM>::type   m;
            if (!MatOf<DIM>::readStrict(ctx,env,this_val,m))
                return  JS_ThrowTypeError(ctx,"%s.toString: bad this",MatOf<DIM>::name);
            char    buf[32];
            snprintf(buf,sizeof(buf),"%s(%dx%d)",MatOf<DIM>::name,DIM,DIM);
            return  JS_NewString(ctx,buf);
        }

        template<int DIM>
        static  void    registerMatProto(JSContext* ctx,JSEnv* env,JSValue fe,const char* ctorName)
        {
            JSValue proto   =   JS_NewObject(ctx);
            setFn<DIM>(ctx,proto,"col",matCol<DIM>,1);
            setFn<DIM>(ctx,proto,"setCol",matSetCol<DIM>,2);
            setFn<DIM>(ctx,proto,"mul",matMul<DIM>,1);
            setFn<DIM>(ctx,proto,"transpose",matTranspose<DIM>,0);
            setFn<DIM>(ctx,proto,"inverse",matInverse<DIM>,0);
            setFn<DIM>(ctx,proto,"determinant",matDeterminant<DIM>,0);
            setFn<DIM>(ctx,proto,"equals",matEquals<DIM>,1);
            setFn<DIM>(ctx,proto,"clone",matClone<DIM>,0);
            setFn<DIM>(ctx,proto,"toString",matToString<DIM>,0);
            if constexpr (DIM == 4)
            {
                setFn<DIM>(ctx,proto,"transformPoint",mat4TransformPoint,1);
                setFn<DIM>(ctx,proto,"transformVector",mat4TransformVector,1);
            }
            switch (DIM)
            {
            case 3:
                env->protoMat3  =   proto;
                break;
            case 4:
                env->protoMat4  =   proto;
                break;
            default:
                break;
            }
            JS_SetPropertyStr(ctx,fe,ctorName,JS_NewCFunction(ctx,matCtor<DIM>,ctorName,0));
        }
    }

    /// <summary>
    /// 注册数学原型与 fe 表上的构造函数
    /// </summary>
    void    FEQJSRegisterMath(JSContext* ctx,JSEnv* env,JSValue fe)
    {
        registerVecProto<2>(ctx,env,fe,"vec2");
        registerVecProto<3>(ctx,env,fe,"vec3");
        registerVecProto<4>(ctx,env,fe,"vec4");
        registerQuatProto(ctx,env,fe);
        registerMatProto<3>(ctx,env,fe,"mat3");
        registerMatProto<4>(ctx,env,fe,"mat4");
        JS_SetPropertyStr(ctx,fe,"mat4Perspective",JS_NewCFunction(ctx,mat4Perspective,"mat4Perspective",4));
        JS_SetPropertyStr(ctx,fe,"mat4LookAt",JS_NewCFunction(ctx,mat4LookAt,"mat4LookAt",3));
    }
}
