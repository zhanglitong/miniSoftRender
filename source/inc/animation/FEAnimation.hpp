#pragma     once
#include    <functional>
#include    <format>
#include    <vector>
#include    "../FETransform.hpp"
#include    "../FEComponent.hpp"
#include    "FEAnimClip.hpp"

namespace FE
{
    DEFINE_CLASS_UUID(FEAnimation, "{D0749EE9-7126-4A57-B4F2-84798E4F40F2}");

    /// <summary>
    /// 对象中包含需要被动画的对象
    /// 以及clip信息，clip 动画片段，包含实际的动画关键帧组数据
    /// 可以实现动画数据与动画对象分离,让动画数据有复用价值
    /// 典型应用：一个对象，在时间线上，可以断开
    /// 一个节点对象可以有多个动画
    /// </summary>
    
    class   FE_API  FEAnimation : public FEComponent
    {
    public:
        enum    ClipFlag
        {
            /// <summary>
            /// 标记数据变更
            /// </summary>
            AnimationChanged    =   (FE::FLAG_LAST),
            /// <summary>
            /// 标记在update函数中标记，标记当前帧是否有效
            /// 决定在transform 是否应该被应用到owner上
            /// </summary>
            OutOfRangeBit       =   (AnimationChanged<<1),
        };
    public:
        IMPLEMENT_CLASS_REFLECT(FEAnimation)
    public:
        FEAnimation(FEContext& ctx)  ;
        FEAnimation(const FEAnimation& other);
        ~FEAnimation();
    public:
        const   auto&   name() const
        {
            return  _name;
        }
        inline  void    setName(const String& name)
        {
            _name   =   name;
        }
        /// <summary>
        /// 关联所有者
        /// </summary>
        /// <param name="owner"></param>
        virtual void    attach(Object owner) override
        {
            _owner      =   owner;
            flags().addFlag(AnimationChanged);
        }
        /// <summary>
        /// 取消关联
        /// </summary>
        virtual void    detach() override
        {
            _owner      =   nullptr;
            flags().addFlag(AnimationChanged);
        }
        /// <summary>
        /// 是否发生变更
        /// </summary>
        /// <returns></returns>
        inline  bool    isChanged() const
        {
            if (flags().hasFlag(AnimationChanged))
                return  true;
            else if(_clip)
                return  _clip->isChanged();
            else
                return  false;
        }
        /// <summary>
        /// 获取起始时间,对于时间线
        /// </summary>
        /// <returns></returns>
        inline  real    offset() const
        {
            return  _offset;
        }
        /// <summary>
        /// 设置起始播放时间,相对于时间线
        /// </summary>
        /// <param name="offset"></param>
        inline  void    setOffset(const real& offset)
        {
            _offset =   offset;
            flags().addFlag(AnimationChanged);
        }
        /// <summary>
        /// 获取范围,时间播放的范围
        /// </summary>
        /// <returns></returns>
        inline  real2   range() const
        {
            if (_clip == nullptr)
                return  real2();
            else
                return  _clip->range() + real2(_offset,_offset);
        }
        /// <summary>
        /// 是否有效
        /// </summary>
        /// <returns></returns>
        inline  bool    isValid() const
        {
            return  _clip != nullptr &&  owner() != nullptr;
        }
        /// <summary>
        /// 当前是否仍向对象贡献属性:组件启用、clip 与 owner 有效且时间未越界
        /// </summary>
        /// <returns></returns>
        inline  bool    isContributing() const
        {
            return  isEnable() && isValid() && !outOfRange();
        }
        /// <summary>
        /// update 函数计算完成后，会设置标记
        /// </summary>
        /// <returns></returns>
        inline  bool   outOfRange() const
        {
            return  flags().hasFlag(OutOfRangeBit);
        }
        inline  auto    clip() const
        {
            return  _clip;
        }
        inline  void    setClip(AnimClip clip)
        {
            _clip   =   clip;
            flags().addFlag(AnimationChanged);
        }
        inline  void    clearChanged() 
        {
            flags().removeFlag(AnimationChanged);
        }
        /// <summary>
        /// 返回子对象个数，用于树形访问遍历使用
        /// 配合traverseObject使用
        /// </summary>
        /// <returns></returns>
        virtual size_t  objectCount() const override;
        virtual bool    traverseObject(const ObjectVisitor&,uint depth = 0,bool recur = false) const override;
        /// <summary>
        /// 组件每一帧更新
        /// 返回值表示，是否已经修改了 _owner
        /// 如果修改了 返回true,没有修改返回false
        /// </summary>
        /// <param name="deltaTm"></param>
        /// <returns>true/false</returns>
        virtual bool    update(const real& clipTime) override;
        /// <summary>
        /// 动画系统独有接口
        /// 通用设置对象属性接口，子类实现
        /// </summary>
        virtual void    beginSetProp() override;
        /// <summary>
        /// 动画系统独有接口
        /// 设置属性:直接写 owner(任意 FEObject),动画自身不保存副本
        /// </summary>
        /// <param name="prop">属性索引(别名)</param>
        /// <param name="value">属性值</param>
        /// <returns>true,表示修改成功,否则没有修改</returns>
        virtual bool    setProperty(int prop,const KFValue& value) override;

        /// <summary>
        /// 获取属性值
        /// 一律从 _owner 读取
        /// </summary>
        /// <param name="prop">属性索引</param>
        /// <returns>属性值</returns>
        virtual KFValue getProperty(int prop) const override;

        /// <summary>
        /// 动画系统独有接口
        /// @ref setProperty 返回结果作为输入参数，用来决定是否需要更新操作
        /// </summary>
        /// <param name="bModify"></param>
        virtual void    endSetProp(const real&,bool bModify) override;
    protected:
        /// <summary>
        /// 判断属性是否属于变换相关属性(位置/旋转/缩放)
        /// </summary>
        /// <param name="prop"></param>
        /// <returns></returns>
        static  bool    isTransformProperty(int prop);
        /// <summary>
        /// 把本次采样结果通过属性接口写回 owner(与约束求解一致,对象不再反向依赖组件)
        /// 轴心由 owner 自己持有,这里只传位置/缩放/旋转
        /// </summary>
        /// <param name="tf">本次采样得到的本地变换</param>
        void    applyTransform(const FETransform& tf);
        /// <summary>
        /// 一次遍历分析 clip 的变换轨道:
        /// bRotation/bPosition 表示是否存在带关键帧的旋转/位置轨道,
        /// segTrack 返回作为轴心圆弧段边界的关键帧轨道(优先整体位置轨道)
        /// </summary>
        void    analyzeTracks(bool& bRotation,bool& bPosition,const FEKeyFrameTrack*& segTrack) const;
        /// <summary>
        /// 在指定剪辑时间采样一组变换通道,结果写入 out(未驱动的通道保持 out 原值)
        /// </summary>
        bool    sampleTransformAt(real localTime,FETransform& out);
        /// <summary>
        /// 获取依赖的对象,子类实现
        /// </summary>
        /// <param name="uset"></param>
        /// <returns>返回以来的对象个数</returns>
        virtual size_t  queryDepends(ObjectUSet& uSet) const override;
        /// <summary>
        /// 子类实现
        /// </summary>
        /// <param name="writer"></param>
        /// <param name="chunk">数据头，子类可根据情况修改(flags字段)，实现一些优化处理</param>
        /// <param name="version">版本号</param>
        /// <param name="ctx">上下文对象</param>
        /// <returns></returns>
        virtual void    serializeTraits(FEWriter& writer,FEChunkInf& chunk,uint version,FESerializeCtx& ctx) const override;
        /// <summary>
        /// 子类实现,只关注自己需要读取的数据
        /// </summary>
        /// <param name="reader"></param>
        /// <param name="chunk">数据头，子类可根据chunk._flags字段控制读取</param>
        /// <param name="ctx"></param>
        /// <returns></returns>
        virtual void    deserializeTraits(FEReader& reader,const FEChunkInf& chunk,uint version,FESerializeCtx& ctx) override;
    protected:
        /// <summary>
        /// AnimClip 类型的变量，用于保存动画剪辑。
        /// </summary>
        AnimClip            _clip;
        /// <summary>
        /// 动画在大时间线上的起始时间，标记了动画从什么时间开始播放
        /// </summary>
        real                _offset     =   0;
        /// <summary>
        /// 记录动画播放到哪里了
        /// </summary>
        real                _clipTime   =   0;
        TrackResults        _results; 
        String              _name;
    };

    using   Animation       =   SharedPtr<FEAnimation>;
    using   Animations      =   std::vector<Animation>;
}
