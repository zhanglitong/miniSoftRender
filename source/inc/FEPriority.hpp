#pragma     once
#include    <stdint.h>
namespace   FE
{
    enum    EPriority : uint16_t
    {
        /// 先绘制
        EP_Fist,
        /// 其次
        EP_Second,
        /// 最后
        EP_Last,
    };
    /// <summary>
    /// EPriority 同一个优先级有很多个，则依赖order进行排序
    /// 即先按照 EPriority排序，在按照order排序
    /// </summary>
    class   FEPriority
    {
    public:
        FEPriority(EPriority pri = EP_Fist,int16_t order = 0)
        {
            _priority   =   pri;
            _order      =   order;
        }
        /// <summary>
        /// 获取优先级分类
        /// </summary>
        /// <returns></returns>
        auto    priority() const
        {
            return  _priority;
        }
        /// <summary>
        /// 设置优先级
        /// </summary>
        /// <param name="pri"></param>
        /// <returns></returns>
        auto&   setPriority(EPriority pri)
        {
            _priority   =   pri;
            return  *this;
        }
        /// <summary>
        /// 获取顺序号
        /// </summary>
        /// <returns></returns>
        auto    order() const
        {
            return  _order;
        }
        /// <summary>
        /// 设置顺序号
        /// </summary>
        /// <param name="order"></param>
        /// <returns></returns>
        auto&   setOrder(int16_t order)
        {
            _order  =   order;
            return  *this;
        }
    protected:
        /// <summary>
        /// 优先级类别
        /// </summary>
        EPriority   _priority   =   EP_Fist;
        /// <summary>
        /// 级别 SHORT_MIN ~ SHORT_MAX
        /// </summary>
        int16_t     _order      =   0;
    };
}


