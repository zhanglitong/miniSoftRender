#pragma once
/// FEQuickJSMath
/// 数学类型绑定(值语义普通对象 + 共享原型,与引擎 real 精度一致):
///   fe.vec2/vec3/vec4/quat/mat3/mat4/quatAngleAxis/mat4Perspective/mat4LookAt
/// 分量通过 x/y/z/w 属性读写;矩阵列通过 c0..c3(向量对象)读写
/// 运算以方法提供(add/sub/mul/div/neg/equals),实现于 FEQuickJSMath.cpp
/// 本文件位于 namespace FE 内部被 FEQuickJSHelper.hpp include,不能再包一层 namespace

/// <summary>
/// 新建一个挂指定原型的普通对象
/// </summary>
inline  JSValue newMath(JSContext* ctx,JSValueConst proto)
{
    JSValue obj =   JS_NewObject(ctx);
    JS_SetPrototype(ctx,obj,proto);
    return  obj;
}

inline  JSValue pushReal2(JSContext* ctx,JSEnv* env,const real2& v)
{
    JSValue obj =   newMath(ctx,env->protoVec2);
    JS_SetPropertyStr(ctx,obj,"x",JS_NewFloat64(ctx,v.x));
    JS_SetPropertyStr(ctx,obj,"y",JS_NewFloat64(ctx,v.y));
    return  obj;
}

inline  JSValue pushReal3(JSContext* ctx,JSEnv* env,const real3& v)
{
    JSValue obj =   newMath(ctx,env->protoVec3);
    JS_SetPropertyStr(ctx,obj,"x",JS_NewFloat64(ctx,v.x));
    JS_SetPropertyStr(ctx,obj,"y",JS_NewFloat64(ctx,v.y));
    JS_SetPropertyStr(ctx,obj,"z",JS_NewFloat64(ctx,v.z));
    return  obj;
}

inline  JSValue pushReal4(JSContext* ctx,JSEnv* env,const real4& v)
{
    JSValue obj =   newMath(ctx,env->protoVec4);
    JS_SetPropertyStr(ctx,obj,"x",JS_NewFloat64(ctx,v.x));
    JS_SetPropertyStr(ctx,obj,"y",JS_NewFloat64(ctx,v.y));
    JS_SetPropertyStr(ctx,obj,"z",JS_NewFloat64(ctx,v.z));
    JS_SetPropertyStr(ctx,obj,"w",JS_NewFloat64(ctx,v.w));
    return  obj;
}

inline  JSValue pushQuatr(JSContext* ctx,JSEnv* env,const quatr& q)
{
    JSValue obj =   newMath(ctx,env->protoQuat);
    JS_SetPropertyStr(ctx,obj,"x",JS_NewFloat64(ctx,q.x));
    JS_SetPropertyStr(ctx,obj,"y",JS_NewFloat64(ctx,q.y));
    JS_SetPropertyStr(ctx,obj,"z",JS_NewFloat64(ctx,q.z));
    JS_SetPropertyStr(ctx,obj,"w",JS_NewFloat64(ctx,q.w));
    return  obj;
}

/// <summary>
/// 矩阵以列向量对象 c0..c3 形式存放(与 glm 列主序一致)
/// </summary>
inline  JSValue pushMat3(JSContext* ctx,JSEnv* env,const mat3r& m)
{
    JSValue obj =   newMath(ctx,env->protoMat3);
    JS_SetPropertyStr(ctx,obj,"c0",pushReal3(ctx,env,m[0]));
    JS_SetPropertyStr(ctx,obj,"c1",pushReal3(ctx,env,m[1]));
    JS_SetPropertyStr(ctx,obj,"c2",pushReal3(ctx,env,m[2]));
    return  obj;
}

inline  JSValue pushMat4(JSContext* ctx,JSEnv* env,const mat4r& m)
{
    JSValue obj =   newMath(ctx,env->protoMat4);
    JS_SetPropertyStr(ctx,obj,"c0",pushReal4(ctx,env,m[0]));
    JS_SetPropertyStr(ctx,obj,"c1",pushReal4(ctx,env,m[1]));
    JS_SetPropertyStr(ctx,obj,"c2",pushReal4(ctx,env,m[2]));
    JS_SetPropertyStr(ctx,obj,"c3",pushReal4(ctx,env,m[3]));
    return  obj;
}

/// <summary>
/// 注册数学原型与 fe 表上的构造函数(要求传入 fe 表)
/// </summary>
void    FEQJSRegisterMath(JSContext* ctx,JSEnv* env,JSValue fe);
