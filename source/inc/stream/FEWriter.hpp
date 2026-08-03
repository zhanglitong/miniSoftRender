#pragma     once
#include    "../FEChunkInf.hpp"
#include    "../FEObject.h"
#include    "../FEString.hpp"
namespace   FE
{
    class   FEWriter :public FEObject
    {
    public:
        /// <summary>
        /// 被写入器解释，协助框架实现，无业务价值
        /// </summary>
        struct  Tag
        {
            std::string_view    tag;
            std::string_view    type;
            mutable void*       ctx =   nullptr;
        };
        FEWriter(FEContext& ctx)
            :FEObject(ctx)
        {}
        FEWriter(const FEWriter& other) =   delete;
    public:
        /// <summary>
        /// 写入数据模版方法
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="val"></param>
        /// <param name="tag">标签,在xml,json 等文本格式有意义,用来描述数据</param>
        /// <returns></returns>
        template<class T>
        inline  uint64  write(const T& val,const Tag& tag = {})
        {
            return  writeBuffer(&val,sizeof(val));
        }
        template<typename T>
        inline  uint64  write(const std::vector<T>& vals)
        {
            uint    len     =   (uint)vals.size();
            auto    result  =   writeBuffer(&len,sizeof(len));
            if (result != sizeof(len))
                return  result;
            for (auto& var : vals)
            {
                result  +=   write(var);
            }
            return  result;
        }
        template<typename T>
        inline  uint64  write(const TVector<T>& vals)
        {
            uint    len     =   (uint)vals.size();
            auto    result  =   writeBuffer(&len,sizeof(len));
            if (result != sizeof(len))
                return  result;
            for (auto& var : vals)
            {
                result  +=   write(var);
            }
            return  result;
        }
        template<typename TSize = uint>
        inline  uint64  write(const FEString& val)
        {
            TSize   len =   (TSize)val.size();
            auto    res =   writeBuffer(&len,       sizeof(len));
                    res +=  writeBuffer(val.c_str(),len);
            assert(res == len + sizeof(len));
            return  res;
        }
        template<typename TSize = uint>
        inline  uint64  write(const String& val)
        {
            TSize   len =   (TSize)val.size();
            auto    res =   writeBuffer(&len,       sizeof(len));
            res +=  writeBuffer(val.c_str(),len);
            assert(res == len + sizeof(len));
            return  res;
        }
        /// <summary>
        /// 写缓冲区
        /// </summary>
        /// <param name="data">数据地址 </param>
        /// <param name="len">数据长度</param>
        /// <param name="tag">数据标签</param>
        /// <returns>写入大小,0失败</returns>
        virtual uint64  writeBuffer(const void*data,uint64 len,const Tag& tag = {})   =   0;
        /// <summary>
        /// 获取位置
        /// </summary>
        /// <returns></returns>
        virtual uint64  tell() const    =   0;
        /// <summary>
        /// 设置位置
        /// </summary>
        /// <param name="pos"></param>
        /// <returns></returns>
        virtual uint64  seek(uint64_t pos)  =   0;
    };

    using   FEWriterPtr     =   SharedPtr<FEWriter>;
}

