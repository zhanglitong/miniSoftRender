/// FELuaAnim
/// 动画系统 Lua 绑定:脚本可创建 clip/track/关键帧,绑定节点并控制播放
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
/// 对象方法见各 luaXxxMethods 表;数学参数兼容 {x=,y=,z=} 与 {1,2,3}

#include    "FELuaHelper.hpp"

namespace   FE
{
    // ==================== Animation ====================

    static  int Anim_name(lua_State* L)
    {
        auto    anim    =   checkObj<FEAnimation>(L,1);
        lua_pushstring(L,anim->name().c_str());
        return  1;
    }

    static  int Anim_setName(lua_State* L)
    {
        auto    anim    =   checkObj<FEAnimation>(L,1);
        anim->setName(luaL_checkstring(L,2));
        return  0;
    }

    static  int Anim_offset(lua_State* L)
    {
        auto    anim    =   checkObj<FEAnimation>(L,1);
        lua_pushnumber(L,anim->offset());
        return  1;
    }

    static  int Anim_setOffset(lua_State* L)
    {
        auto    anim    =   checkObj<FEAnimation>(L,1);
        anim->setOffset(luaL_checknumber(L,2));
        return  0;
    }

    static  int Anim_clip(lua_State* L)
    {
        auto    anim    =   checkObj<FEAnimation>(L,1);
        pushObj<FEAnimClip>(L,anim->clip());
        return  1;
    }

    static  int Anim_setClip(lua_State* L)
    {
        auto    anim    =   checkObj<FEAnimation>(L,1);
        auto    clip    =   checkObj<FEAnimClip>(L,2);
        anim->setClip(clip);
        return  0;
    }

    static  int Anim_range(lua_State* L)
    {
        auto    anim    =   checkObj<FEAnimation>(L,1);
        pushReal2Range(L,anim->range());
        return  1;
    }

    static  int Anim_isValid(lua_State* L)
    {
        auto    anim    =   checkObj<FEAnimation>(L,1);
        lua_pushboolean(L,anim->isValid() ? 1 : 0);
        return  1;
    }

    static  int Anim_isChanged(lua_State* L)
    {
        auto    anim    =   checkObj<FEAnimation>(L,1);
        lua_pushboolean(L,anim->isChanged() ? 1 : 0);
        return  1;
    }

    static  int Anim_clearChanged(lua_State* L)
    {
        auto    anim    =   checkObj<FEAnimation>(L,1);
        anim->clearChanged();
        return  0;
    }

    static  const luaL_Reg  AnimMethods[]  =
    {
        {   "name",         Anim_name           },
        {   "setName",      Anim_setName        },
        {   "offset",       Anim_offset         },
        {   "setOffset",    Anim_setOffset      },
        {   "clip",         Anim_clip           },
        {   "setClip",      Anim_setClip        },
        {   "range",        Anim_range          },
        {   "isValid",      Anim_isValid        },
        {   "isChanged",    Anim_isChanged      },
        {   "clearChanged", Anim_clearChanged   },
        {   nullptr,        nullptr             },
    };

    // ==================== AnimClip ====================

    static  int Clip_addKeyFrame(lua_State* L)
    {
        auto    clip    =   checkObj<FEAnimClip>(L,1);
        real    time    =   luaL_checknumber(L,2);
        KFValue    val;
        if (!toKFValue(L,3,val))
            return  luaL_error(L,"addKeyFrame: unsupported value type(number/table expected)");
        lua_pushboolean(L,clip->addKeyFrame(time,val) ? 1 : 0);
        return  1;
    }

    static  int Clip_insertKeyFrame(lua_State* L)
    {
        auto    clip    =   checkObj<FEAnimClip>(L,1);
        /// Lua 索引从 1 开始,引擎从 0 开始
        size_t  idx     =   (size_t)(luaL_checkinteger(L,2) - 1);
        real    time    =   luaL_checknumber(L,3);
        KFValue    val;
        if (!toKFValue(L,4,val))
            return  luaL_error(L,"insertKeyFrame: unsupported value type(number/table expected)");
        lua_pushboolean(L,clip->insertKeyFrame(idx,time,val) ? 1 : 0);
        return  1;
    }

    /// <summary>
    /// removeKeyFrame(time) 按时间移除;removeKeyFrame(idx) 整数按索引移除(Lua 索引从 1 开始)
    /// </summary>
    static  int Clip_removeKeyFrame(lua_State* L)
    {
        auto    clip    =   checkObj<FEAnimClip>(L,1);
        bool    ok      =   false;
        if (lua_isinteger(L,2))
            ok  =   clip->removeKeyFrame((size_t)(lua_tointeger(L,2) - 1));
        else
            ok  =   clip->removeKeyFrame((real)luaL_checknumber(L,2));
        lua_pushboolean(L,ok ? 1 : 0);
        return  1;
    }

    /// <summary>
    /// updateKeyFrame(time,val) / updateKeyFrame(idx,val)(Lua 索引从 1 开始)
    /// </summary>
    static  int Clip_updateKeyFrame(lua_State* L)
    {
        auto    clip    =   checkObj<FEAnimClip>(L,1);
        KFValue    val;
        if (!toKFValue(L,3,val))
            return  luaL_error(L,"updateKeyFrame: unsupported value type(number/table expected)");
        bool    ok      =   false;
        if (lua_isinteger(L,2))
            ok  =   clip->updateKeyFrame((size_t)(lua_tointeger(L,2) - 1),val);
        else
            ok  =   clip->updateKeyFrame((real)luaL_checknumber(L,2),val);
        lua_pushboolean(L,ok ? 1 : 0);
        return  1;
    }

    static  int Clip_addTrack(lua_State* L)
    {
        auto    clip    =   checkObj<FEAnimClip>(L,1);
        auto    track   =   checkObj<FEKeyFrameTrack>(L,2);
        lua_pushinteger(L,(lua_Integer)clip->addTrack(track));
        return  1;
    }

    static  int Clip_trackCount(lua_State* L)
    {
        auto    clip    =   checkObj<FEAnimClip>(L,1);
        lua_pushinteger(L,(lua_Integer)clip->tracks().size());
        return  1;
    }

    static  int Clip_getTrack(lua_State* L)
    {
        auto    clip    =   checkObj<FEAnimClip>(L,1);
        /// Lua 索引从 1 开始
        auto    idx     =   luaL_checkinteger(L,2);
        if (idx < 1 || (size_t)idx > clip->tracks().size())
            return  luaL_error(L,"getTrack: index out of range");
        pushObj<FEKeyFrameTrack>(L,clip->tracks()[(size_t)idx - 1]);
        return  1;
    }

    static  int Clip_range(lua_State* L)
    {
        auto    clip    =   checkObj<FEAnimClip>(L,1);
        pushReal2Range(L,clip->range());
        return  1;
    }

    static  int Clip_simplifyTimeline(lua_State* L)
    {
        auto    clip    =   checkObj<FEAnimClip>(L,1);
        clip->simplifyTimeline();
        return  0;
    }

    static  const luaL_Reg  ClipMethods[]  =
    {
        {   "addKeyFrame",      Clip_addKeyFrame        },
        {   "insertKeyFrame",   Clip_insertKeyFrame     },
        {   "removeKeyFrame",   Clip_removeKeyFrame     },
        {   "updateKeyFrame",   Clip_updateKeyFrame     },
        {   "addTrack",         Clip_addTrack           },
        {   "trackCount",       Clip_trackCount         },
        {   "getTrack",         Clip_getTrack           },
        {   "range",            Clip_range              },
        {   "simplifyTimeline", Clip_simplifyTimeline   },
        {   nullptr,            nullptr                 },
    };

    // ==================== KeyFrameTrack ====================

    static  int Track_name(lua_State* L)
    {
        auto    track   =   checkObj<FEKeyFrameTrack>(L,1);
        lua_pushstring(L,track->name().c_str());
        return  1;
    }

    static  int Track_setName(lua_State* L)
    {
        auto    track   =   checkObj<FEKeyFrameTrack>(L,1);
        track->setName(luaL_checkstring(L,2));
        return  0;
    }

    static  int Track_propertyIndex(lua_State* L)
    {
        auto    track   =   checkObj<FEKeyFrameTrack>(L,1);
        lua_pushinteger(L,track->propertyIndex());
        return  1;
    }

    static  int Track_setPropertyIndex(lua_State* L)
    {
        auto    track   =   checkObj<FEKeyFrameTrack>(L,1);
        track->setPropertyIndex((PropIndex)luaL_checkinteger(L,2));
        return  0;
    }

    static  int Track_addKeyFrame(lua_State* L)
    {
        auto    track   =   checkObj<FEKeyFrameTrack>(L,1);
        real    time    =   luaL_checknumber(L,2);
        KFValue    val;
        if (!toKFValue(L,3,val))
            return  luaL_error(L,"addKeyFrame: unsupported value type(number/table expected)");
        lua_pushboolean(L,track->addKeyFrame(time,val) ? 1 : 0);
        return  1;
    }

    static  int Track_removeKeyFrame(lua_State* L)
    {
        auto    track   =   checkObj<FEKeyFrameTrack>(L,1);
        bool    ok      =   false;
        if (lua_isinteger(L,2))
            ok  =   track->removeKeyFrame((size_t)(lua_tointeger(L,2) - 1));
        else
            ok  =   track->removeKeyFrame((real)luaL_checknumber(L,2));
        lua_pushboolean(L,ok ? 1 : 0);
        return  1;
    }

    static  int Track_updateKeyFrame(lua_State* L)
    {
        auto    track   =   checkObj<FEKeyFrameTrack>(L,1);
        KFValue    val;
        if (!toKFValue(L,3,val))
            return  luaL_error(L,"updateKeyFrame: unsupported value type(number/table expected)");
        bool    ok      =   false;
        if (lua_isinteger(L,2))
            ok  =   track->updateKeyFrame((size_t)(lua_tointeger(L,2) - 1),val);
        else
            ok  =   track->updateKeyFrame((real)luaL_checknumber(L,2),val);
        lua_pushboolean(L,ok ? 1 : 0);
        return  1;
    }

    static  int Track_keyCount(lua_State* L)
    {
        auto    track   =   checkObj<FEKeyFrameTrack>(L,1);
        auto    times   =   track->times();
        lua_pushinteger(L,times ? (lua_Integer)times->values().size() : 0);
        return  1;
    }

    static  int Track_times(lua_State* L)
    {
        auto    track   =   checkObj<FEKeyFrameTrack>(L,1);
        lua_newtable(L);
        auto    times   =   track->times();
        if (times == nullptr)
            return  1;
        auto&   values  =   times->values();
        for (size_t i = 0; i < values.size(); ++i)
        {
            lua_pushnumber(L,values[i]);
            lua_seti(L,-2,(lua_Integer)i + 1);
        }
        return  1;
    }

    /// <summary>
    /// 按值类型压栈:标量压数字,向量压 {x,y[,z[,w]]},颜色压 {r,g,b,a}
    /// glm 向量统一用 operator[] 访问,避免分支实例化差异
    /// </summary>
    template<class V>
    static  void    pushVecValue(lua_State* L,const V& v)
    {
        int     n   =   (int)V::length();
        if (n == 2)
            pushReal2(L,real2(v[0],v[1]));
        else if (n == 3)
            pushReal3(L,real3(v[0],v[1],v[2]));
        else
            pushReal4(L,real4(v[0],v[1],v[2],v[3]));
    }

    static  int Track_values(lua_State* L)
    {
        auto    track   =   checkObj<FEKeyFrameTrack>(L,1);
        lua_newtable(L);
        if (track->values().index() == 0)
            return  1;
        int     n       =   1;
        std::visit([&n,L](auto&& arr)
        {
            using   TArr    =   std::decay_t<decltype(arr)>;
            if constexpr (!std::is_same_v<TArr,std::monostate>)
            {
                for (const auto& v : arr->values())
                {
                    using   V   =   std::decay_t<decltype(v)>;
                    if constexpr (std::is_same_v<V,quatf>)
                        pushQuatr(L,quatr(v.x,v.y,v.z,v.w));
                    else if constexpr (std::is_arithmetic_v<V>)
                        lua_pushnumber(L,(double)v);
                    else
                        pushVecValue(L,v);
                    lua_seti(L,-2,n);
                    ++n;
                }
            }
        },track->values());
        return  1;
    }

    static  int Track_range(lua_State* L)
    {
        auto    track   =   checkObj<FEKeyFrameTrack>(L,1);
        pushReal2Range(L,track->range());
        return  1;
    }

    static  int Track_isValid(lua_State* L)
    {
        auto    track   =   checkObj<FEKeyFrameTrack>(L,1);
        lua_pushboolean(L,track->isValid() ? 1 : 0);
        return  1;
    }

    static  int Track_setInterpolation(lua_State* L)
    {
        auto    track   =   checkObj<FEKeyFrameTrack>(L,1);
        track->_type    =   (FEKeyFrameTrack::InterpolateType)luaL_checkinteger(L,2);
        return  0;
    }

    static  int Track_interpolation(lua_State* L)
    {
        auto    track   =   checkObj<FEKeyFrameTrack>(L,1);
        lua_pushinteger(L,(lua_Integer)track->_type);
        return  1;
    }

    static  int Track_setTension(lua_State* L)
    {
        auto    track   =   checkObj<FEKeyFrameTrack>(L,1);
        track->_tension =   luaL_checknumber(L,2);
        return  0;
    }

    static  int Track_tension(lua_State* L)
    {
        auto    track   =   checkObj<FEKeyFrameTrack>(L,1);
        lua_pushnumber(L,track->_tension);
        return  1;
    }

    static  const luaL_Reg  TrackMethods[] =
    {
        {   "name",             Track_name              },
        {   "setName",          Track_setName           },
        {   "propertyIndex",    Track_propertyIndex     },
        {   "setPropertyIndex", Track_setPropertyIndex  },
        {   "addKeyFrame",      Track_addKeyFrame       },
        {   "removeKeyFrame",   Track_removeKeyFrame    },
        {   "updateKeyFrame",   Track_updateKeyFrame    },
        {   "keyCount",         Track_keyCount          },
        {   "times",            Track_times             },
        {   "values",           Track_values            },
        {   "range",            Track_range             },
        {   "isValid",          Track_isValid           },
        {   "setInterpolation", Track_setInterpolation  },
        {   "interpolation",    Track_interpolation     },
        {   "setTension",       Track_setTension        },
        {   "tension",          Track_tension           },
        {   nullptr,            nullptr                 },
    };

    // ==================== Playback(Action) ====================

    static  int Playback_play(lua_State* L)
    {
        auto    act     =   checkObj<FEPlayback>(L,1);
        act->play();
        return  0;
    }

    static  int Playback_stop(lua_State* L)
    {
        auto    act     =   checkObj<FEPlayback>(L,1);
        act->stop();
        return  0;
    }

    static  int Playback_setMode(lua_State* L)
    {
        auto    act     =   checkObj<FEPlayback>(L,1);
        act->setMode((FEPlayback::PlayMode)luaL_checkinteger(L,2));
        return  0;
    }

    static  int Playback_status(lua_State* L)
    {
        auto    act     =   checkObj<FEPlayback>(L,1);
        lua_pushinteger(L,(lua_Integer)act->status());
        return  1;
    }

    static  int Playback_setClipTime(lua_State* L)
    {
        auto    act     =   checkObj<FEPlayback>(L,1);
        act->setClipTime(luaL_checknumber(L,2));
        return  0;
    }

    static  int Playback_buildCache(lua_State* L)
    {
        auto    act     =   checkObj<FEPlayback>(L,1);
        act->buildCache();
        return  0;
    }

    static  int Playback_clearCache(lua_State* L)
    {
        auto    act     =   checkObj<FEPlayback>(L,1);
        act->clearCache();
        return  0;
    }

    static  int Playback_isCacheMode(lua_State* L)
    {
        auto    act     =   checkObj<FEPlayback>(L,1);
        lua_pushboolean(L,act->isCacheMode() ? 1 : 0);
        return  1;
    }

    static  int Playback_refreshRange(lua_State* L)
    {
        auto    act     =   checkObj<FEPlayback>(L,1);
        act->refreshRange();
        return  0;
    }

    static  int Playback_addAnimation(lua_State* L)
    {
        auto    act     =   checkObj<FEPlayback>(L,1);
        auto    anim    =   checkObj<FEAnimation>(L,2);
        lua_pushinteger(L,(lua_Integer)act->addObject(anim));
        return  1;
    }

    static  int Playback_removeAnimation(lua_State* L)
    {
        auto    act     =   checkObj<FEPlayback>(L,1);
        auto    anim    =   checkObj<FEAnimation>(L,2);
        lua_pushinteger(L,(lua_Integer)act->removeObject(anim));
        return  1;
    }

    static  int Playback_count(lua_State* L)
    {
        auto    act     =   checkObj<FEPlayback>(L,1);
        lua_pushinteger(L,(lua_Integer)act->objects().size());
        return  1;
    }

    static  const luaL_Reg  PlaybackMethods[]  =
    {
        {   "play",         Playback_play           },
        {   "stop",         Playback_stop           },
        {   "setMode",      Playback_setMode        },
        {   "status",       Playback_status         },
        {   "setClipTime",  Playback_setClipTime    },
        {   "buildCache",   Playback_buildCache     },
        {   "clearCache",   Playback_clearCache     },
        {   "isCacheMode",  Playback_isCacheMode    },
        {   "refreshRange", Playback_refreshRange   },
        {   "addAnimation", Playback_addAnimation   },
        {   "removeAnimation",  Playback_removeAnimation },
        {   "count",        Playback_count          },
        {   nullptr,        nullptr                 },
    };

    // ==================== AnimationSys ====================

    /// <summary>
    /// sys:getOrCreate([name]) 获取或创建播放器
    /// </summary>
    static  int AnimSys_getOrCreate(lua_State* L)
    {
        auto    sys     =   checkObj<FEAnimationSys>(L,1);
        String  name    =   lua_isstring(L,2) ? lua_tostring(L,2) : "default";
        pushObj<FEPlayback>(L,sys->getOrCreate(name));
        return  1;
    }

    static  int AnimSys_count(lua_State* L)
    {
        auto    sys     =   checkObj<FEAnimationSys>(L,1);
        lua_pushinteger(L,(lua_Integer)sys->actions().size());
        return  1;
    }

    static  int AnimSys_clear(lua_State* L)
    {
        auto    sys     =   checkObj<FEAnimationSys>(L,1);
        sys->clear();
        return  0;
    }

    static  const luaL_Reg  AnimSysMethods[]   =
    {
        {   "getOrCreate",  AnimSys_getOrCreate },
        {   "count",        AnimSys_count       },
        {   "clear",        AnimSys_clear       },
        {   nullptr,        nullptr             },
    };

    // ==================== fe.anim 模块函数 ====================

    static  int AnimModule_createAnimation(lua_State* L)
    {
        auto*   env     =   getEnv(L);
        Animation   anim    =   new FEAnimation(*env->ctx);
        pushObj<FEAnimation>(L,anim);
        return  1;
    }

    static  int AnimModule_createNodeAnimation(lua_State* L)
    {
        auto*   env     =   getEnv(L);
        NodeProperyBits bits(NP_Default);
        if (lua_isnumber(L,1))
            bits    =   NodeProperyBits((uint32)lua_tointeger(L,1));
        pushObj<FEAnimation>(L,FEAnimationHelper::createNodeAnimtion(*env->ctx,bits));
        return  1;
    }

    static  int AnimModule_createClip(lua_State* L)
    {
        auto*   env     =   getEnv(L);
        AnimClip    clip    =   new FEAnimClip(*env->ctx);
        pushObj<FEAnimClip>(L,clip);
        return  1;
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
    static  int AnimModule_createTrack(lua_State* L)
    {
        auto*   env         =   getEnv(L);
        auto    propIdx     =   (PropIndex)luaL_checkinteger(L,1);
        const char* type    =   luaL_checkstring(L,2);
        bool    ok          =   false;
        auto    valueObj    =   makeValueObject(*env->ctx,type,ok);
        if (!ok)
            return  luaL_error(L,"createTrack: unknown value type '%s'",type);
        KeyFrameTrack   track   =   new FEKeyFrameTrack(*env->ctx,propIdx);
        track->setTimeObject(RealsObject(new FERealsObject(*env->ctx)));
        track->setValueObject(valueObj);
        pushObj<FEKeyFrameTrack>(L,track);
        return  1;
    }

    /// <summary>
    /// action([name]):取场景动画系统中的播放器(即 AnimationSys::getOrCreate)
    /// </summary>
    static  int AnimModule_action(lua_State* L)
    {
        auto*   env     =   getEnv(L);
        if (env->scene == nullptr)
            return  luaL_error(L,"fe.anim.action: scene not set");
        auto*   sys     =   env->scene->animationSystem();
        if (sys == nullptr)
            return  luaL_error(L,"fe.anim.action: animation system not ready");
        String  name    =   lua_isstring(L,1) ? lua_tostring(L,1) : "default";
        pushObj<FEPlayback>(L,sys->getOrCreate(name));
        return  1;
    }

    static  int AnimModule_addKeyFrameFromNode(lua_State* L)
    {
        auto    anim    =   checkObj<FEAnimation>(L,1);
        real    time    =   luaL_checknumber(L,2);
        auto    node    =   checkObj<FENode>(L,3);
        NodeProperyBits bits(0xFFFFFFFF);
        if (lua_isnumber(L,4))
            bits    =   NodeProperyBits((uint32)lua_tointeger(L,4));
        lua_pushboolean(L,FEAnimationHelper::addNodeKeyFrame(anim,time,node,bits) ? 1 : 0);
        return  1;
    }

    static  int AnimModule_updateKeyFrameFromNode(lua_State* L)
    {
        auto    anim    =   checkObj<FEAnimation>(L,1);
        real    time    =   luaL_checknumber(L,2);
        auto    node    =   checkObj<FENode>(L,3);
        NodeProperyBits bits(0xFFFFFFFF);
        if (lua_isnumber(L,4))
            bits    =   NodeProperyBits((uint32)lua_tointeger(L,4));
        lua_pushboolean(L,FEAnimationHelper::updateNodeKeyFrame(anim,time,node,bits) ? 1 : 0);
        return  1;
    }

    static  int AnimModule_removeKeyFrameFromNode(lua_State* L)
    {
        auto    anim    =   checkObj<FEAnimation>(L,1);
        real    time    =   luaL_checknumber(L,2);
        lua_pushboolean(L,FEAnimationHelper::removeNodeKeyFrame(anim,time) ? 1 : 0);
        return  1;
    }

    static  const luaL_Reg  AnimModuleMethods[]    =
    {
        {   "createAnimation",          AnimModule_createAnimation          },
        {   "createNodeAnimation",      AnimModule_createNodeAnimation      },
        {   "createClip",               AnimModule_createClip               },
        {   "createTrack",              AnimModule_createTrack              },
        {   "action",                   AnimModule_action                   },
        {   "addKeyFrameFromNode",      AnimModule_addKeyFrameFromNode      },
        {   "updateKeyFrameFromNode",   AnimModule_updateKeyFrameFromNode   },
        {   "removeKeyFrameFromNode",   AnimModule_removeKeyFrameFromNode   },
        {   nullptr,                    nullptr                             },
    };

    /// <summary>
    /// 向栈顶的 fe 表写入 anim 子模块(函数+常量),并注册各对象元表
    /// </summary>
    void    FELuaRegisterAnim(lua_State* L)
    {
        /// 注册对象元表
        registerType<FEAnimation>(L,AnimMethods);
        registerType<FEAnimClip>(L,ClipMethods);
        registerType<FEKeyFrameTrack>(L,TrackMethods);
        registerType<FEPlayback>(L,PlaybackMethods);
        registerType<FEAnimationSys>(L,AnimSysMethods);

        /// fe.anim 子表
        lua_newtable(L);
        luaL_setfuncs(L,AnimModuleMethods,0);

        /// 节点属性位(NP_*)
        lua_pushinteger(L,NP_TRANSFORM_X);      lua_setfield(L,-2,"NP_TRANSFORM_X");
        lua_pushinteger(L,NP_TRANSFORM_Y);      lua_setfield(L,-2,"NP_TRANSFORM_Y");
        lua_pushinteger(L,NP_TRANSFORM_Z);      lua_setfield(L,-2,"NP_TRANSFORM_Z");
        lua_pushinteger(L,NP_TRANSFORM_XYZ);    lua_setfield(L,-2,"NP_TRANSFORM_XYZ");
        lua_pushinteger(L,NP_SCALE_X);          lua_setfield(L,-2,"NP_SCALE_X");
        lua_pushinteger(L,NP_SCALE_Y);          lua_setfield(L,-2,"NP_SCALE_Y");
        lua_pushinteger(L,NP_SCALE_Z);          lua_setfield(L,-2,"NP_SCALE_Z");
        lua_pushinteger(L,NP_SCALE_XYZ);        lua_setfield(L,-2,"NP_SCALE_XYZ");
        lua_pushinteger(L,NP_ROTATE_X);         lua_setfield(L,-2,"NP_ROTATE_X");
        lua_pushinteger(L,NP_ROTATE_Y);         lua_setfield(L,-2,"NP_ROTATE_Y");
        lua_pushinteger(L,NP_ROTATE_Z);         lua_setfield(L,-2,"NP_ROTATE_Z");
        lua_pushinteger(L,NP_ROTATE_XYZ);       lua_setfield(L,-2,"NP_ROTATE_XYZ");
        lua_pushinteger(L,NP_QUAT);             lua_setfield(L,-2,"NP_QUAT");
        lua_pushinteger(L,NP_COLOR_RGB);        lua_setfield(L,-2,"NP_COLOR_RGB");
        lua_pushinteger(L,NP_COLOR_ALPHA);      lua_setfield(L,-2,"NP_COLOR_ALPHA");
        lua_pushinteger(L,NP_VISIBLE);          lua_setfield(L,-2,"NP_VISIBLE");
        lua_pushinteger(L,NP_Default);          lua_setfield(L,-2,"NP_Default");

        /// 属性索引(PROP_*)
        lua_pushinteger(L,PROP_TRANSFORM_X);        lua_setfield(L,-2,"PROP_TRANSFORM_X");
        lua_pushinteger(L,PROP_TRANSFORM_Y);        lua_setfield(L,-2,"PROP_TRANSFORM_Y");
        lua_pushinteger(L,PROP_TRANSFORM_Z);        lua_setfield(L,-2,"PROP_TRANSFORM_Z");
        lua_pushinteger(L,PROP_TRANSFORM_XYZ);      lua_setfield(L,-2,"PROP_TRANSFORM_XYZ");
        lua_pushinteger(L,PROP_SCALE_X);            lua_setfield(L,-2,"PROP_SCALE_X");
        lua_pushinteger(L,PROP_SCALE_Y);            lua_setfield(L,-2,"PROP_SCALE_Y");
        lua_pushinteger(L,PROP_SCALE_Z);            lua_setfield(L,-2,"PROP_SCALE_Z");
        lua_pushinteger(L,PROP_SCALE_XYZ);          lua_setfield(L,-2,"PROP_SCALE_XYZ");
        lua_pushinteger(L,PROP_ROTATE_X);           lua_setfield(L,-2,"PROP_ROTATE_X");
        lua_pushinteger(L,PROP_ROTATE_Y);           lua_setfield(L,-2,"PROP_ROTATE_Y");
        lua_pushinteger(L,PROP_ROTATE_Z);           lua_setfield(L,-2,"PROP_ROTATE_Z");
        lua_pushinteger(L,PROP_ROTATE_XYZ);         lua_setfield(L,-2,"PROP_ROTATE_XYZ");
        lua_pushinteger(L,PROP_QUAT);               lua_setfield(L,-2,"PROP_QUAT");
        lua_pushinteger(L,PROP_COLOR_RGB);          lua_setfield(L,-2,"PROP_COLOR_RGB");
        lua_pushinteger(L,PROP_COLOR_ALPHA);        lua_setfield(L,-2,"PROP_COLOR_ALPHA");
        lua_pushinteger(L,PROP_VISIBLE);            lua_setfield(L,-2,"PROP_VISIBLE");
        lua_pushinteger(L,PROP_MAT_DIFFUSE);        lua_setfield(L,-2,"PROP_MAT_DIFFUSE");
        lua_pushinteger(L,PROP_G_TRANSFORM_XYZ);    lua_setfield(L,-2,"PROP_G_TRANSFORM_XYZ");

        /// 插值方式
        lua_pushinteger(L,(lua_Integer)FEKeyFrameTrack::IT_Bezier);     lua_setfield(L,-2,"IT_BEZIER");
        lua_pushinteger(L,(lua_Integer)FEKeyFrameTrack::IT_Linear);     lua_setfield(L,-2,"IT_LINEAR");
        lua_pushinteger(L,(lua_Integer)FEKeyFrameTrack::IT_Constant);   lua_setfield(L,-2,"IT_CONSTANT");

        /// 播放模式与时间模式
        lua_pushinteger(L,(lua_Integer)FEPlayback::PT_Once);        lua_setfield(L,-2,"PT_ONCE");
        lua_pushinteger(L,(lua_Integer)FEPlayback::PT_Loop);        lua_setfield(L,-2,"PT_LOOP");
        lua_pushinteger(L,(lua_Integer)FEPlayback::PT_PingPong);    lua_setfield(L,-2,"PT_PINGPONG");
        lua_pushinteger(L,(lua_Integer)FEPlayback::TM_Default);     lua_setfield(L,-2,"TM_DEFAULT");
        lua_pushinteger(L,(lua_Integer)FEPlayback::TM_Invert);      lua_setfield(L,-2,"TM_INVERT");

        lua_setfield(L,-2,"anim");
    }
}
