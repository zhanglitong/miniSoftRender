#pragma     once
#include    "../FEMath.hpp"
#include    "../FEObject.h"

/// 
/// tension值	效果	            应用场景
/// 0.0	        完全线性插值	    匀速运动、机械运动、默认值
/// 0.3 - 0.5   缓入缓出（标准）	最常用，自然运动、UI动画
/// 0.5 - 0.8   强缓入缓出	        强调效果、弹性感、UI弹窗
/// -0.3 - -0.5	反向缓入缓出	    先快后慢再快（特殊效果）
/// 1.0	        极端加速	        快速启动、冲击效果
/// -1.0        极端减速	        缓慢启动、阻尼效果
/// 

namespace FE
{   
    /// Material Design 标准缓动
    const   real    MD_EASE     =   0.4;  
    /// iOS 标准缓动
    const   real    IOS_EASE    =   0.42;
    /// 弹性效果
    const   real    BOUNCE_EASE =   0.6;

    DEFINE_CLASS_UUID(FEKeyFrame, "{1BEB59B8-64DE-4BF1-BDA8-B92BEB06909C}");
    class  FEKeyFrame : public FEObject
    {
    public:
        IMPLEMENT_CLASS_REFLECT(FEKeyFrame)
    public:
        enum    InterpolateType : uint8_t
        {
            /// <summary>
            /// 贝塞尔曲线
            /// </summary>
            IT_Bezier,
            /// <summary>
            /// 线性
            /// </summary>
            IT_Linear,
            /// <summary>
            /// 常量（离散）
            /// </summary>
            IT_Constant,
        };
    public:
        FEKeyFrame(FEContext& ctx)
            :FEObject(ctx)
        {
            _frame              =   0;
            _value              =   0;
            _interpolateType    =   IT_Linear;
        }
        FEKeyFrame(const FEKeyFrame & other)
            :FEObject(other)
        {
            _frame              =   other._frame;
            _value              =   other._value;
            _interpolateType    =   other._interpolateType;
        }
        ~FEKeyFrame()   =   default;

        inline  real    value() const
        {
            return _value;
        }
        inline  void    setValue(const real& value)
        {
            _value = value;
        }
        inline  uint    frame() const
        {
            return  _frame;
        }
        inline  void    setFrame(const uint& frame) 
        {
            _frame  =   frame;
        }
        inline  auto    interpolateType() const
        {
            return _interpolateType;
        }
        inline  void    setInterpolateType(InterpolateType type)
        {
            _interpolateType    =   type;
        }
    public:
        /// <summary>
        /// 计算差值
        /// </summary>
        /// <param name="iFrame">帧</param>
        /// <param name="eFrame">另一个关键帧</param>
        /// <param name="tension">贝塞尔插值有效</param>
        /// <returns></returns>
        real        interpolate(const real& dFrame, const FEKeyFrame& eFrame,real tension = MD_EASE) const
        {
            if (dFrame >= eFrame.frame())
                return eFrame.value();
            switch (_interpolateType)
            {
            case IT_Bezier:
                {
                    if (frame() == eFrame.frame()) 
                        return  value();
                    real    t   =   real(dFrame - frame()) / real(eFrame.frame() - frame());
                            t   =   std::clamp(t,0.0,1.0);
                    /// 使用三次贝塞尔实现缓入缓出
                    /// 控制点根据tension参数调整曲线形状
                    real    p0  =   value();
                    real    p3  =   eFrame.value();
                    real    p1  =   this->value() + (eFrame.value() - this->value()) * (0.5 - tension * 0.3);
                    real    p2  =   this->value() + (eFrame.value() - this->value()) * (0.5 + tension * 0.3);
                    return  cubicBezierInterpolate(p0,p1,p2,p3,t);
                }
            case IT_Linear:
                {
                    real    t   =   real(dFrame - frame()) / real(eFrame.frame() - frame());
                    return  (1.0 - t) * this->value() + t * eFrame.value();
                }
            case IT_Constant:
                {
                    return dFrame >= eFrame.frame() ? eFrame.value() : value();
                }
            }
        }
    protected:
        /// <summary>
        /// 在两个关键帧之间进行三次贝塞尔插值（最常用）
        /// </summary>
        /// <param name="p0">起始关键帧值</param>
        /// <param name="p1">起始控制点（影响曲线起始方向）</param>
        /// <param name="p2">结束控制点（影响曲线结束方向）</param>
        /// <param name="p3">结束关键帧值</param>
        /// <param name="t">插值参数 [0, 1]</param>
        /// <returns></returns>
        template<class TValue>
        static  TValue  cubicBezierInterpolate(TValue p0, TValue p1, TValue p2, TValue p3, real t) 
        {
            // 三次贝塞尔公式: B(t) = (1-t)³P0 + 3(1-t)²tP1 + 3(1-t)t²P2 + t³P3
            real    u   =   1.0 - t;
            return  u * u * u * p0 + 
                    3.0 * u * u * t * p1 + 
                    3.0 * u * t * t * p2 + 
                    t * t * t * p3;
        }
    protected:
        real            _value;
        uint            _frame;
        InterpolateType _interpolateType;
    };

    using   KeyFrame        =   SharedPtr<FEKeyFrame>;
    using   KeyFrames       =   std::vector<KeyFrame>;
}
