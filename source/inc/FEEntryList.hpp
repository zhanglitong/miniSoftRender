#pragma     once
#include    "FEObject.h"
#include    "FEReaderHelper.hpp"
#include    "FEWriterHelper.hpp"

namespace   FE
{
    DEFINE_CLASS_UUID(FEEntryList,"{D8FCAE35-0D25-4782-A6F3-998FA64E1CC4}");

    /// <summary>
    /// 在存储文件的时候，定义要返回哪些对象多为输出
    /// 例如:如果用户存储的node，则返回node
    /// 如果用户存储的是材质，返回的是材质
    /// </summary>
    class   FEEntryList:public FEObject
    {
    public:
        IMPLEMENT_CLASS_REFLECT(FEEntryList)
    protected:
        Objects     _entryList;
    public:
        FEEntryList(FEContext& ctx)
            :FEObject(ctx)
        {}
        FEEntryList(const FEEntryList& other)
            :FEObject(other)
        {
            _entryList  =   other._entryList;
        }
        Objects&    entryList()
        {
            return  _entryList;
        }
    public:
        /// <summary>
        /// 子类实现
        /// </summary>
        /// <param name="writer"></param>
        /// <param name="version">版本号</param>
        /// <param name="ctx">上下文对象</param>
        /// <returns></returns>
        virtual void        serializeTraits(FEWriter& writer,FEChunkInf& chunk,uint version,FESerializeCtx& ctx) const override
        {
            uint    cnt =   (uint)_entryList.size();
            writer.write(cnt);
            for (auto& var: _entryList)
            {
                OBJId   id  =   var->objectId();
                writer.write(id);
            }
;        }
        /// <summary>
        /// 子类实现,只关注自己需要读取的数据
        /// </summary>
        /// <param name="reader"></param>
        /// <param name="ctx"></param>
        /// <returns></returns>
        virtual void        deserializeTraits(FEReader& reader,const FEChunkInf& chunk,uint version,FESerializeCtx& ctx) override
        {
            uint    cnt =   (uint)_entryList.size();
            reader.read(cnt);
            _entryList.reserve(cnt);
            for (uint i = 0 ;i < cnt;++i)
            {
                OBJId   id  =   {};
                reader.read(id);
                auto    result  =   ctx.query(id,nullptr,FESerializeCtx::O_Query);
                if (result.first == nullptr)
                    continue;
                else
                    _entryList.push_back(result.first);
            }
        }
    };
}