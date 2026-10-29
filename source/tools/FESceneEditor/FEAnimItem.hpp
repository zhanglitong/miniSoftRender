/// <summary>
/// FEAnimItem
/// 动画树的一行只保存"行类型 + 对象":节点/动画/轨道/播放器都是 FEObject,
/// 用统一的 Object 引用保存,避免每种行类型各留一个引用成员。
/// </summary>

#pragma     once

#include    "../../../inc/animation/FEPlayback.hpp"
#include    "../../../inc/animation/FEAnimation.hpp"
#include    "../../../inc/animation/FEKeyFrameTrack.hpp"
#include    "../../../inc/node/FENode.hpp"

#include    <vector>

namespace   FE
{
    class   FEAnimItem
    {
    public:
        /// <summary>
        /// 行类型:播放器 / 节点 / 动画 / 轨道
        /// </summary>
        enum    class   Type
        {
            Playback,
            Node,
            Animation,
            Track,
        };
    public:
        FEAnimItem() = default;
        /// <summary>
        /// 由任意 FEObject 派生类型(节点/动画/轨道/播放器)构造,统一保存为 Object
        /// </summary>
        template<typename T>
        FEAnimItem(Type type,const FERef<T>& object)
            :_type(type)
            ,_object(const_cast<T*>(object.get()))
        {
        }
        Type        type()  const
        {
            return  _type;
        }
        Object      object() const
        {
            return  _object;
        }
        /// <summary>
        /// 播放器(行类型不是播放器时返回空)
        /// </summary>
        Playback    playback() const
        {
            return  is(Type::Playback) ? castObject<FEPlayback>() : Playback();
        }
        /// <summary>
        /// 节点(行类型不是节点时返回空)
        /// </summary>
        Node        node() const
        {
            return  is(Type::Node) ? castObject<FENode>() : Node();
        }
        /// <summary>
        /// 动画(行类型不是动画时返回空)
        /// </summary>
        Animation   anim() const
        {
            return  is(Type::Animation) ? castObject<FEAnimation>() : Animation();
        }
        /// <summary>
        /// 轨道(行类型不是轨道时返回空)
        /// </summary>
        auto        track() const
        {
            return  is(Type::Track) ? castObject<FEKeyFrameTrack>() : KeyFrameTrack();
        }
    protected:
        /// <summary>
        /// 按行类型把对象转换为派生类型
        /// </summary>
        template<typename T>
        FERef<T>    castObject() const
        {
            if (!_object)
                return  FERef<T>();
            FEObject*   raw =   const_cast<FEObject*>(_object.get());
            return  FERef<T>(raw->cast<T>());
        }
    protected:
        /// <summary>
        /// 行类型与类型是否一致(且对象有效)
        /// </summary>
        bool    is(Type type) const
        {
            return  _type == type && _object;
        }
    protected:
        /// <summary>
        /// 行类型
        /// </summary>
        Type            _type   =   Type::Track;
        /// <summary>
        /// 行对应的对象
        /// </summary>
        Object          _object;
    };
    /// <summary>
    /// 动画树行列表
    /// </summary>
    using   AnimItems   =   std::vector<FEAnimItem>;
}
