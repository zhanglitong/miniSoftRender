#pragma     once
#include    <functional>
#include    <format>
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
            AnimationChanged    =   (FLAG_ACTOR <<1),
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
        /// 返回节点自身的transform;不含组件信息
        /// </summary>
        /// <returns></returns>
        inline  auto&   transform()
        {
            return  _transform;
        }
        const   auto&   transform() const
        {
            return  _transform;
        }
        /// <summary>
        /// 关联所有者
        /// </summary>
        /// <param name="owner"></param>
        virtual void    attach(Object owner) override
        {
            _owner  =   owner;
            flags().addFlag(AnimationChanged);
        }
        /// <summary>
        /// 取消关键
        /// </summary>
        virtual void    detach() override
        {
            _owner  =   nullptr;
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
        /// 组件会把自己的变换数据应用到 global上
        /// </summary>
        /// <param name="parent"></param>
        /// <param name=""></param>
        virtual void    appTransform(mat4r& global) override
        {
            global  =   global * _transform.toMatrix();
        }
        /// <summary>
        /// 应用一条轨道的采样结果
        /// 变换属性写入自身 _transform,其他属性写入 _owner
        /// </summary>
        /// <param name="prop">属性索引</param>
        /// <param name="value">采样值</param>
        /// <returns>是否修改了变换(true 表示需要触发节点 updateTransform)</returns>
        bool    applyTrackResult(int prop,const KFValue& value);
        /// <summary>
        /// 获取属性值
        /// 变换属性从自身 _transform 读取,其他属性从 _owner 读取
        /// </summary>
        /// <param name="prop">属性索引</param>
        /// <returns>属性值</returns>
        virtual KFValue     getProperty(int prop) const override;
    protected:
        /// <summary>
        /// 判断属性是否属于变换相关属性(位置/旋转/缩放)
        /// </summary>
        /// <param name="prop"></param>
        /// <returns></returns>
        static  bool    isTransformProperty(int prop);
        /// <summary>
        /// 将变换相关属性写入 _transform
        /// </summary>
        /// <param name="prop"></param>
        /// <param name="value"></param>
        void    setTransformProperty(int prop,const KFValue& value);
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
        /// <summary>
        /// 根据类型id获取接口信息
        /// </summary>
        /// <param name="classId"></param>
        /// <returns></returns>
        virtual void*   queryInterface(const char* clsName) override;
    protected:
        /// <summary>
        /// AnimClip 类型的变量，用于保存动画剪辑。
        /// </summary>
        AnimClip        _clip;
        /// <summary>
        /// 动画在大时间线上的起始时间，标记了动画从什么时间开始播放
        /// </summary>
        real            _offset     =   0;
        /// <summary>
        /// 记录动画播放到哪里了
        /// </summary>
        real            _clipTime   =   0;
        FETransform     _transform;
        TrackResults    _results; 
        String          _name;
    };

    using   Animation       =   SharedPtr<FEAnimation>;
    using   Animations      =   std::vector<Animation>;
}
