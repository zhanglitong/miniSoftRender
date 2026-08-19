#pragma     once

#include    <type_traits>
#include    <functional>
#include    "../FEObject.h"
#include    "../FEColor.hpp"
#include    "../mesh/FEMeshHelper.hpp"
#include    "../mesh/FEAttributeLibrary.hpp"
#include    "FEGeometry.hpp"

namespace   FE
{

    class   FEGeometryParam :public FEGeometry
    {
    public:
        FEGeometryParam(FEContext& ctx)
            :FEGeometry(ctx)
        {}
        FEGeometryParam(const FEGeometryParam& other)
            :FEGeometry(other)
        {}
        virtual ~FEGeometryParam()
        {}
        /// <summary>
        /// 是否是参数化模型
        /// </summary>
        /// <returns></returns>
        virtual bool    isParam() const override
        {
            return  true;
        }
        /// <summary>
        /// 根据属性生成mesh
        /// </summary>
        /// <param name="attr"></param>
        /// <returns></returns>
        virtual Mesh    triangular(const Attrs& inputs)    =   0;
    protected:
        /// <summary>
        /// 子类实现
        /// </summary>
        /// <param name="writer"></param>
        /// <param name="version">版本号</param>
        /// <param name="ctx">上下文对象</param>
        /// <returns></returns>
        virtual void    serializeTraits(FEWriter& writer,FEChunkInf& chk ,uint version,FESerializeCtx& ctx) const override;
        /// <summary>
        /// 子类实现,只关注自己需要读取的数据
        /// 
        /// 需要优化属性部分，可以直接用索引号，从系统库中找
        /// </summary>
        /// <param name="reader"></param>
        /// <param name="ctx"></param>
        /// <returns></returns>
        virtual void    deserializeTraits(FEReader& reader,const FEChunkInf& chk ,uint version,FESerializeCtx& ctx) override;
    };
    template<typename   TParam>
    class   TFEGeometryParam :public FEGeometryParam
    {
    public:
        TFEGeometryParam(FEContext& ctx)
            :FEGeometryParam(ctx)
        {}
        TFEGeometryParam(const TFEGeometryParam& other)
            :FEGeometryParam(other)
        {}
        virtual ~TFEGeometryParam()
        {}
        inline  auto&   param()
        {
            return  _param;
        }
        const   auto&   param() const
        {
            return  _param;
        }
    protected:
        /// <summary>
        /// 子类实现
        /// </summary>
        /// <param name="writer"></param>
        /// <param name="version">版本号</param>
        /// <param name="ctx">上下文对象</param>
        /// <returns></returns>
        virtual void    serializeTraits(FEWriter& writer,FEChunkInf& chk ,uint version,FESerializeCtx& ctx) const override
        {
            /// 参数化信息写入 基类中调用 triangular,需要使用 _param
            /// 所以先写入参数化信息
            writer.write(_param);
            FEGeometryParam::serializeTraits(writer,chk,version,ctx);
        }
        /// <summary>
        /// 子类实现,只关注自己需要读取的数据
        /// 
        /// 需要优化属性部分，可以直接用索引号，从系统库中找
        /// </summary>
        /// <param name="reader"></param>
        /// <param name="ctx"></param>
        /// <returns></returns>
        virtual void    deserializeTraits(FEReader& reader,const FEChunkInf& chk ,uint version,FESerializeCtx& ctx) override
        {
            reader.read(_param);
            FEGeometryParam::deserializeTraits(reader,chk,version,ctx);
        }
    protected:
        TParam      _param  =   {};
    };
}

