/// FEQuickJSAnim
/// 动画系统 QuickJS 绑定:脚本可创建 clip/track/关键帧,绑定节点并控制播放
///
/// fe.anim 模块函数:
///   createAnimation()                          -> Animation(空动画,无 clip)
///   createNodeAnimation([bits])                -> Animation(含默认轨道,见 FEAnimationHelper)
///   createClip()                               -> AnimClip
///   createTrack(propIndex, valueType)          -> KeyFrameTrack
///        valueType: "real" "real2" "real3" "real4" "quat" "quatf"
///                   "float" "float2" "float3" "float4"
///                   "color" "alpha" "bool"
///                   "int" "int2" "int3" "int4" "uint" "uint2" "uint3" "uint4"
///   action([name])                             -> Playback(AnimationSys::getOrCreate)
///   addKeyFrameFromNode(anim, time, node[, bits])   -> bool
///   updateKeyFrameFromNode(anim, time, node[, bits])-> bool
///   removeKeyFrameFromNode(anim, time)         -> bool
///
/// 常量: NP_*/PROP_*(属性位/索引)、IT_BEZIER/IT_LINEAR/IT_CONSTANT、
///       PT_ONCE/PT_LOOP/PT_PINGPONG、TM_DEFAULT/TM_INVERT
///
/// 与 Lua 版差异(JS 惯例):
///   索引统一 0-based(insertKeyFrame/getTrack 等,Lua 版为 1-based)
///   removeKeyFrame/updateKeyFrame 的位置参数:整数按索引,小数按时间

#include    "FEQuickJS.hpp"
#include    "FEQuickJSHelper.hpp"

namespace   FE
{
    namespace
    {
        /// <summary>
        /// 位置参数二义消除:整数(含整数值的 number)按索引,非整数按时间
        /// </summary>
        static  bool    isIntegerValue(JSContext* ctx,JSValueConst v,int64_t& out)
        {
            if (!JS_IsNumber(v))
                return  false;
            double  d;
            JS_ToFloat64(ctx,&d,v);
            int64_t i;
            if (JS_ToInt64(ctx,&i,v) != 0)
                return  false;
            if ((double)i != d)
                return  false;
            out =   i;
            return  true;
        }

        /// <summary>
        /// 严格 number 参数读取
        /// </summary>
        static  bool    argNum(JSContext* ctx,JSValueConst v,real& out)
        {
            double  d;
            if (!JS_IsNumber(v) || JS_ToFloat64(ctx,&d,v) != 0)
                return  false;
            out =   (real)d;
            return  true;
        }

        // ==================== Animation ====================

        static  JSValue Anim_name(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            (void)argv;
            auto    anim    =   checkObj<FEAnimation>(ctx,getEnv(ctx),this_val);
            if (anim == nullptr)
                return  JS_EXCEPTION;
            return  JS_NewString(ctx,anim->name().c_str());
        }

        static  JSValue Anim_setName(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            auto    anim    =   checkObj<FEAnimation>(ctx,getEnv(ctx),this_val);
            if (anim == nullptr)
                return  JS_EXCEPTION;
            if (!JS_IsString(argv[0]))
                return  JS_ThrowTypeError(ctx,"setName: expect string");
            const char* s   =   JS_ToCString(ctx,argv[0]);
            anim->setName(s != nullptr ? s : "");
            JS_FreeCString(ctx,s);
            return  JS_UNDEFINED;
        }

        static  JSValue Anim_offset(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            (void)argv;
            auto    anim    =   checkObj<FEAnimation>(ctx,getEnv(ctx),this_val);
            if (anim == nullptr)
                return  JS_EXCEPTION;
            return  JS_NewFloat64(ctx,(double)anim->offset());
        }

        static  JSValue Anim_setOffset(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            auto    anim    =   checkObj<FEAnimation>(ctx,getEnv(ctx),this_val);
            if (anim == nullptr)
                return  JS_EXCEPTION;
            real    v;
            if (!argNum(ctx,argv[0],v))
                return  JS_ThrowTypeError(ctx,"setOffset: expect number");
            anim->setOffset(v);
            return  JS_UNDEFINED;
        }

        static  JSValue Anim_clip(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            (void)argv;
            auto    anim    =   checkObj<FEAnimation>(ctx,getEnv(ctx),this_val);
            if (anim == nullptr)
                return  JS_EXCEPTION;
            return  pushObj<FEAnimClip>(ctx,getEnv(ctx),anim->clip());
        }

        static  JSValue Anim_setClip(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            JSEnv*  env     =   getEnv(ctx);
            auto    anim    =   checkObj<FEAnimation>(ctx,env,this_val);
            if (anim == nullptr)
                return  JS_EXCEPTION;
            auto    clip    =   checkObj<FEAnimClip>(ctx,env,argv[0]);
            if (clip == nullptr)
                return  JS_EXCEPTION;
            anim->setClip(clip);
            return  JS_UNDEFINED;
        }

        static  JSValue Anim_range(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            (void)argv;
            auto    anim    =   checkObj<FEAnimation>(ctx,getEnv(ctx),this_val);
            if (anim == nullptr)
                return  JS_EXCEPTION;
            return  pushReal2Range(ctx,anim->range());
        }

        static  JSValue Anim_isValid(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            (void)argv;
            auto    anim    =   checkObj<FEAnimation>(ctx,getEnv(ctx),this_val);
            if (anim == nullptr)
                return  JS_EXCEPTION;
            return  JS_NewBool(ctx,anim->isValid());
        }

        static  JSValue Anim_isChanged(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            (void)argv;
            auto    anim    =   checkObj<FEAnimation>(ctx,getEnv(ctx),this_val);
            if (anim == nullptr)
                return  JS_EXCEPTION;
            return  JS_NewBool(ctx,anim->isChanged());
        }

        static  JSValue Anim_clearChanged(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            (void)argv;
            auto    anim    =   checkObj<FEAnimation>(ctx,getEnv(ctx),this_val);
            if (anim == nullptr)
                return  JS_EXCEPTION;
            anim->clearChanged();
            return  JS_UNDEFINED;
        }

        static  const JSFuncDef AnimMethods[]   =
        {
            {   "name",         Anim_name,          0   },
            {   "setName",      Anim_setName,       1   },
            {   "offset",       Anim_offset,        0   },
            {   "setOffset",    Anim_setOffset,     1   },
            {   "clip",         Anim_clip,          0   },
            {   "setClip",      Anim_setClip,       1   },
            {   "range",        Anim_range,         0   },
            {   "isValid",      Anim_isValid,       0   },
            {   "isChanged",    Anim_isChanged,     0   },
            {   "clearChanged", Anim_clearChanged,  0   },
            {   nullptr,        nullptr,            0   },
        };

        // ==================== AnimClip ====================

        static  JSValue Clip_addKeyFrame(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            JSEnv*  env     =   getEnv(ctx);
            auto    clip    =   checkObj<FEAnimClip>(ctx,env,this_val);
            if (clip == nullptr)
                return  JS_EXCEPTION;
            real    time;
            KFValue val;
            if (!argNum(ctx,argv[0],time))
                return  JS_ThrowTypeError(ctx,"addKeyFrame: expect number time");
            if (!toKFValue(ctx,env,argv[1],val))
                return  JS_ThrowTypeError(ctx,"addKeyFrame: unsupported value type(number/object expected)");
            return  JS_NewBool(ctx,clip->addKeyFrame(time,val));
        }

        static  JSValue Clip_insertKeyFrame(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            JSEnv*  env     =   getEnv(ctx);
            auto    clip    =   checkObj<FEAnimClip>(ctx,env,this_val);
            if (clip == nullptr)
                return  JS_EXCEPTION;
            /// JS 索引从 0 开始,引擎从 0 开始
            int64_t idx;
            real    time;
            KFValue    val;
            if (!isIntegerValue(ctx,argv[0],idx) || idx < 0)
                return  JS_ThrowTypeError(ctx,"insertKeyFrame: expect integer index (0-based)");
            if (!argNum(ctx,argv[1],time))
                return  JS_ThrowTypeError(ctx,"insertKeyFrame: expect number time");
            if (!toKFValue(ctx,env,argv[2],val))
                return  JS_ThrowTypeError(ctx,"insertKeyFrame: unsupported value type(number/object expected)");
            return  JS_NewBool(ctx,clip->insertKeyFrame((size_t)idx,time,val));
        }

        static  JSValue Clip_removeKeyFrame(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            auto    clip    =   checkObj<FEAnimClip>(ctx,getEnv(ctx),this_val);
            if (clip == nullptr)
                return  JS_EXCEPTION;
            bool    ok  =   false;
            int64_t idx;
            real    time;
            if (isIntegerValue(ctx,argv[0],idx))
                ok  =   clip->removeKeyFrame((size_t)idx);
            else if (argNum(ctx,argv[0],time))
                ok  =   clip->removeKeyFrame(time);
            else
                return  JS_ThrowTypeError(ctx,"removeKeyFrame: expect integer index or number time");
            return  JS_NewBool(ctx,ok);
        }

        static  JSValue Clip_updateKeyFrame(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            JSEnv*  env     =   getEnv(ctx);
            auto    clip    =   checkObj<FEAnimClip>(ctx,env,this_val);
            if (clip == nullptr)
                return  JS_EXCEPTION;
            KFValue    val;
            if (!toKFValue(ctx,env,argv[1],val))
                return  JS_ThrowTypeError(ctx,"updateKeyFrame: unsupported value type(number/object expected)");
            bool    ok  =   false;
            int64_t idx;
            real    time;
            if (isIntegerValue(ctx,argv[0],idx))
                ok  =   clip->updateKeyFrame((size_t)idx,val);
            else if (argNum(ctx,argv[0],time))
                ok  =   clip->updateKeyFrame(time,val);
            else
                return  JS_ThrowTypeError(ctx,"updateKeyFrame: expect integer index or number time");
            return  JS_NewBool(ctx,ok);
        }

        static  JSValue Clip_addTrack(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            JSEnv*  env     =   getEnv(ctx);
            auto    clip    =   checkObj<FEAnimClip>(ctx,env,this_val);
            if (clip == nullptr)
                return  JS_EXCEPTION;
            auto    track   =   checkObj<FEKeyFrameTrack>(ctx,env,argv[0]);
            if (track == nullptr)
                return  JS_EXCEPTION;
            return  JS_NewInt32(ctx,(int32_t)clip->addTrack(track));
        }

        static  JSValue Clip_trackCount(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            (void)argv;
            auto    clip    =   checkObj<FEAnimClip>(ctx,getEnv(ctx),this_val);
            if (clip == nullptr)
                return  JS_EXCEPTION;
            return  JS_NewInt32(ctx,(int32_t)clip->tracks().size());
        }

        static  JSValue Clip_getTrack(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            auto    clip    =   checkObj<FEAnimClip>(ctx,getEnv(ctx),this_val);
            if (clip == nullptr)
                return  JS_EXCEPTION;
            /// JS 索引从 0 开始
            int64_t idx;
            if (!isIntegerValue(ctx,argv[0],idx) || idx < 0 || (size_t)idx >= clip->tracks().size())
                return  JS_ThrowRangeError(ctx,"getTrack: index out of range");
            return  pushObj<FEKeyFrameTrack>(ctx,getEnv(ctx),clip->tracks()[(size_t)idx]);
        }

        static  JSValue Clip_range(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            (void)argv;
            auto    clip    =   checkObj<FEAnimClip>(ctx,getEnv(ctx),this_val);
            if (clip == nullptr)
                return  JS_EXCEPTION;
            return  pushReal2Range(ctx,clip->range());
        }

        static  JSValue Clip_simplifyTimeline(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            (void)argv;
            auto    clip    =   checkObj<FEAnimClip>(ctx,getEnv(ctx),this_val);
            if (clip == nullptr)
                return  JS_EXCEPTION;
            clip->simplifyTimeline();
            return  JS_UNDEFINED;
        }

        static  const JSFuncDef ClipMethods[]   =
        {
            {   "addKeyFrame",      Clip_addKeyFrame,       2   },
            {   "insertKeyFrame",   Clip_insertKeyFrame,    3   },
            {   "removeKeyFrame",   Clip_removeKeyFrame,    1   },
            {   "updateKeyFrame",   Clip_updateKeyFrame,    2   },
            {   "addTrack",         Clip_addTrack,          1   },
            {   "trackCount",       Clip_trackCount,        0   },
            {   "getTrack",         Clip_getTrack,          1   },
            {   "range",            Clip_range,             0   },
            {   "simplifyTimeline", Clip_simplifyTimeline,  0   },
            {   nullptr,            nullptr,                0   },
        };

        // ==================== KeyFrameTrack ====================

        static  JSValue Track_name(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            (void)argv;
            auto    track   =   checkObj<FEKeyFrameTrack>(ctx,getEnv(ctx),this_val);
            if (track == nullptr)
                return  JS_EXCEPTION;
            return  JS_NewString(ctx,track->name().c_str());
        }

        static  JSValue Track_setName(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            auto    track   =   checkObj<FEKeyFrameTrack>(ctx,getEnv(ctx),this_val);
            if (track == nullptr)
                return  JS_EXCEPTION;
            if (!JS_IsString(argv[0]))
                return  JS_ThrowTypeError(ctx,"setName: expect string");
            const char* s   =   JS_ToCString(ctx,argv[0]);
            track->setName(s != nullptr ? s : "");
            JS_FreeCString(ctx,s);
            return  JS_UNDEFINED;
        }

        static  JSValue Track_propertyIndex(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            (void)argv;
            auto    track   =   checkObj<FEKeyFrameTrack>(ctx,getEnv(ctx),this_val);
            if (track == nullptr)
                return  JS_EXCEPTION;
            return  JS_NewInt32(ctx,(int32_t)track->propertyIndex());
        }

        static  JSValue Track_setPropertyIndex(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            auto    track   =   checkObj<FEKeyFrameTrack>(ctx,getEnv(ctx),this_val);
            if (track == nullptr)
                return  JS_EXCEPTION;
            int64_t v;
            if (!isIntegerValue(ctx,argv[0],v))
                return  JS_ThrowTypeError(ctx,"setPropertyIndex: expect integer");
            track->setPropertyIndex((PropIndex)v);
            return  JS_UNDEFINED;
        }

        static  JSValue Track_addKeyFrame(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            JSEnv*  env     =   getEnv(ctx);
            auto    track   =   checkObj<FEKeyFrameTrack>(ctx,env,this_val);
            if (track == nullptr)
                return  JS_EXCEPTION;
            real    time;
            KFValue    val;
            if (!argNum(ctx,argv[0],time))
                return  JS_ThrowTypeError(ctx,"addKeyFrame: expect number time");
            if (!toKFValue(ctx,env,argv[1],val))
                return  JS_ThrowTypeError(ctx,"addKeyFrame: unsupported value type(number/object expected)");
            return  JS_NewBool(ctx,track->addKeyFrame(time,val));
        }

        static  JSValue Track_removeKeyFrame(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            auto    track   =   checkObj<FEKeyFrameTrack>(ctx,getEnv(ctx),this_val);
            if (track == nullptr)
                return  JS_EXCEPTION;
            bool    ok  =   false;
            int64_t idx;
            real    time;
            if (isIntegerValue(ctx,argv[0],idx))
                ok  =   track->removeKeyFrame((size_t)idx);
            else if (argNum(ctx,argv[0],time))
                ok  =   track->removeKeyFrame(time);
            else
                return  JS_ThrowTypeError(ctx,"removeKeyFrame: expect integer index or number time");
            return  JS_NewBool(ctx,ok);
        }

        static  JSValue Track_updateKeyFrame(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            JSEnv*  env     =   getEnv(ctx);
            auto    track   =   checkObj<FEKeyFrameTrack>(ctx,env,this_val);
            if (track == nullptr)
                return  JS_EXCEPTION;
            KFValue    val;
            if (!toKFValue(ctx,env,argv[1],val))
                return  JS_ThrowTypeError(ctx,"updateKeyFrame: unsupported value type(number/object expected)");
            bool    ok  =   false;
            int64_t idx;
            real    time;
            if (isIntegerValue(ctx,argv[0],idx))
                ok  =   track->updateKeyFrame((size_t)idx,val);
            else if (argNum(ctx,argv[0],time))
                ok  =   track->updateKeyFrame(time,val);
            else
                return  JS_ThrowTypeError(ctx,"updateKeyFrame: expect integer index or number time");
            return  JS_NewBool(ctx,ok);
        }

        static  JSValue Track_keyCount(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            (void)argv;
            auto    track   =   checkObj<FEKeyFrameTrack>(ctx,getEnv(ctx),this_val);
            if (track == nullptr)
                return  JS_EXCEPTION;
            auto    times   =   track->times();
            return  JS_NewInt32(ctx,times ? (int32_t)times->values().size() : 0);
        }

        static  JSValue Track_times(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            (void)argv;
            auto    track   =   checkObj<FEKeyFrameTrack>(ctx,getEnv(ctx),this_val);
            if (track == nullptr)
                return  JS_EXCEPTION;
            JSValue arr     =   JS_NewArray(ctx);
            uint32_t    n   =   0;
            auto    times   =   track->times();
            if (times != nullptr)
            {
                for (auto v : times->values())
                    JS_DefinePropertyValueUint32(ctx,arr,n++,JS_NewFloat64(ctx,(double)v),JS_PROP_C_W_E);
            }
            return  arr;
        }

        /// <summary>
        /// 按值类型压栈:标量压数字,向量压 {x,y[,z[,w]]},颜色压 {r,g,b,a}
        /// </summary>
        template<class V>
        static  JSValue pushVecValue(JSContext* ctx,JSEnv* env,const V& v)
        {
            int     n   =   (int)V::length();
            if (n == 2)
                return  pushReal2(ctx,env,real2(v[0],v[1]));
            else if (n == 3)
                return  pushReal3(ctx,env,real3(v[0],v[1],v[2]));
            return  pushReal4(ctx,env,real4(v[0],v[1],v[2],v[3]));
        }

        static  JSValue Track_values(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            (void)argv;
            JSEnv*  env     =   getEnv(ctx);
            auto    track   =   checkObj<FEKeyFrameTrack>(ctx,env,this_val);
            if (track == nullptr)
                return  JS_EXCEPTION;
            JSValue arr     =   JS_NewArray(ctx);
            uint32_t    n   =   0;
            if (track->values().index() != 0)
            {
                std::visit([&](auto&& valuesObj)
                {
                    using   TObj    =   std::decay_t<decltype(valuesObj)>;
                    if constexpr (!std::is_same_v<TObj,std::monostate>)
                    {
                        for (const auto& v : valuesObj->values())
                        {
                            using   V   =   std::decay_t<decltype(v)>;
                            JSValue out;
                            if constexpr (std::is_same_v<V,quatf> || std::is_same_v<V,quatr>)
                            {
                                /// 轨道值存的是 quatr(tquat<real>=double),与 quatf(float) 不是同一类型;
                                /// glm tquat 构造函数是 (w,x,y,z),成员赋值保证 (x,y,z,w) 顺序
                                quatr   qv;
                                qv.x    =   v.x;
                                qv.y    =   v.y;
                                qv.z    =   v.z;
                                qv.w    =   v.w;
                                out =   pushQuatr(ctx,env,qv);
                            }
                            else if constexpr (std::is_same_v<V,uint8x4>)
                            {
                                /// 颜色值压成 {r,g,b,a},对齐 toKFValue 的 uint8x4 分支
                                out =   pushUint8x4(ctx,v);
                            }
                            else if constexpr (std::is_arithmetic_v<V>)
                                out =   JS_NewFloat64(ctx,(double)v);
                            else
                                out =   pushVecValue(ctx,env,v);
                            JS_DefinePropertyValueUint32(ctx,arr,n++,out,JS_PROP_C_W_E);
                        }
                    }
                },track->values());
            }
            return  arr;
        }

        static  JSValue Track_range(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            (void)argv;
            auto    track   =   checkObj<FEKeyFrameTrack>(ctx,getEnv(ctx),this_val);
            if (track == nullptr)
                return  JS_EXCEPTION;
            return  pushReal2Range(ctx,track->range());
        }

        static  JSValue Track_isValid(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            (void)argv;
            auto    track   =   checkObj<FEKeyFrameTrack>(ctx,getEnv(ctx),this_val);
            if (track == nullptr)
                return  JS_EXCEPTION;
            return  JS_NewBool(ctx,track->isValid());
        }

        static  JSValue Track_setInterpolation(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            auto    track   =   checkObj<FEKeyFrameTrack>(ctx,getEnv(ctx),this_val);
            if (track == nullptr)
                return  JS_EXCEPTION;
            int64_t v;
            if (!isIntegerValue(ctx,argv[0],v))
                return  JS_ThrowTypeError(ctx,"setInterpolation: expect integer");
            track->_type    =   (FEKeyFrameTrack::InterpolateType)v;
            return  JS_UNDEFINED;
        }

        static  JSValue Track_interpolation(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            (void)argv;
            auto    track   =   checkObj<FEKeyFrameTrack>(ctx,getEnv(ctx),this_val);
            if (track == nullptr)
                return  JS_EXCEPTION;
            return  JS_NewInt32(ctx,(int32_t)track->_type);
        }

        static  JSValue Track_setTension(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            auto    track   =   checkObj<FEKeyFrameTrack>(ctx,getEnv(ctx),this_val);
            if (track == nullptr)
                return  JS_EXCEPTION;
            real    v;
            if (!argNum(ctx,argv[0],v))
                return  JS_ThrowTypeError(ctx,"setTension: expect number");
            track->_tension =   v;
            return  JS_UNDEFINED;
        }

        static  JSValue Track_tension(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            (void)argv;
            auto    track   =   checkObj<FEKeyFrameTrack>(ctx,getEnv(ctx),this_val);
            if (track == nullptr)
                return  JS_EXCEPTION;
            return  JS_NewFloat64(ctx,(double)track->_tension);
        }

        static  const JSFuncDef TrackMethods[]  =
        {
            {   "name",             Track_name,             0   },
            {   "setName",          Track_setName,          1   },
            {   "propertyIndex",    Track_propertyIndex,    0   },
            {   "setPropertyIndex", Track_setPropertyIndex, 1   },
            {   "addKeyFrame",      Track_addKeyFrame,      2   },
            {   "removeKeyFrame",   Track_removeKeyFrame,   1   },
            {   "updateKeyFrame",   Track_updateKeyFrame,   2   },
            {   "keyCount",         Track_keyCount,         0   },
            {   "times",            Track_times,            0   },
            {   "values",           Track_values,           0   },
            {   "range",            Track_range,            0   },
            {   "isValid",          Track_isValid,          0   },
            {   "setInterpolation", Track_setInterpolation, 1   },
            {   "interpolation",    Track_interpolation,    0   },
            {   "setTension",       Track_setTension,       1   },
            {   "tension",          Track_tension,          0   },
            {   nullptr,            nullptr,                0   },
        };

        // ==================== Playback(Action) ====================

        static  JSValue Playback_play(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            (void)argv;
            auto    act     =   checkObj<FEPlayback>(ctx,getEnv(ctx),this_val);
            if (act == nullptr)
                return  JS_EXCEPTION;
            act->play();
            return  JS_UNDEFINED;
        }

        static  JSValue Playback_stop(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            (void)argv;
            auto    act     =   checkObj<FEPlayback>(ctx,getEnv(ctx),this_val);
            if (act == nullptr)
                return  JS_EXCEPTION;
            act->stop();
            return  JS_UNDEFINED;
        }

        static  JSValue Playback_setMode(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            auto    act     =   checkObj<FEPlayback>(ctx,getEnv(ctx),this_val);
            if (act == nullptr)
                return  JS_EXCEPTION;
            int64_t v;
            if (!isIntegerValue(ctx,argv[0],v))
                return  JS_ThrowTypeError(ctx,"setMode: expect integer");
            act->setMode((FEPlayback::PlayMode)v);
            return  JS_UNDEFINED;
        }

        static  JSValue Playback_status(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            (void)argv;
            auto    act     =   checkObj<FEPlayback>(ctx,getEnv(ctx),this_val);
            if (act == nullptr)
                return  JS_EXCEPTION;
            return  JS_NewInt32(ctx,(int32_t)act->status());
        }

        static  JSValue Playback_setClipTime(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            auto    act     =   checkObj<FEPlayback>(ctx,getEnv(ctx),this_val);
            if (act == nullptr)
                return  JS_EXCEPTION;
            real    v;
            if (!argNum(ctx,argv[0],v))
                return  JS_ThrowTypeError(ctx,"setClipTime: expect number");
            act->setClipTime(v);
            return  JS_UNDEFINED;
        }

        static  JSValue Playback_buildCache(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            (void)argv;
            auto    act     =   checkObj<FEPlayback>(ctx,getEnv(ctx),this_val);
            if (act == nullptr)
                return  JS_EXCEPTION;
            act->buildCache();
            return  JS_UNDEFINED;
        }

        static  JSValue Playback_clearCache(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            (void)argv;
            auto    act     =   checkObj<FEPlayback>(ctx,getEnv(ctx),this_val);
            if (act == nullptr)
                return  JS_EXCEPTION;
            act->clearCache();
            return  JS_UNDEFINED;
        }

        static  JSValue Playback_isCacheMode(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            (void)argv;
            auto    act     =   checkObj<FEPlayback>(ctx,getEnv(ctx),this_val);
            if (act == nullptr)
                return  JS_EXCEPTION;
            return  JS_NewBool(ctx,act->isCacheMode());
        }

        static  JSValue Playback_refreshRange(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            (void)argv;
            auto    act     =   checkObj<FEPlayback>(ctx,getEnv(ctx),this_val);
            if (act == nullptr)
                return  JS_EXCEPTION;
            act->refreshRange();
            return  JS_UNDEFINED;
        }

        static  JSValue Playback_addAnimation(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            JSEnv*  env     =   getEnv(ctx);
            auto    act     =   checkObj<FEPlayback>(ctx,env,this_val);
            if (act == nullptr)
                return  JS_EXCEPTION;
            auto    anim    =   checkObj<FEAnimation>(ctx,env,argv[0]);
            if (anim == nullptr)
                return  JS_EXCEPTION;
            return  JS_NewInt32(ctx,(int32_t)act->addObject(anim));
        }

        static  JSValue Playback_removeAnimation(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            JSEnv*  env     =   getEnv(ctx);
            auto    act     =   checkObj<FEPlayback>(ctx,env,this_val);
            if (act == nullptr)
                return  JS_EXCEPTION;
            auto    anim    =   checkObj<FEAnimation>(ctx,env,argv[0]);
            if (anim == nullptr)
                return  JS_EXCEPTION;
            return  JS_NewInt32(ctx,(int32_t)act->removeObject(anim));
        }

        static  JSValue Playback_count(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            (void)argv;
            auto    act     =   checkObj<FEPlayback>(ctx,getEnv(ctx),this_val);
            if (act == nullptr)
                return  JS_EXCEPTION;
            return  JS_NewInt32(ctx,(int32_t)act->objects().size());
        }

        static  const JSFuncDef PlaybackMethods[]   =
        {
            {   "play",             Playback_play,            0   },
            {   "stop",             Playback_stop,            0   },
            {   "setMode",          Playback_setMode,         1   },
            {   "status",           Playback_status,          0   },
            {   "setClipTime",      Playback_setClipTime,     1   },
            {   "buildCache",       Playback_buildCache,      0   },
            {   "clearCache",       Playback_clearCache,      0   },
            {   "isCacheMode",      Playback_isCacheMode,     0   },
            {   "refreshRange",     Playback_refreshRange,    0   },
            {   "addAnimation",     Playback_addAnimation,    1   },
            {   "removeAnimation",  Playback_removeAnimation, 1   },
            {   "count",            Playback_count,           0   },
            {   nullptr,            nullptr,                  0   },
        };

        // ==================== AnimationSys ====================

        /// <summary>
        /// sys:getOrCreate([name]) 获取或创建播放器
        /// </summary>
        static  JSValue AnimSys_getOrCreate(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            auto    sys     =   checkObj<FEAnimationSys>(ctx,getEnv(ctx),this_val);
            if (sys == nullptr)
                return  JS_EXCEPTION;
            String  name    =   "default";
            if (JS_IsString(argv[0]))
            {
                const char* s   =   JS_ToCString(ctx,argv[0]);
                if (s != nullptr)
                    name    =   s;
                JS_FreeCString(ctx,s);
            }
            return  pushObj<FEPlayback>(ctx,getEnv(ctx),sys->getOrCreate(name));
        }

        static  JSValue AnimSys_count(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            (void)argv;
            auto    sys     =   checkObj<FEAnimationSys>(ctx,getEnv(ctx),this_val);
            if (sys == nullptr)
                return  JS_EXCEPTION;
            return  JS_NewInt32(ctx,(int32_t)sys->actions().size());
        }

        static  JSValue AnimSys_clear(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)argc;
            (void)argv;
            auto    sys     =   checkObj<FEAnimationSys>(ctx,getEnv(ctx),this_val);
            if (sys == nullptr)
                return  JS_EXCEPTION;
            sys->clear();
            return  JS_UNDEFINED;
        }

        static  const JSFuncDef AnimSysMethods[]    =
        {
            {   "getOrCreate",  AnimSys_getOrCreate,    1   },
            {   "count",        AnimSys_count,          0   },
            {   "clear",        AnimSys_clear,          0   },
            {   nullptr,        nullptr,                0   },
        };

        // ==================== fe.anim 模块函数 ====================

        static  JSValue AnimModule_createAnimation(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)this_val;
            (void)argc;
            (void)argv;
            auto*   env     =   getEnv(ctx);
            Animation   anim    =   new FEAnimation(*env->ctx);
            return  pushObj<FEAnimation>(ctx,env,anim);
        }

        static  JSValue AnimModule_createNodeAnimation(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)this_val;
            (void)argc;
            auto*   env     =   getEnv(ctx);
            NodeProperyBits bits(NP_Default);
            if (argc >= 1 && JS_IsNumber(argv[0]))
            {
                int64_t v;
                JS_ToInt64(ctx,&v,argv[0]);
                bits    =   NodeProperyBits((uint32)v);
            }
            return  pushObj<FEAnimation>(ctx,env,FEAnimationHelper::createNodeAnimtion(*env->ctx,bits));
        }

        static  JSValue AnimModule_createClip(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)this_val;
            (void)argc;
            (void)argv;
            auto*   env     =   getEnv(ctx);
            AnimClip    clip    =   new FEAnimClip(*env->ctx);
            return  pushObj<FEAnimClip>(ctx,env,clip);
        }

        /// <summary>
        /// 值类型字符串 -> 值数组对象
        /// </summary>
        static  ValueObject makeValueObject(FEContext& ctx,const char* type,bool& ok)
        {
            ok      =   true;
            String  t   =   type;
            if (t == "real")    return  ValueObject(RealsObject(new FERealsObject(ctx)));
            if (t == "real2")   return  ValueObject(Real2sObject(new FEReal2sObject(ctx)));
            if (t == "real3")   return  ValueObject(Real3sObject(new FEReal3sObject(ctx)));
            if (t == "real4")   return  ValueObject(Real4sObject(new FEReal4sObject(ctx)));
            if (t == "quat")    return  ValueObject(QuatrsObject(new FEQuatrsObject(ctx)));
            if (t == "quatf")   return  ValueObject(QuatfsObject(new FEQuatfsObject(ctx)));
            if (t == "float")   return  ValueObject(FloatsObject(new FEFloatsObject(ctx)));
            if (t == "float2")  return  ValueObject(Float2sObject(new FEFloat2sObject(ctx)));
            if (t == "float3")  return  ValueObject(Float3sObject(new FEFloat3sObject(ctx)));
            if (t == "float4")  return  ValueObject(Float4sObject(new FEFloat4sObject(ctx)));
            if (t == "color")   return  ValueObject(RgbaObject(new FEColorObject(ctx)));
            if (t == "alpha")   return  ValueObject(AlphaObject(new FEAlphaObject(ctx)));
            if (t == "bool")    return  ValueObject(BoolsObject(new FEBoolsObject(ctx)));
            if (t == "int")     return  ValueObject(IntsObject(new FEIntsObject(ctx)));
            if (t == "int2")    return  ValueObject(Int2sObject(new FEInt2sObject(ctx)));
            if (t == "int3")    return  ValueObject(Int3sObject(new FEInt3sObject(ctx)));
            if (t == "int4")    return  ValueObject(Int4sObject(new FEInt4sObject(ctx)));
            if (t == "uint")    return  ValueObject(UintsObject(new FEUintsObject(ctx)));
            if (t == "uint2")   return  ValueObject(Uint2sObject(new FEUint2sObject(ctx)));
            if (t == "uint3")   return  ValueObject(Uint3sObject(new FEUint3sObject(ctx)));
            if (t == "uint4")   return  ValueObject(Uint4sObject(new FEUint4sObject(ctx)));
            ok  =   false;
            return  ValueObject();
        }

        /// <summary>
        /// createTrack(propIndex, valueType):创建轨道并初始化时间线/值数组
        /// </summary>
        static  JSValue AnimModule_createTrack(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)this_val;
            (void)argc;
            auto*   env         =   getEnv(ctx);
            int64_t propIdx     =   0;
            if (!isIntegerValue(ctx,argv[0],propIdx))
                return  JS_ThrowTypeError(ctx,"createTrack: expect integer propertyIndex");
            if (!JS_IsString(argv[1]))
                return  JS_ThrowTypeError(ctx,"createTrack: expect value type string");
            const char* type    =   JS_ToCString(ctx,argv[1]);
            if (type == nullptr)
                return  JS_EXCEPTION;
            bool    ok          =   false;
            auto    valueObj    =   makeValueObject(*env->ctx,type,ok);
            JS_FreeCString(ctx,type);
            if (!ok)
                return  JS_ThrowTypeError(ctx,"createTrack: unknown value type");
            KeyFrameTrack   track   =   new FEKeyFrameTrack(*env->ctx,(PropIndex)propIdx);
            track->setTimeObject(RealsObject(new FERealsObject(*env->ctx)));
            track->setValueObject(valueObj);
            return  pushObj<FEKeyFrameTrack>(ctx,env,track);
        }

        /// <summary>
        /// action([name]):取场景动画系统中的播放器(即 AnimationSys::getOrCreate)
        /// </summary>
        static  JSValue AnimModule_action(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)this_val;
            (void)argc;
            auto*   env     =   getEnv(ctx);
            if (env->scene == nullptr)
                return  JS_ThrowTypeError(ctx,"fe.anim.action: scene not set");
            auto*   sys     =   env->scene->animationSystem();
            if (sys == nullptr)
                return  JS_ThrowTypeError(ctx,"fe.anim.action: animation system not ready");
            String  name    =   "default";
            if (JS_IsString(argv[0]))
            {
                const char* s   =   JS_ToCString(ctx,argv[0]);
                if (s != nullptr)
                    name    =   s;
                JS_FreeCString(ctx,s);
            }
            return  pushObj<FEPlayback>(ctx,env,sys->getOrCreate(name));
        }

        /// <summary>
        /// 可选整数位标志参数(NodeProperyBits)
        /// </summary>
        static  NodeProperyBits optBits(JSContext* ctx,int argc,JSValueConst* argv,uint32 def)
        {
            if (argc >= 1 && JS_IsNumber(argv[0]))
            {
                int64_t v;
                JS_ToInt64(ctx,&v,argv[0]);
                return  NodeProperyBits((uint32)v);
            }
            return  NodeProperyBits(def);
        }

        static  JSValue AnimModule_addKeyFrameFromNode(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)this_val;
            JSEnv*  env     =   getEnv(ctx);
            auto    anim    =   checkObj<FEAnimation>(ctx,env,argv[0]);
            if (anim == nullptr)
                return  JS_EXCEPTION;
            real    time;
            if (!argNum(ctx,argv[1],time))
                return  JS_ThrowTypeError(ctx,"addKeyFrameFromNode: expect number time");
            auto    node    =   checkObj<FENode>(ctx,env,argv[2]);
            if (node == nullptr)
                return  JS_EXCEPTION;
            NodeProperyBits bits    =   optBits(ctx,argc - 3,argv + 3,0xFFFFFFFF);
            return  JS_NewBool(ctx,FEAnimationHelper::addNodeKeyFrame(anim,time,node,bits));
        }

        static  JSValue AnimModule_updateKeyFrameFromNode(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)this_val;
            JSEnv*  env     =   getEnv(ctx);
            auto    anim    =   checkObj<FEAnimation>(ctx,env,argv[0]);
            if (anim == nullptr)
                return  JS_EXCEPTION;
            real    time;
            if (!argNum(ctx,argv[1],time))
                return  JS_ThrowTypeError(ctx,"updateKeyFrameFromNode: expect number time");
            auto    node    =   checkObj<FENode>(ctx,env,argv[2]);
            if (node == nullptr)
                return  JS_EXCEPTION;
            NodeProperyBits bits    =   optBits(ctx,argc - 3,argv + 3,0xFFFFFFFF);
            return  JS_NewBool(ctx,FEAnimationHelper::updateNodeKeyFrame(anim,time,node,bits));
        }

        static  JSValue AnimModule_removeKeyFrameFromNode(JSContext* ctx,JSValueConst this_val,int argc,JSValueConst* argv)
        {
            (void)this_val;
            auto    anim    =   checkObj<FEAnimation>(ctx,getEnv(ctx),argv[0]);
            if (anim == nullptr)
                return  JS_EXCEPTION;
            real    time;
            if (!argNum(ctx,argv[1],time))
                return  JS_ThrowTypeError(ctx,"removeKeyFrameFromNode: expect number time");
            return  JS_NewBool(ctx,FEAnimationHelper::removeNodeKeyFrame(anim,time));
        }

        static  const JSFuncDef AnimModuleMethods[] =
        {
            {   "createAnimation",          AnimModule_createAnimation,         0   },
            {   "createNodeAnimation",      AnimModule_createNodeAnimation,     1   },
            {   "createClip",               AnimModule_createClip,              0   },
            {   "createTrack",              AnimModule_createTrack,             2   },
            {   "action",                   AnimModule_action,                  1   },
            {   "addKeyFrameFromNode",      AnimModule_addKeyFrameFromNode,     4   },
            {   "updateKeyFrameFromNode",   AnimModule_updateKeyFrameFromNode,  4   },
            {   "removeKeyFrameFromNode",   AnimModule_removeKeyFrameFromNode,  2   },
            {   nullptr,                    nullptr,                            0   },
        };

        /// <summary>
        /// 向对象写入一个整型常量
        /// </summary>
        static  void    setConst(JSContext* ctx,JSValue obj,const char* name,int32 v)
        {
            JS_SetPropertyStr(ctx,obj,name,JS_NewInt32(ctx,v));
        }
    }

    /// <summary>
    /// 注册动画对象类与 fe.anim 子模块(函数+常量)
    /// </summary>
    void    FEQJSRegisterAnim(JSContext* ctx,JSEnv* env,JSValue fe)
    {
        /// 注册对象类
        registerClass<FEAnimation>(ctx,env,AnimMethods);
        registerClass<FEAnimClip>(ctx,env,ClipMethods);
        registerClass<FEKeyFrameTrack>(ctx,env,TrackMethods);
        registerClass<FEPlayback>(ctx,env,PlaybackMethods);
        registerClass<FEAnimationSys>(ctx,env,AnimSysMethods);

        /// fe.anim 子表
        JSValue anim    =   JS_NewObject(ctx);
        for (int i = 0; AnimModuleMethods[i].name != nullptr; ++i)
            JS_SetPropertyStr(ctx,anim,AnimModuleMethods[i].name,
                              JS_NewCFunction(ctx,AnimModuleMethods[i].fn,AnimModuleMethods[i].name,AnimModuleMethods[i].length));

        /// 节点属性位(NP_*)
        setConst(ctx,anim,"NP_TRANSFORM_X",NP_TRANSFORM_X);
        setConst(ctx,anim,"NP_TRANSFORM_Y",NP_TRANSFORM_Y);
        setConst(ctx,anim,"NP_TRANSFORM_Z",NP_TRANSFORM_Z);
        setConst(ctx,anim,"NP_TRANSFORM_XYZ",NP_TRANSFORM_XYZ);
        setConst(ctx,anim,"NP_SCALE_X",NP_SCALE_X);
        setConst(ctx,anim,"NP_SCALE_Y",NP_SCALE_Y);
        setConst(ctx,anim,"NP_SCALE_Z",NP_SCALE_Z);
        setConst(ctx,anim,"NP_SCALE_XYZ",NP_SCALE_XYZ);
        setConst(ctx,anim,"NP_ROTATE_X",NP_ROTATE_X);
        setConst(ctx,anim,"NP_ROTATE_Y",NP_ROTATE_Y);
        setConst(ctx,anim,"NP_ROTATE_Z",NP_ROTATE_Z);
        setConst(ctx,anim,"NP_ROTATE_XYZ",NP_ROTATE_XYZ);
        setConst(ctx,anim,"NP_QUAT",NP_QUAT);
        setConst(ctx,anim,"NP_COLOR_RGB",NP_COLOR_RGB);
        setConst(ctx,anim,"NP_COLOR_ALPHA",NP_COLOR_ALPHA);
        setConst(ctx,anim,"NP_VISIBLE",NP_VISIBLE);
        setConst(ctx,anim,"NP_Default",NP_Default);

        /// 属性索引(PROP_*)
        setConst(ctx,anim,"PROP_TRANSFORM_X",PROP_TRANSFORM_X);
        setConst(ctx,anim,"PROP_TRANSFORM_Y",PROP_TRANSFORM_Y);
        setConst(ctx,anim,"PROP_TRANSFORM_Z",PROP_TRANSFORM_Z);
        setConst(ctx,anim,"PROP_TRANSFORM_XYZ",PROP_TRANSFORM_XYZ);
        setConst(ctx,anim,"PROP_SCALE_X",PROP_SCALE_X);
        setConst(ctx,anim,"PROP_SCALE_Y",PROP_SCALE_Y);
        setConst(ctx,anim,"PROP_SCALE_Z",PROP_SCALE_Z);
        setConst(ctx,anim,"PROP_SCALE_XYZ",PROP_SCALE_XYZ);
        setConst(ctx,anim,"PROP_ROTATE_X",PROP_ROTATE_X);
        setConst(ctx,anim,"PROP_ROTATE_Y",PROP_ROTATE_Y);
        setConst(ctx,anim,"PROP_ROTATE_Z",PROP_ROTATE_Z);
        setConst(ctx,anim,"PROP_ROTATE_XYZ",PROP_ROTATE_XYZ);
        setConst(ctx,anim,"PROP_QUAT",PROP_QUAT);
        setConst(ctx,anim,"PROP_COLOR_RGB",PROP_COLOR_RGB);
        setConst(ctx,anim,"PROP_COLOR_ALPHA",PROP_COLOR_ALPHA);
        setConst(ctx,anim,"PROP_VISIBLE",PROP_VISIBLE);
        setConst(ctx,anim,"PROP_MAT_DIFFUSE",PROP_MAT_DIFFUSE);
        setConst(ctx,anim,"PROP_G_TRANSFORM_XYZ",PROP_G_TRANSFORM_XYZ);

        /// 插值方式
        setConst(ctx,anim,"IT_BEZIER",(int32)FEKeyFrameTrack::IT_Bezier);
        setConst(ctx,anim,"IT_LINEAR",(int32)FEKeyFrameTrack::IT_Linear);
        setConst(ctx,anim,"IT_CONSTANT",(int32)FEKeyFrameTrack::IT_Constant);

        /// 播放模式与时间模式
        setConst(ctx,anim,"PT_ONCE",(int32)FEPlayback::PT_Once);
        setConst(ctx,anim,"PT_LOOP",(int32)FEPlayback::PT_Loop);
        setConst(ctx,anim,"PT_PINGPONG",(int32)FEPlayback::PT_PingPong);
        setConst(ctx,anim,"TM_DEFAULT",(int32)FEPlayback::TM_Default);
        setConst(ctx,anim,"TM_INVERT",(int32)FEPlayback::TM_Invert);

        JS_SetPropertyStr(ctx,fe,"anim",anim);
    }
}
