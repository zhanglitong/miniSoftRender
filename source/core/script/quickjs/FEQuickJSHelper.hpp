#pragma once
/// FEQuickJSHelper
/// QuickJS 绑定内部辅助(仅被 FEQuickJS*.cpp 包含,不对外导出):
///   1. JSEnv               宿主环境(ctx/scene/类ID/数学原型),挂 runtime+context opaque
///   2. registerClass       JS 类注册(原型方法表 + finalizer 释放 SharedPtr)
///   3. pushObj/checkObj    引擎对象 <-> JS 对象(opaque 持 SharedPtr,finalizer 减引用)
///   4. math push           JS 数学对象(普通对象 + 共享原型,x/y/z/w 分量属性)
///   5. 数学转换            JS 对象/数组 <-> real2/real3/quatr/uint8x4/KFValue
/// 说明:quickjs-ng 此版本不提供运算符重载,数学运算以方法形式提供
///      (add/sub/mul/div/neg/equals),与 Lua 版运算符一一对应

#include    <quickjs.h>

#include    <stdio.h>
#include    <string.h>
#include    <string>
#include    <vector>

#include    "../../inc/FEContext.hpp"
#include    "../../inc/node/FENode.hpp"
#include    "../../inc/mesh/FEMesh.hpp"
#include    "../../inc/graphic/FEScene.h"
#include    "../../inc/animation/FEAnimationHelper.hpp"
#include    "../../inc/animation/FEAnimationSys.hpp"

namespace   FE
{
    class   FEScene;

    // ==================== 宿主环境 ====================

    /// <summary>
    /// 脚本宿主环境,生命周期由 FEQuickJS 管理
    /// scene 由宿主(setScene)持有,绑定代码不负责其生命周期
    /// protoXxx 为数学类型共享原型(值语义普通对象),析构时需 JS_FreeValue
    /// </summary>
    struct  JSEnv
    {
        FEContext*  ctx             =   nullptr;
        FEScene*    scene           =   nullptr;
        JSClassID   clsNode         =   0;
        JSClassID   clsMesh         =   0;
        JSClassID   clsAnimation    =   0;
        JSClassID   clsAnimClip     =   0;
        JSClassID   clsTrack        =   0;
        JSClassID   clsPlayback     =   0;
        JSClassID   clsAnimSys      =   0;
        JSValue     protoVec2       =   JS_UNDEFINED;
        JSValue     protoVec3       =   JS_UNDEFINED;
        JSValue     protoVec4       =   JS_UNDEFINED;
        JSValue     protoQuat       =   JS_UNDEFINED;
        JSValue     protoMat3       =   JS_UNDEFINED;
        JSValue     protoMat4       =   JS_UNDEFINED;
    };

    /// <summary>
    /// 从 context opaque 取回宿主环境
    /// </summary>
    inline  JSEnv*  getEnv(JSContext* ctx)
    {
        return  (JSEnv*)JS_GetContextOpaque(ctx);
    }

    /// 数学类型 push 实现(依赖上方 JSEnv,注册入口实现于 FEQuickJSMath.cpp)
    #include    "FEQuickJSMath.hpp"

    // ==================== 参数读取辅助 ====================

    /// <summary>
    /// 读数值属性:先按命名 key(x/y/z/w/r/g/b/a),缺失再按数字下标(0-based)
    /// 兼容 {x=1,y=2} 对象与 [1,2] 数组两种形态,对应 Lua 的分量/数组 table 约定
    /// </summary>
    inline  bool    readNum(JSContext* ctx,JSValueConst obj,const char* key,int pos,double& out)
    {
        JSValue v   =   JS_GetPropertyStr(ctx,obj,key);
        bool    ok  =   JS_IsNumber(v);
        if (ok)
            JS_ToFloat64(ctx,&out,v);
        JS_FreeValue(ctx,v);
        if (!ok)
        {
            v   =   JS_GetPropertyUint32(ctx,obj,(uint32_t)pos);
            ok  =   JS_IsNumber(v);
            if (ok)
                JS_ToFloat64(ctx,&out,v);
            JS_FreeValue(ctx,v);
        }
        return  ok;
    }

    /// <summary>
    /// 可选数值属性,缺失返回 false
    /// </summary>
    inline  bool    readNumOpt(JSContext* ctx,JSValueConst obj,const char* key,int pos,double& out)
    {
        return  JS_IsObject(obj) ? readNum(ctx,obj,key,pos,out) : false;
    }

    inline  bool    toReal2(JSContext* ctx,JSValueConst v,real2& out)
    {
        double  x,y;
        if (!JS_IsObject(v) || !readNum(ctx,v,"x",0,x) || !readNum(ctx,v,"y",1,y))
            return  false;
        out =   real2((real)x,(real)y);
        return  true;
    }

    inline  bool    toReal3(JSContext* ctx,JSValueConst v,real3& out)
    {
        double  x,y,z;
        if (!JS_IsObject(v) || !readNum(ctx,v,"x",0,x) || !readNum(ctx,v,"y",1,y) || !readNum(ctx,v,"z",2,z))
            return  false;
        out =   real3((real)x,(real)y,(real)z);
        return  true;
    }

    inline  bool    toReal4(JSContext* ctx,JSValueConst v,real4& out)
    {
        double  x,y,z,w;
        if (!JS_IsObject(v) || !readNum(ctx,v,"x",0,x) || !readNum(ctx,v,"y",1,y) ||
            !readNum(ctx,v,"z",2,z) || !readNum(ctx,v,"w",3,w))
            return  false;
        out =   real4((real)x,(real)y,(real)z,(real)w);
        return  true;
    }

    /// <summary>
    /// 读四元数:兼容 {x,y,z,w} 对象与 [1,2,3,4] 数组(含 protoQuat 原型的 quat 对象)
    /// 分量顺序统一为 (x,y,z,w),glm 构造函数的 (w,x,y,z) 差异已在绑定层处理
    /// </summary>
    inline  bool    toQuatr(JSContext* ctx,JSValueConst v,quatr& out)
    {
        real3   v3;
        if (!toReal3(ctx,v,v3))
            return  false;
        double  w;
        if (!readNum(ctx,v,"w",3,w))
            return  false;
        /// glm tquat 构造函数是 (w,x,y,z),须用成员赋值保证 (x,y,z,w) 分量顺序
        out.x   =   v3.x;
        out.y   =   v3.y;
        out.z   =   v3.z;
        out.w   =   (real)w;
        return  true;
    }

    /// <summary>
    /// 读颜色 {r,g,b,a} 或 [r,g,b,a],分量 0-255
    /// </summary>
    inline  bool    toUint8x4(JSContext* ctx,JSValueConst v,uint8x4& out)
    {
        double  r,g,b,a;
        if (!JS_IsObject(v) || !readNum(ctx,v,"r",0,r) || !readNum(ctx,v,"g",1,g) ||
            !readNum(ctx,v,"b",2,b) || !readNum(ctx,v,"a",3,a))
            return  false;
        out =   uint8x4((uint8)r,(uint8)g,(uint8)b,(uint8)a);
        return  true;
    }

    /// <summary>
    /// JS 值是否为指定原型对象(用于区分 quat 对象与普通 {x,y,z,w} table)
    /// </summary>
    inline  bool    isProtoOf(JSContext* ctx,JSValueConst v,JSValueConst proto)
    {
        if (!JS_IsObject(v))
            return  false;
        JSValue p   =   JS_GetPrototype(ctx,v);
        bool    ok  =   JS_VALUE_GET_PTR(p) == JS_VALUE_GET_PTR(proto);
        JS_FreeValue(ctx,p);
        return  ok;
    }

    /// <summary>
    /// JS 值 -> KFValue 启发式(对齐 Lua 版):
    ///   quat 对象 -> quatr;boolean -> uint8(0/1);number -> real
    ///   {r,g,b,a} -> uint8x4;{x,y,z[,w]} -> real3/real4;{x,y} -> real2
    /// </summary>
    inline  bool    toKFValue(JSContext* ctx,JSEnv* env,JSValueConst v,KFValue& out)
    {
        quatr   q;
        if (isProtoOf(ctx,v,env->protoQuat) && toQuatr(ctx,v,q))
        {
            out =   KFValue(q);
            return  true;
        }
        if (JS_IsBool(v))
        {
            out =   KFValue((uint8)(JS_ToBool(ctx,v) ? 1 : 0));
            return  true;
        }
        if (JS_IsNumber(v))
        {
            double  d;
            JS_ToFloat64(ctx,&d,v);
            out =   KFValue((real)d);
            return  true;
        }
        if (!JS_IsObject(v))
            return  false;

        uint8x4 rgba;
        if (toUint8x4(ctx,v,rgba))
        {
            out =   KFValue(rgba);
            return  true;
        }
        real3   t3;
        if (toReal3(ctx,v,t3))
        {
            double  w;
            if (readNumOpt(ctx,v,"w",3,w))
                out =   KFValue(real4(t3.x,t3.y,t3.z,(real)w));
            else
                out =   KFValue(t3);
            return  true;
        }
        real2   t2;
        if (toReal2(ctx,v,t2))
        {
            out =   KFValue(t2);
            return  true;
        }
        return  false;
    }

    // ==================== 引擎对象包装 ====================

    /// <summary>
    /// 引擎对象 <-> JS 类实例的类别映射(类ID与展示名)
    /// </summary>
    template<class T>   JSClassID&  classIdRef(JSEnv* env);
    template<> inline JSClassID&    classIdRef<FENode>(JSEnv* env)          {   return  env->clsNode;       }
    template<> inline JSClassID&    classIdRef<FEMesh>(JSEnv* env)          {   return  env->clsMesh;       }
    template<> inline JSClassID&    classIdRef<FEAnimation>(JSEnv* env)     {   return  env->clsAnimation;  }
    template<> inline JSClassID&    classIdRef<FEAnimClip>(JSEnv* env)      {   return  env->clsAnimClip;   }
    template<> inline JSClassID&    classIdRef<FEKeyFrameTrack>(JSEnv* env) {   return  env->clsTrack;      }
    template<> inline JSClassID&    classIdRef<FEPlayback>(JSEnv* env)      {   return  env->clsPlayback;  }
    template<> inline JSClassID&    classIdRef<FEAnimationSys>(JSEnv* env)  {   return  env->clsAnimSys;    }

    template<class T>   const char* jsTypeName();
    template<> inline const char*   jsTypeName<FENode>()            {   return  "FE.Node";          }
    template<> inline const char*   jsTypeName<FEMesh>()            {   return  "FE.Mesh";          }
    template<> inline const char*   jsTypeName<FEAnimation>()       {   return  "FE.Animation";     }
    template<> inline const char*   jsTypeName<FEAnimClip>()        {   return  "FE.AnimClip";      }
    template<> inline const char*   jsTypeName<FEKeyFrameTrack>()   {   return  "FE.KeyFrameTrack"; }
    template<> inline const char*   jsTypeName<FEPlayback>()        {   return  "FE.Playback";      }
    template<> inline const char*   jsTypeName<FEAnimationSys>()    {   return  "FE.AnimationSys";  }

    /// <summary>
    /// JS 类实例包装:持有一份 SharedPtr,保证对象在 JS 侧存活
    /// finalizer 析构时引用计数自动减一
    /// </summary>
    template<class T>
    struct  JSObjPtr
    {
        SharedPtr<T>    obj;
    };

    template<class T>
    static  void    finalizerObj(JSRuntime* rt,JSValueConst val)
    {
        (void)rt;
        auto*   p   =   (JSObjPtr<T>*)JS_GetOpaque(val,JS_GetClassID(val));
        delete  p;
    }

    /// <summary>
    /// JS 方法表项
    /// </summary>
    struct  JSFuncDef
    {
        const char*     name;
        JSCFunction*    fn;
        int             length;
    };

    /// <summary>
    /// 注册一个绑定类型:类ID + finalizer + 原型(方法表)
    /// </summary>
    template<class T>
    void    registerClass(JSContext* ctx,JSEnv* env,const JSFuncDef* methods)
    {
        JSRuntime*  rt  =   JS_GetRuntime(ctx);
        JSClassID&  id  =   classIdRef<T>(env);
        JS_NewClassID(rt,&id);
        JSClassDef  def =   {};
        def.class_name  =   jsTypeName<T>();
        def.finalizer   =   finalizerObj<T>;
        JS_NewClass(rt,id,&def);
        JSValue proto   =   JS_NewObject(ctx);
        for (int i = 0; methods[i].name != nullptr; ++i)
            JS_SetPropertyStr(ctx,proto,methods[i].name,
                              JS_NewCFunction(ctx,methods[i].fn,methods[i].name,methods[i].length));
        JS_SetClassProto(ctx,id,proto);
    }

    /// <summary>
    /// 引擎对象转 JS 类实例(nullptr 转 null,对应 Lua nil)
    /// 返回值引用归调用方所有
    /// </summary>
    template<class T>
    JSValue pushObj(JSContext* ctx,JSEnv* env,const SharedPtr<T>& obj)
    {
        if (obj == nullptr)
            return  JS_NULL;
        JSValue o   =   JS_NewObjectClass(ctx,classIdRef<T>(env));
        if (JS_IsException(o))
            return  o;
        auto*   p   =   new JSObjPtr<T>{obj};
        JS_SetOpaque(o,p);
        return  o;
    }

    /// <summary>
    /// 从 JS 值取引擎对象,类型不符挂起 TypeError 异常并返回 nullptr
    /// </summary>
    template<class T>
    SharedPtr<T>    checkObj(JSContext* ctx,JSEnv* env,JSValueConst v)
    {
        auto*   p   =   (JSObjPtr<T>*)JS_GetOpaque2(ctx,v,classIdRef<T>(env));
        if (p == nullptr)
            return  nullptr;
        return  p->obj;
    }

    /// <summary>
    /// 值是否为指定引擎类型(用于 addComponent 等参数探测)
    /// </summary>
    template<class T>
    bool    isObj(JSEnv* env,JSValueConst v)
    {
        return  JS_IsObject(v) && JS_GetClassID(v) == classIdRef<T>(env);
    }

    // ==================== 结构化值 push ====================

    inline  JSValue pushUint8x4(JSContext* ctx,const uint8x4& v)
    {
        JSValue obj =   JS_NewObject(ctx);
        JS_SetPropertyStr(ctx,obj,"r",JS_NewInt32(ctx,v.x));
        JS_SetPropertyStr(ctx,obj,"g",JS_NewInt32(ctx,v.y));
        JS_SetPropertyStr(ctx,obj,"b",JS_NewInt32(ctx,v.z));
        JS_SetPropertyStr(ctx,obj,"a",JS_NewInt32(ctx,v.w));
        return  obj;
    }

    inline  JSValue pushReal2Range(JSContext* ctx,const real2& v)
    {
        JSValue obj =   JS_NewObject(ctx);
        JS_SetPropertyStr(ctx,obj,"min",JS_NewFloat64(ctx,v.x));
        JS_SetPropertyStr(ctx,obj,"max",JS_NewFloat64(ctx,v.y));
        return  obj;
    }

    inline  JSValue pushAabb3dr(JSContext* ctx,JSEnv* env,const aabb3dr& box)
    {
        JSValue obj =   JS_NewObject(ctx);
        JS_SetPropertyStr(ctx,obj,"min",pushReal3(ctx,env,real3(box.getMinimum().x,box.getMinimum().y,box.getMinimum().z)));
        JS_SetPropertyStr(ctx,obj,"max",pushReal3(ctx,env,real3(box.getMaximum().x,box.getMaximum().y,box.getMaximum().z)));
        return  obj;
    }
}
