#pragma     once

#include    "FEDateTime.hpp"


namespace   FE
{
    class FETimestamp
    {
    public:
        FETimestamp()
        {
            _nano   =   FEDateTime::now().nanoSeconds();
        }
        ~FETimestamp()
        {}

        inline  void    update()
        {
            _nano   =   FEDateTime::now().nanoSeconds();
        }
        /// <summary>
        /// 返回消耗时间，单位秒
        /// </summary>
        /// <returns></returns>
        inline  double  second() const
        {
            return  nano() * 0.000000001;
        }
        /// <summary>
        /// 返回消耗时间，单位毫秒
        /// </summary>
        /// <returns></returns>
        inline  double  milliSec() const
        {
            return  nano() * 0.000001;
        }
        /// <summary>
        /// 返回消耗时间，单位微秒
        /// </summary>
        /// <returns></returns>
        inline  double  microSec() const
        {
            return  nano() * 0.001;
        }
        /// <summary>
        /// 返回消耗时间，单位纳秒
        /// </summary>
        /// <returns></returns>
        inline  double  nano() const
        {
            auto    nano    =   FEDateTime::now().nanoSeconds() - _nano;
            return  double(nano);
        }
    protected:
        int64_t         _nano;
    };
}
