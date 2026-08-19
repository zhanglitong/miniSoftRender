#pragma     once
#include    "../FEChunkInf.hpp"
#include    "../FEObject.h"
#include    "../FEDefine.h"
#include    "../FEString.hpp"

namespace   FE
{
    class   FEReader :public FEObject
    {
    public:
        FEReader(FEContext& ctx)
            :FEObject(ctx)
        {}
        FEReader(const FEReader& other) =   delete;
    public:
        template<class T>
        inline  uint64  read(T& val)
        {
            auto    result  =   readBuffer(&val,sizeof(val));
            assert(result == sizeof(val));
            return  result;
        }
        template<class T>
        inline  T       readValue()
        {
            T       value   =   {};
            auto    result  =   read(value);
            UNUSED(result);
            assert(result == sizeof(value));
            return  value;
        }
        /// <summary>
        /// 读取字符串
        /// </summary>
        /// <param name="val"></param>
        /// <returns></returns>
        template<typename TSize>
        inline  uint64  read(FEString& val)
        {
            TSize   len     =   0;
            auto    result  =   readBuffer(&len,sizeof(len));
            if (result != len)
                return  result;
            val.resize(len + 1);
            result  +=   readBuffer(val.data(),len);
            assert(result == len + sizeof(len));
            return  result;
        }
        template<typename TSize>
        inline  uint64  read(String& val)
        {
            TSize   len     =   0;
            auto    result  =   readBuffer(&len,sizeof(len));
            if (result != sizeof(len))
                return  result;
            val.resize(len);
            result  +=   readBuffer(val.data(),len);
            assert(result == len + sizeof(len));
            return  result;
        }
        /// <summary>
        /// 读字符串数组
        /// </summary>
        /// <param name="vals"></param>
        /// <returns></returns>
        inline  uint64  read(Strings& vals)
        {
            uint    len     =   0;
            auto    result  =   readBuffer(&len,sizeof(len));
            if (result != len)
                return  result;
            vals.resize(len);
            for (auto& var : vals)
            {
                result  +=   read(var);
            }
            return  result;
        }
        template<typename T>
        inline  uint64  read(std::vector<T>& vals)
        {
            uint    len     =   0;
            auto    result  =   readBuffer(&len,sizeof(len));
            if (result != sizeof(len))
                return  result;
            vals.resize(len);
            for (auto& var : vals)
            {
                result  +=   read(var);
            }
            return  result;
        }
        template<typename T>
        inline  uint64  read(TVector<T>& vals)
        {
            uint    len     =   0;
            auto    result  =   readBuffer(&len,sizeof(len));
            if (result != sizeof(len))
                return  result;
            vals.resize(len);
            for (auto& var : vals)
            {
                result  +=   read(var);
            }
            return  result;
        }
        /// <summary>
        /// 读取指定大小的数据s
        /// </summary>
        /// <param name="data"></param>
        /// <param name="len"></param>
        /// <returns></returns>
        virtual uint64  readBuffer(void* data,uint64 len)   =   0;
        /// <summary>
        /// 获取位置
        /// </summary>
        /// <returns></returns>
        virtual uint64  tell() const    =   0;
        /// <summary>
        /// 设置位置
        /// </summary>
        /// <param name="pos"></param>
        /// <returns>上一次的位置</returns>
        virtual uint64  seek(uint64 pos)    =   0;;
        /// <summary>
        /// 跳过
        /// </summary>
        /// <param name="pos"></param>
        /// <returns>上一次的位置</returns>
        virtual uint64  skip(int64 pos) =   0;
        /// <summary>
        /// 长度
        /// </summary>
        /// <returns></returns>
        virtual uint64  length() const  =   0;
        /// <summary>
        /// 关闭
        /// </summary>
        virtual void    close() =   0;
        /// <summary>
        /// 是否有效
        /// </summary>
        /// <returns></returns>
        virtual bool    isValid() const =   0;
    };

    using   FEReaderPtr     =   SharedPtr<FEReader>;
}

