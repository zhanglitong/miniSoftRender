#pragma     once
#include    <functional>
#include    "FEClip.hpp"

namespace FE
{
    DEFINE_CLASS_UUID(FEAnimation, "{D0749EE9-7126-4A57-B4F2-84798E4F40F2}");

    /// <summary>
    /// 对象中包含需要被动画的对象
    /// 以及clip信息，clip 动画片段，包含实际的动画关键帧组数据
    /// 可以实现动画数据与动画对象分离,让动画数据有复用价值
    /// 典型应用：一个对象，在时间线上，可以断开
    /// </summary>
    class   FEAnimation : public FEObject
    {
    public:
        IMPLEMENT_CLASS_REFLECT(FEAnimation)
    public:
        FEAnimation(FEContext& ctx)  
            :FEObject(ctx)
        {}
        FEAnimation(const FEAnimation& other)
            :FEObject(other)
        {
            _clip       =   other._clip;
            _object     =   other._object;
            _results    =   other._results;
        }
        ~FEAnimation()   =   default;
    public:
        /// <summary>
        /// 是否有效
        /// </summary>
        /// <returns></returns>
        bool    isValid() const
        {
            return  _clip != nullptr &&  _object != nullptr;
        }
        ClipPtr clip() const
        {
            return  _clip;
        }
        void    setClip(ClipPtr clip)
        {
            _clip   =   clip;
        }

        Object  object() const
        {
            return  _object;
        }
        void    setObject(Object object)
        {
            _object =   object;
        }
        /// <summary>
        /// 更新,会把动画数据应用到对象上
        /// </summary>
        /// <param name="tmDelat"></param>
        bool    update(const real& tmDelta)
        {
            if (_clip == nullptr || _object == nullptr)
                return  false;
            _clip->update(tmDelta,_results);

            _object->beginSetProp();

            bool    bModify =   false;

            for (auto& var: _results)
            {
                if (!var._valid)
                    continue;
                bModify |=  _object->setProperty(var._prop,var._value);
            }
            _object->endSetProp(bModify);
            return  true;
        }
    public:
        ClipPtr         _clip;
        Object          _object;
        TrackResults    _results; 
    };

    using   Animation       =   SharedPtr<FEAnimation>;
    using   Animations      =   std::vector<Animation>;
}
