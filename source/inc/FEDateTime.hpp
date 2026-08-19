#pragma     once
#include    <stdio.h>
#include    <string>
#include    <chrono>

namespace   FE
{
    class  FEDateTime
    {
    public:
        using   tpoint  =   std::chrono::time_point<std::chrono::system_clock>;
    public:
        tpoint      _now;
        int16_t     _year;
        int16_t     _month;
        int16_t     _day;
        int16_t     _hour;
        int16_t     _minutes;
        int16_t     _second;
        /// milliseconds
        int16_t     _millis;
        int16_t     _micro;
    public:
        FEDateTime()
        {
            update(true);
        }

        FEDateTime(int year,int month,int day,int h = 0,int m = 0,int s = 0)
        {
            _now    =   makeTimePoint(year,month,day,h,m,s);
            update(false);
        }

        inline  char*   toDateString(char buf[128]) const
        {
            sprintf(buf
                , "%04hd-%02hd-%02hd"
                , this->_year
                , this->_month
                , this->_day);
            return  buf;
        }
        inline  char*   toTimeString(char buf[128]) const
        {
            sprintf(buf
                , "%02hd:%02hd:%02hd.%03hd"
                , this->_hour
                , this->_minutes
                , this->_second
                , this->_millis);
            return  buf;
        }
        inline  char*   toDateTimeString(char buf[128],const char* fmt = "%04hd-%02hd-%02hd %02hd:%02hd:%02hd.%03hd") const
        {
            sprintf(buf
                , fmt
                , this->_year
                , this->_month
                , this->_day
                , this->_hour
                , this->_minutes
                , this->_second
                , this->_millis);
            return  buf;
        }
        /// <summary>
        /// 增加天数量
        /// </summary>
        /// <param name="days"></param>
        /// <returns></returns>
        inline  auto&   plusDays(int64_t days)
        {
            _now    +=  std::chrono::hours(days * 24);
            update(false);
            return  *this;
        }
        inline  auto&   plusHours(int64_t hs)
        {
            _now    +=  std::chrono::hours(hs);
            update(false);
            return  *this;
        }
        inline  auto&   plusMinute(int64_t minus)
        {
            _now    +=  std::chrono::minutes(minus);
            update(false);
            return  *this;
        }
        inline  auto&   plusSecond(int64_t second)
        {
            _now    +=  std::chrono::seconds(second);
            update(false);
            return  *this;
        }
        inline  auto&   plusMilliseconds(int64_t ms)
        {
            _now    +=  std::chrono::milliseconds(ms);
            update(false);
            return  *this;
        }

        /// <summary>
        /// 获取天数
        /// </summary>
        /// <returns></returns>
        inline  int64_t days() const
        {
            auto    hs  =   hours() ;
            return  hs / 24 + (hs % 24 ) ? 1 : 0;
        }
        /// <summary>
        /// 获取小时，从1900-1-1开始的小时数
        /// </summary>
        inline  int64_t hours() const 
        {
            auto    tms =  _now.time_since_epoch();
            return  std::chrono::duration_cast<std::chrono::hours>(tms).count();
        }
        /// <summary>
        /// 获取分钟，从1900-1-1开始的分钟数
        /// </summary>
        inline  int64_t minutes() const 
        {
            auto    tms =  _now.time_since_epoch();
            return  std::chrono::duration_cast<std::chrono::minutes>(tms).count();
        }
        /// <summary>
        /// 获取秒，从1900-1-1开始的秒数
        /// </summary>
        inline  int64_t seconds() const
        {
            auto    tms =  _now.time_since_epoch();
            return  std::chrono::duration_cast<std::chrono::seconds>(tms).count();
        }
        /// <summary>
        /// 获取毫秒，从1900-1-1开始的毫秒数
        /// </summary>
        inline  int64_t milliSeconds() const
        {
            auto    tms =  _now.time_since_epoch();
            return  std::chrono::duration_cast<std::chrono::milliseconds>(tms).count();
        }
        /// <summary>
        /// 获取微妙，从1900-1-1开始的微秒数
        /// </summary>
        inline  int64_t microSeconds() const 
        {
            auto    tms =  _now.time_since_epoch();
            return  std::chrono::duration_cast<std::chrono::microseconds>(tms).count();
        }
        /// <summary>
        /// 获取 纳秒，从1900-1-1开始的纳秒数
        /// </summary>
        inline  int64_t nanoSeconds() const
        {
            auto    tms =  _now.time_since_epoch();
            return  std::chrono::duration_cast<std::chrono::nanoseconds>(tms).count();
        }
        /// <summary>
        /// 更新时间
        /// </summary>
        /// <param name="fromNow"></param>
        inline  void    update(bool fromNow = true)
        {
            if (fromNow)
                _now            =   std::chrono::system_clock::now();
            auto        nowLocal=  _now.time_since_epoch();
            
            time_t      tmNow   =   std::chrono::system_clock::to_time_t(_now);
            auto        cms     =   std::chrono::duration_cast<std::chrono::milliseconds>(nowLocal);
            auto        cmi     =   std::chrono::duration_cast<std::chrono::microseconds>(nowLocal);

            int         iMill   =   (cms.count()%1000);
            int         iMicro  =   (cmi.count()%1000);
            struct tm * ptminfo =   localtime(&tmNow);

            this->_year         =   (int16_t)ptminfo->tm_year + 1900;
            this->_month        =   (int16_t)ptminfo->tm_mon + 1;
            this->_day          =   (int16_t)ptminfo->tm_mday;

            this->_hour         =   (int16_t)ptminfo->tm_hour;
            this->_minutes      =   (int16_t)ptminfo->tm_min;
            this->_second       =   (int16_t)ptminfo->tm_sec;

            this->_millis       =   (int16_t)iMill;
            this->_micro        =   (int16_t)iMicro;
            
        }
        /// <summary>
        /// 构造一个时间点对象
        /// </summary>
        /// <param name="year"></param>
        /// <param name="month"></param>
        /// <param name="day"></param>
        /// <param name="hour"></param>
        /// <param name="minute"></param>
        /// <param name="second"></param>
        /// <returns></returns>
        static  tpoint  makeTimePoint(int year, int month, int day, int hour, int minute, int second)
        {
            std::tm time_info = {};
            time_info.tm_year   =   year - 1900;    // tm_year is year since 1900
            time_info.tm_mon    =   month - 1;      // tm_mon is month since January (0-11)
            time_info.tm_mday   =   day;
            time_info.tm_hour   =   hour;
            time_info.tm_min    =   minute;
            time_info.tm_sec    =   second;

            /// Convert to time_t and then to time_point
            std::time_t tmValue =   std::mktime(&time_info);
            if (tmValue == -1)
                return std::chrono::system_clock::from_time_t(0);
            else
                return std::chrono::system_clock::from_time_t(tmValue);
        }
        /// <summary>
        /// 获取两个时间之间的天数量
        /// </summary>
        /// <param name="l"></param>
        /// <param name="r"></param>
        /// <returns></returns>
        static  int     durationDays(const FEDateTime& l,const FEDateTime& r)
        {
            auto    var     =   l._now - r._now;
            auto    hs      =   l.hours() - r.hours();
            auto    days    =   hs / 24 + (hs % 24) ? 1 : 0;
            return  int(days);
        }
        /// <summary>
        /// 获取当前时间
        /// </summary>
        /// <returns></returns>
        static  auto    now()
        {
            return  FEDateTime();
        }
        /// <summary>
        /// 从字符串中构造时间对象
        /// </summary>
        /// <param name="str"></param>
        /// <returns></returns>
        static  auto    fromString(const char* str)
        {
            FEDateTime    dt;
            sscanf_s(str
                , "%hd-%hd-%hd %hd:%hd:%hd.%hd" 
                , &dt._year
                , &dt._month
                , &dt._day
                , &dt._hour
                , &dt._minutes
                , &dt._second
                , &dt._millis);
            return  dt;
        }
    };
}
