#pragma     once
#include    "FEMath.hpp"

namespace   FE
{
    class   FEFileFormat
    {
    public:
        enum    StreamMode :uint16_t
        {
            /// 支持模式,support函数中调用,read from file 
            SM_FILE    =   1<<0,
            /// read from memory
            SM_MEMORY  =   1<<1,
        };
        using   ModeBits    =   FEFlags<StreamMode,uint16_t>;
        /// <summary>
        /// 解决加载器类型过滤问题，例如当我只需要加载模型类文件
        /// 那么只需要列出来支持哪些模型类的文件类型即可
        /// </summary>
        enum    Type : uint16_t
        {
            /// <summary>
            /// 三维模型类型
            /// </summary>
            DT_Model,
            /// <summary>
            /// 图片类型,   数据为ImagePtr list
            /// </summary>
            DT_Image,
            /// <summary>
            /// 缓冲区类,   数据为 DataObject List
            /// </summary>
            DT_Buffer,
            /// <summary>
            /// GIS 要素类
            /// </summary>
            DT_Feature,
            /// <summary>
            /// 属性相关，模型属性
            /// </summary>
            DT_Attribute,
            /// <summary>
            /// 数据库
            /// </summary>
            DT_Database,
        };
    public:
        FEFileFormat(PCSTR format = nullptr,PCSTR version=nullptr,PCSTR desc = nullptr)
        {
            if (format)
                strncpy(_format,    format, sizeof(_format) - 1);
            else
                memset(_format,     0,      sizeof(_format));

            if (version)
                strncpy(_version,   version,sizeof(_version) - 1);
            else
                memset(_version,    0,      sizeof(_version));

            if (desc)
                strncpy(_desc,      desc,   sizeof(_desc) - 1);
            else
                memset(_desc,       0,      sizeof(_desc));
        }
    public:
        ModeBits    _mode           =   SM_FILE;
        Type        _type           =   DT_Model;
        CLSId       _loaderId;
        /// 格式字符串
        char        _format[16]     =   {0};
        /// 版本
        char        _version[16]    =   {0};
        /// 描述信息
        char        _desc[64]       =   {0};
    public:
        /// <summary>
        /// 加载器类型id
        /// </summary>
        /// <returns></returns>
        const   CLSId&  loaderId() const
        {
            return  _loaderId;
        }
        inline  bool    isValid() const
        {
            return  strlen(_format) != 0;
        }
        /// <summary>
        /// 返回格式字符串,例如".pprj" 可写
        /// </summary>
        /// <returns>扩展名</returns>
        inline  auto    ext() 
        {
            return  _format;
        }
        /// <summary>
        /// 返回格式字符串,例如".pprj" 只读
        /// </summary>
        /// <returns></returns>
        const   auto    ext() const
        {
            return  _format;
        }
        /// <summary>
        /// 获取版本号,例如:"1.0.0.1"
        /// </summary>
        /// <returns></returns>
        const   auto    version() const
        {
            return  _format;
        }
        /// <summary>
        /// 返回描述,例如: "video file"
        /// </summary>
        /// <returns></returns>
        const   auto    desc() const
        {
            return  _desc;
        }
        /// <summary>
        /// 返回描述字符串，用于组装打开文件列表
        /// </summary>
        inline  String  toString() const
        {
            char    szBuf[128]  =   {0};
            if(strlen(_version) != 0)
                sprintf(szBuf,"%s[%s](*%s)",_desc,  _version,   _format);
            else
                sprintf(szBuf,"%s(*%s)",   _desc,   _format);

            return  szBuf;
        }
    public:
        /// <summary>
        /// 比较是否是相同的类型,如果格式一样，比较版本，没有版本信息，则比较通过
        /// </summary>
        friend  bool    operator  == (const FEFileFormat& l,const FEFileFormat& r)
        {
            /// 如果扩展名不一样，不比较，直接不匹配
            if (_stricmp(l.ext(),r.ext()) != 0)
                return  false;
            /// 如果左侧有版本，则要比较版本
            if (strlen(l.version()) > 0 && _stricmp(l.version(),r.version()) != 0)
                return  false;
            /// 如果左侧有过滤信息，则要比较，否则不比较
            if (strlen(l.desc()) > 0 && _stricmp(l.desc(),r.desc()) != 0)
                return  false;
            return  true;
        }
        /// <summary>
        /// 比较是否是相同的类型,如果格式一样，比较版本，没有版本信息，则比较通过
        /// </summary>
        friend  bool    operator  != (const FEFileFormat& l,const FEFileFormat& r)
        {
            return  !(l == r);
        }
        /// <summary>
        /// 格式化对象
        /// </summary>
        /// <param name="filter"></param>
        /// <returns></returns>
        static  auto    fromFilter(PCSTR filter)
        {
            FEFileFormat  fmt("",nullptr,nullptr);
            String      sFilter     =   filter;
            String      desc;
            String      version;
            auto        nVStart     =   sFilter.find("[");
            auto        nVEnd       =   sFilter.find("]");
            auto        nExtStart   =   sFilter.find_last_of("(");
            auto        nExtEnd     =   sFilter.find_last_of(")");
            if (nVStart != std::string::npos && nVEnd != std::string::npos)
                strncpy_s(fmt._version,   filter + nVStart + 1,   nVEnd - nVStart - 1);
            if (nExtStart != std::string::npos && nExtEnd != std::string::npos)
                strncpy_s(fmt._format,    filter + nExtStart + 2, nExtEnd - nExtStart - 2);
            if (nVStart != std::string::npos)
                strncpy_s(fmt._desc,filter,nVStart);
            else if(nExtStart != std::string::npos)
                strncpy_s(fmt._desc,filter,nExtStart);
            return  fmt;
        }
    };

    extern  void    systemRegReader(const FEFileFormat& fmt);
    extern  void    systemRegWriter(const FEFileFormat& fmt);

    #define DEFINE_FORMAT_READER(className)             \
            for (auto& fmt: className::formatList())    \
            {                                           \
                systemRegReader(fmt);                   \
            }                                           \

    #define DEFINE_FORMAT_WRITER(className)             \
            for (auto& fmt: className::formatList())    \
            {                                           \
                systemRegWriter(fmt);                   \
            }                                           \

    using   Format      =   FEFileFormat;
    using   Formats     =   FEVector<FEFileFormat>;
}