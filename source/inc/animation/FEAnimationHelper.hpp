#pragma     once
#include    <functional>
#include    <format>
#include    "../node/FENode.hpp"
#include    "../material/FEMaterialLibrary.hpp"
#include    "../FEPropertyIndex.hpp"
#include    "FEAnimation.hpp"
#include    "FEAnimClip.hpp"
#include    "FEKeyFrameTrack.hpp"
#include    "FERealsObject.hpp"
#include    "FEReal3sObject.hpp"
#include    "FEQuatrsObject.hpp"
#include    "FEColorObject.hpp"
#include    "FEAlphaObject.hpp"
#include    "FEBoolsObject.hpp"
#include    "../FEEulerObject.hpp"


namespace FE
{
   
    class   FE_API  FEAnimationHelper : public FEComponent
    {
    public:
        /// <summary>
        /// 为节点创建动画对象(含clip),根据prop标志创建对应的轨道
        /// 轨道初始为空(无关键帧),由调用方后续填充
        /// </summary>
        /// <param name="_ctx">上下文</param>
        /// <param name="node">目标节点,动画将作为组件添加到该节点</param>
        /// <param name="prop">需要创建的轨道属性位掩码,默认位置+缩放+旋转</param>
        /// <returns>创建的动画对象</returns>
        static  Animation   createNodeAnimtion(FEContext& _ctx,NodeProperyBits prop = NP_Default)
        {
            auto    anim    =   new FEAnimation(_ctx);
            auto    clip    =   new FEAnimClip(_ctx); 
            anim->setClip(clip);
            anim->setName("Animation");

            /// 创建共享的时间线(空),由各轨道引用
            /// 注意:track共享同一时间线对象时,FEAction::updateBatch 会做相同时间线优化
            /// 但编辑场景下,每条track独立时间线更直观,这里为每条track创建独立时间线
            auto    makeReal3Track = [&](PropIndex idx,const char* name)
            {
                KeyFrameTrack   track   =   new FEKeyFrameTrack(_ctx,idx);
                RealsObject     times   =   new FERealsObject(_ctx);
                Real3sObject    values  =   new FEReal3sObject(_ctx);
                track->setName(name);
                track->setTimeObject(times);
                track->setValueObject(values);
                clip->addTrack(track);
            };
            auto    makeRealTrack = [&](PropIndex idx,const char* name)
            {
                KeyFrameTrack   track   =   new FEKeyFrameTrack(_ctx,idx);
                RealsObject     times   =   new FERealsObject(_ctx);
                RealsObject     values  =   new FERealsObject(_ctx);
                track->setName(name);
                track->setTimeObject(times);
                track->setValueObject(values);
                clip->addTrack(track);
            };

            /// 位置轨道
            if (prop.hasFlag(NP_TRANSFORM_XYZ))
                makeReal3Track(PROP_TRANSFORM_XYZ,"Position");
            else
            {
                if (prop.hasFlag(NP_TRANSFORM_X))    makeRealTrack(PROP_TRANSFORM_X,"Position.X");
                if (prop.hasFlag(NP_TRANSFORM_Y))    makeRealTrack(PROP_TRANSFORM_Y,"Position.Y");
                if (prop.hasFlag(NP_TRANSFORM_Z))    makeRealTrack(PROP_TRANSFORM_Z,"Position.Z");
            }

            /// 缩放轨道
            if (prop.hasFlag(NP_SCALE_XYZ))
                makeReal3Track(PROP_SCALE_XYZ,"Scale");
            else
            {
                if (prop.hasFlag(NP_SCALE_X))    makeRealTrack(PROP_SCALE_X,"Scale.X");
                if (prop.hasFlag(NP_SCALE_Y))    makeRealTrack(PROP_SCALE_Y,"Scale.Y");
                if (prop.hasFlag(NP_SCALE_Z))    makeRealTrack(PROP_SCALE_Z,"Scale.Z");
            }

            /// 旋转轨道(欧拉角,单位角度)
            /// NP_ROTATE_XYZ 创建三条独立轨道(X/Y/Z),使用轴角方式
            if (prop.hasFlag(NP_ROTATE_XYZ))
            {
                makeReal3Track(PROP_ROTATE_XYZ,"Rotation.XYZ");
            }
            else
            {
                if (prop.hasFlag(NP_ROTATE_X))    makeRealTrack(PROP_ROTATE_X,"Rotation.X");
                if (prop.hasFlag(NP_ROTATE_Y))    makeRealTrack(PROP_ROTATE_Y,"Rotation.Y");
                if (prop.hasFlag(NP_ROTATE_Z))    makeRealTrack(PROP_ROTATE_Z,"Rotation.Z");
            }

            /// 四元数轨道(整旋转)
            if (prop.hasFlag(NP_QUAT))
            {
                KeyFrameTrack   track   =   new FEKeyFrameTrack(_ctx,PROP_QUAT);
                RealsObject     times   =   new FERealsObject(_ctx);
                QuatrsObject    values  =   new FEQuatrsObject(_ctx);
                track->setName("Rotation");
                track->setTimeObject(times);
                track->setValueObject(values);
                clip->addTrack(track);
            }

            /// 颜色轨道(RGBA, uint8x4)
            if (prop.hasFlag(NP_COLOR_RGB))
            {
                KeyFrameTrack   track   =   new FEKeyFrameTrack(_ctx,PROP_COLOR_RGB);
                RealsObject     times   =   new FERealsObject(_ctx);
                RgbaObject      values  =   new FEColorObject(_ctx);
                track->setName("Color");
                track->setTimeObject(times);
                track->setValueObject(values);
                clip->addTrack(track);
            }

            /// 透明度轨道(uint8)
            if (prop.hasFlag(NP_COLOR_ALPHA))
            {
                KeyFrameTrack   track   =   new FEKeyFrameTrack(_ctx,PROP_COLOR_ALPHA);
                RealsObject     times   =   new FERealsObject(_ctx);
                AlphaObject     values  =   new FEAlphaObject(_ctx);
                track->setName("Alpha");
                track->setTimeObject(times);
                track->setValueObject(values);
                clip->addTrack(track);
            }

            /// 可见性轨道(bool -> uint8)
            if (prop.hasFlag(NP_VISIBLE))
            {
                KeyFrameTrack   track   =   new FEKeyFrameTrack(_ctx,PROP_VISIBLE);
                RealsObject     times   =   new FERealsObject(_ctx);
                BoolsObject     values  =   new FEBoolsObject(_ctx);
                track->setName("Visible");
                track->setTimeObject(times);
                track->setValueObject(values);
                clip->addTrack(track);
            }
            return  anim;
        }
        /// <summary>
        /// 动画中添加关键帧数据,time 时间，如果时间点存在,返回失败,propBits 指定哪些属性轨道需要添加关键帧
        /// 动画数据来自node,如果anim 如果没有对应的属性track，即使 propBits 指定，也不需要添加
        /// 如果anim:clip 中没有 track,则返回false
        /// </summary>
        /// <param name="anim"></param>
        /// <param name="time"></param>
        /// <param name="node">节点对象,数据提供者</param>
        /// <param name="prop"></param>
        /// <returns></returns>
        static  bool        addNodeKeyFrame(Animation anim,real time,Node node,NodeProperyBits propBits = 0xFFFFFFFF)
        {
            if (anim == nullptr || node == nullptr)
                return  false;
            auto    clip    =   anim->clip();
            if (clip == nullptr || clip->tracks().empty())
                return  false;

            /// 分解 node 的 transform
            real3   pos,scale;
            quatr   rot;
            decompose<real>(node->globalTransform(),pos,scale,rot);
            auto    eulerRad    =   quatToEuler(rot);
            real3   eulerDeg(RAD2DEG(eulerRad.x),RAD2DEG(eulerRad.y),RAD2DEG(eulerRad.z));

            /// 颜色/透明度/可见性
            auto    nodeColor   =   node->color();
            uint8x4 colorVal    =   nodeColor.value();
            uint8   alphaVal    =   colorVal.w;
            uint8   visibleVal  =   node->flags().hasFlag(FLAG_VISIBLE) ? 1 : 0;

            /// 将属性索引映射到位标记,判断是否需要处理该轨道
            auto    propToBit = [](PropIndex idx) -> uint32
            {
                switch (idx)
                {
                case    PROP_TRANSFORM_X:    return  NP_TRANSFORM_X;
                case    PROP_TRANSFORM_Y:    return  NP_TRANSFORM_Y;
                case    PROP_TRANSFORM_Z:    return  NP_TRANSFORM_Z;
                case    PROP_TRANSFORM_XYZ:  return  NP_TRANSFORM_XYZ;
                case    PROP_SCALE_X:        return  NP_SCALE_X;
                case    PROP_SCALE_Y:        return  NP_SCALE_Y;
                case    PROP_SCALE_Z:        return  NP_SCALE_Z;
                case    PROP_SCALE_XYZ:      return  NP_SCALE_XYZ;
                case    PROP_ROTATE_X:       return  NP_ROTATE_X;
                case    PROP_ROTATE_Y:       return  NP_ROTATE_Y;
                case    PROP_ROTATE_Z:       return  NP_ROTATE_Z;
                case    PROP_ROTATE_XYZ:     return  NP_ROTATE_XYZ;
                case    PROP_QUAT:           return  NP_QUAT;
                case    PROP_COLOR_RGB:      return  NP_COLOR_RGB;
                case    PROP_COLOR_ALPHA:    return  NP_COLOR_ALPHA;
                case    PROP_VISIBLE:        return  NP_VISIBLE;
                default:                     return  0;
                }
            };

            for (auto track : clip->tracks())
            {
                PropIndex   propIdx =   track->propertyIndex();
                uint32      bit     =   propToBit(propIdx);
                /// 该属性不在 propBits 中,跳过
                if (bit == 0 || !propBits.hasFlag((NodeProperyBit)bit))
                    continue;

                /// 时间点已存在,返回失败(跳过该轨道)
                if (track->_times)
                {
                    auto&   times   =   track->_times->values();
                    if (std::find(times.begin(),times.end(),time) != times.end())
                        continue;
                }

                /// 确保时间对象存在
                if (!track->_times)
                    track->setTimeObject(RealsObject(new FERealsObject(node->ctx())));
                track->_times->values().push_back(time);

                /// 根据属性索引写入对应的值
                auto    valIdx  =   track->_values.index();
                switch (propIdx)
                {
                case    PROP_TRANSFORM_X:
                    if (valIdx == 1) std::get<1>(track->_values)->values().push_back(pos.x);
                    break;
                case    PROP_TRANSFORM_Y:
                    if (valIdx == 1) std::get<1>(track->_values)->values().push_back(pos.y);
                    break;
                case    PROP_TRANSFORM_Z:
                    if (valIdx == 1) std::get<1>(track->_values)->values().push_back(pos.z);
                    break;
                case    PROP_TRANSFORM_XYZ:
                    if (valIdx == 3) std::get<3>(track->_values)->values().push_back(pos);
                    break;
                case    PROP_SCALE_X:
                    if (valIdx == 1) std::get<1>(track->_values)->values().push_back(scale.x);
                    break;
                case    PROP_SCALE_Y:
                    if (valIdx == 1) std::get<1>(track->_values)->values().push_back(scale.y);
                    break;
                case    PROP_SCALE_Z:
                    if (valIdx == 1) std::get<1>(track->_values)->values().push_back(scale.z);
                    break;
                case    PROP_SCALE_XYZ:
                    if (valIdx == 3) std::get<3>(track->_values)->values().push_back(scale);
                    break;
                case    PROP_ROTATE_X:
                    if (valIdx == 1) std::get<1>(track->_values)->values().push_back(eulerDeg.x);
                    break;
                case    PROP_ROTATE_Y:
                    if (valIdx == 1) std::get<1>(track->_values)->values().push_back(eulerDeg.y);
                    break;
                case    PROP_ROTATE_Z:
                    if (valIdx == 1) std::get<1>(track->_values)->values().push_back(eulerDeg.z);
                    break;
                case    PROP_ROTATE_XYZ:
                    if (valIdx == 3) std::get<3>(track->_values)->values().push_back(eulerDeg);
                    break;
                case    PROP_QUAT:
                    if (valIdx == 5) std::get<5>(track->_values)->values().push_back(rot);
                    break;
                case    PROP_COLOR_RGB:
                    if (valIdx == 12) std::get<12>(track->_values)->values().push_back(colorVal);
                    break;
                case    PROP_COLOR_ALPHA:
                    if (valIdx == 13) std::get<13>(track->_values)->values().push_back(alphaVal);
                    break;
                case    PROP_VISIBLE:
                    if (valIdx == 11) std::get<11>(track->_values)->values().push_back(visibleVal);
                    break;
                default:
                    break;
                }

                track->sortKeyFames();
                track->flags().addFlag(FEKeyFrameTrack::TrackChanged);
            }
            return  true;
        }
        /// <summary>
        /// 移除指定时间点的关键帧
        /// 遍历anim的所有track,移除时间等于time的关键帧数据
        /// </summary>
        /// <param name="anim">动画对象</param>
        /// <param name="time">关键帧时间</param>
        /// <returns>是否有任意track移除了关键帧</returns>
        static  bool        removeNodeKeyFrame(Animation anim,real time)
        {
            if (anim == nullptr)
                return  false;
            auto    clip    =   anim->clip();
            if (clip == nullptr)
                return  false;

            bool    bRemoved    =   false;
            for (auto track : clip->tracks())
            {
                if (!track->_times)
                    continue;
                auto&   times   =   track->_times->values();
                auto    itr     =   std::find(times.begin(),times.end(),time);
                if (itr == times.end())
                    continue;
                size_t  idx =   (size_t)std::distance(times.begin(),itr);

                /// 同步移除 times 和 values 中对应索引的元素
                times.erase(times.begin() + idx);
                auto    valIdx  =   track->_values.index();
                switch (valIdx)
                {
                case 1:     std::get<1>(track->_values)->values().erase(std::get<1>(track->_values)->values().begin() + idx);    break;
                case 2:     std::get<2>(track->_values)->values().erase(std::get<2>(track->_values)->values().begin() + idx);    break;
                case 3:     std::get<3>(track->_values)->values().erase(std::get<3>(track->_values)->values().begin() + idx);    break;
                case 4:     std::get<4>(track->_values)->values().erase(std::get<4>(track->_values)->values().begin() + idx);    break;
                case 5:     std::get<5>(track->_values)->values().erase(std::get<5>(track->_values)->values().begin() + idx);    break;
                case 6:     std::get<6>(track->_values)->values().erase(std::get<6>(track->_values)->values().begin() + idx);    break;
                case 7:     std::get<7>(track->_values)->values().erase(std::get<7>(track->_values)->values().begin() + idx);    break;
                case 8:     std::get<8>(track->_values)->values().erase(std::get<8>(track->_values)->values().begin() + idx);    break;
                case 9:     std::get<9>(track->_values)->values().erase(std::get<9>(track->_values)->values().begin() + idx);    break;
                case 10:    std::get<10>(track->_values)->values().erase(std::get<10>(track->_values)->values().begin() + idx);  break;
                case 11:    std::get<11>(track->_values)->values().erase(std::get<11>(track->_values)->values().begin() + idx);  break;
                case 12:    std::get<12>(track->_values)->values().erase(std::get<12>(track->_values)->values().begin() + idx);  break;
                case 13:    std::get<13>(track->_values)->values().erase(std::get<13>(track->_values)->values().begin() + idx);  break;
                default:    break;
                }
                track->flags().addFlag(FEKeyFrameTrack::TrackChanged);
                bRemoved    =   true;
            }
            return  bRemoved;
        }
        /// <summary>
        /// 更新指定时间点的关键帧数据,数据来自node
        /// 如果该时间点不存在,返回false;存在则用node当前属性覆盖
        /// propBits 指定哪些属性轨道需要更新
        /// </summary>
        /// <param name="anim">动画对象</param>
        /// <param name="time">关键帧时间</param>
        /// <param name="node">节点对象,数据提供者</param>
        /// <param name="propBits">需要更新的属性位掩码,默认全部</param>
        /// <returns>是否有任意track更新了关键帧</returns>
        static  bool        updateNodeKeyFrame(Animation anim,real time,Node node,NodeProperyBits propBits = 0xFFFFFFFF)
        {
            if (anim == nullptr || node == nullptr)
                return  false;
            auto    clip    =   anim->clip();
            if (clip == nullptr || clip->tracks().empty())
                return  false;

            /// 分解 node 的 transform
            real3   pos,scale;
            quatr   rot;
            decompose<real>(node->globalTransform(),pos,scale,rot);
            auto    eulerRad    =   quatToEuler(rot);
            real3   eulerDeg(RAD2DEG(eulerRad.x),RAD2DEG(eulerRad.y),RAD2DEG(eulerRad.z));

            /// 颜色/透明度/可见性
            auto    nodeColor   =   node->color();
            uint8x4 colorVal    =   nodeColor.value();
            uint8   alphaVal    =   colorVal.w;
            uint8   visibleVal  =   node->flags().hasFlag(FLAG_VISIBLE) ? 1 : 0;

            /// 属性索引到位标记映射
            auto    propToBit = [](PropIndex idx) -> uint32
            {
                switch (idx)
                {
                case    PROP_TRANSFORM_X:    return  NP_TRANSFORM_X;
                case    PROP_TRANSFORM_Y:    return  NP_TRANSFORM_Y;
                case    PROP_TRANSFORM_Z:    return  NP_TRANSFORM_Z;
                case    PROP_TRANSFORM_XYZ:  return  NP_TRANSFORM_XYZ;
                case    PROP_SCALE_X:        return  NP_SCALE_X;
                case    PROP_SCALE_Y:        return  NP_SCALE_Y;
                case    PROP_SCALE_Z:        return  NP_SCALE_Z;
                case    PROP_SCALE_XYZ:      return  NP_SCALE_XYZ;
                case    PROP_ROTATE_X:       return  NP_ROTATE_X;
                case    PROP_ROTATE_Y:       return  NP_ROTATE_Y;
                case    PROP_ROTATE_Z:       return  NP_ROTATE_Z;
                case    PROP_ROTATE_XYZ:     return  NP_ROTATE_XYZ;
                case    PROP_QUAT:           return  NP_QUAT;
                case    PROP_COLOR_RGB:      return  NP_COLOR_RGB;
                case    PROP_COLOR_ALPHA:    return  NP_COLOR_ALPHA;
                case    PROP_VISIBLE:        return  NP_VISIBLE;
                default:                     return  0;
                }
            };

            bool    bUpdated    =   false;
            for (auto track : clip->tracks())
            {
                PropIndex   propIdx =   track->propertyIndex();
                uint32      bit     =   propToBit(propIdx);
                if (bit == 0 || !propBits.hasFlag((NodeProperyBit)bit))
                    continue;

                /// 查找时间点,不存在则跳过
                if (!track->_times)
                    continue;
                auto&   times   =   track->_times->values();
                auto    itr     =   std::find(times.begin(),times.end(),time);
                if (itr == times.end())
                    continue;
                size_t  idx =   (size_t)std::distance(times.begin(),itr);

                /// 用 node 当前属性值覆盖该索引处的值
                auto    valIdx  =   track->_values.index();
                switch (propIdx)
                {
                case    PROP_TRANSFORM_X:
                    if (valIdx == 1) std::get<1>(track->_values)->values()[idx] = pos.x;
                    break;
                case    PROP_TRANSFORM_Y:
                    if (valIdx == 1) std::get<1>(track->_values)->values()[idx] = pos.y;
                    break;
                case    PROP_TRANSFORM_Z:
                    if (valIdx == 1) std::get<1>(track->_values)->values()[idx] = pos.z;
                    break;
                case    PROP_TRANSFORM_XYZ:
                    if (valIdx == 3) std::get<3>(track->_values)->values()[idx] = pos;
                    break;
                case    PROP_SCALE_X:
                    if (valIdx == 1) std::get<1>(track->_values)->values()[idx] = scale.x;
                    break;
                case    PROP_SCALE_Y:
                    if (valIdx == 1) std::get<1>(track->_values)->values()[idx] = scale.y;
                    break;
                case    PROP_SCALE_Z:
                    if (valIdx == 1) std::get<1>(track->_values)->values()[idx] = scale.z;
                    break;
                case    PROP_SCALE_XYZ:
                    if (valIdx == 3) std::get<3>(track->_values)->values()[idx] = scale;
                    break;
                case    PROP_ROTATE_X:
                    if (valIdx == 1) std::get<1>(track->_values)->values()[idx] = eulerDeg.x;
                    break;
                case    PROP_ROTATE_Y:
                    if (valIdx == 1) std::get<1>(track->_values)->values()[idx] = eulerDeg.y;
                    break;
                case    PROP_ROTATE_Z:
                    if (valIdx == 1) std::get<1>(track->_values)->values()[idx] = eulerDeg.z;
                    break;
                case    PROP_ROTATE_XYZ:
                    if (valIdx == 3) std::get<3>(track->_values)->values()[idx] = eulerDeg;
                    break;
                case    PROP_QUAT:
                    if (valIdx == 5) std::get<5>(track->_values)->values()[idx] = rot;
                    break;
                case    PROP_COLOR_RGB:
                    if (valIdx == 12) std::get<12>(track->_values)->values()[idx] = colorVal;
                    break;
                case    PROP_COLOR_ALPHA:
                    if (valIdx == 13) std::get<13>(track->_values)->values()[idx] = alphaVal;
                    break;
                case    PROP_VISIBLE:
                    if (valIdx == 11) std::get<11>(track->_values)->values()[idx] = visibleVal;
                    break;
                default:
                    break;
                }

                track->flags().addFlag(FEKeyFrameTrack::TrackChanged);
                bUpdated    =   true;
            }
            return  bUpdated;
        }
    };
}
