#pragma     once

#include    "FEObject.h"
#include    "FEReaderHelper.hpp"
#include    "FEWriterHelper.hpp"
#include    "FEContext.hpp"
namespace   FE
{
    class   FEObjectHelper
    {
    public:
        template<typename T>
        static  FEVector<SharedPtr<T>>      clone(const FEVector<SharedPtr<T>>& others)
        {
            FEVector<SharedPtr<T>>  result;
            result.reserve(others.size());
            for (auto& var : others)
            {
                auto            object  =   var->clone();
                if (object == nullptr)
                    continue;
                SharedPtr<T>    refObj  =   object->as<T>();
                if (refObj == nullptr)
                    continue;
                result.emplace_back(refObj);
            }
            return  result;
        }

        template<typename T>
        static  std::vector<SharedPtr<T>>   clone(const std::vector<SharedPtr<T>>& others)
        {
            std::vector<SharedPtr<T>>   result;
            result.reserve(others.size());
            for (auto& var : others)
            {
                auto            object  =   var->clone();
                if (object == nullptr)
                    continue;
                SharedPtr<T>    refObj  =   object->as<T>();
                if (refObj == nullptr)
                    continue;
                result.emplace_back(refObj);
            }
            return  result;
        }
        /// <summary>
        /// 标准对象读取流程
        /// </summary>
        /// <param name="_ctx"></param>
        /// <param name="reader"></param>
        /// <param name="version"></param>
        /// <param name="xctx"></param>
        /// <returns>nullptr 失败</returns>
        static  Object  readObject(FEContext& _ctx,FEReader& reader,uint& version,FESerializeCtx& xctx)
        {
            (void)version;
            FEReaderHelper  helper(reader);
            FEChunkInf      infor       =   {};
            CLSId           clsId;
            uint            nVerison    =   0;
            const auto      nCur        =   reader.tell();
            Object          result      =   nullptr;
            do
            {  
                if(reader.read(infor)  !=  sizeof(infor))
                    break;
                if(reader.read(clsId)  !=  sizeof(clsId))
                    break;
                /// 相同,直接使用构建器
                if (clsId == xctx.preCLSId)
                {
                    result  =   createObject(_ctx,clsId,&xctx.preCreator,nullptr);
                }
                /// 不同,更新构建器
                else
                {
                    xctx.preCLSId   =   clsId;
                    result          =   createObject(_ctx,clsId,nullptr,&xctx.preCreator);
                }
                if (result == nullptr)
                {
                    reader.skip(infor._length - sizeof(infor) - sizeof(clsId));
                    break;
                }   
                /// 序列化的时候要从块的开始读取，所以要跳回块的开始
                reader.seek(nCur);
                /// 读取
                result->deserialize(reader,nVerison,xctx);

            } while (false);
            return  result;
        }
        /// <summary>
        /// 创建对象
        /// </summary>
        /// <param name="_ctx">应用上下文</param>
        /// <param name="clsId">类型Id</param>
        /// <param name="pIn">输入构建器</param>
        /// <param name="pOut">输出构建器</param>
        /// <returns>对象</returns>
        static  Object  createObject(FEContext& _ctx,const CLSId& clsId,FECreator* pIn  = nullptr,FECreator* pOut = nullptr)
        {
            FECreator*  pCreator    =   nullptr;
            if (pIn)
            {
                pCreator    =   pIn;
            }
            else
            {
                pCreator    =   FEContext::creators().isExist(clsId);
            } 
            if(pOut)    
            {
                *pOut   =   (pCreator !=nullptr) ? *pCreator : FECreator();
            }
            if (pCreator == nullptr)
            {
                auto    name    =   clsId.toString();
                LOG_INF("_ctx.objectCreator().isExist(%s) return nullptr",name.c_str());
                return  nullptr;
            }
            
            Object  result  =   (*pCreator)(_ctx,{});
            if (result == nullptr)
            {
                auto    name    =   clsId.toString();
                LOG_INF("(*pCreator)(_ctx,{}) return nullptr");
                return  nullptr;
            }
            return  result;
        }
    };
   
}

