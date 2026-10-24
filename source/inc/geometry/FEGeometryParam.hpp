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

    class   FE_API  FEGeometryParam :public FEGeometry
    {
    public:
        FEGeometryParam(FEContext& ctx)
            :FEGeometry(ctx)
        {}
        FEGeometryParam(const FEGeometryParam& other) = default;
        virtual ~FEGeometryParam()
        {}
        /// <summary>
        /// 根据属性生成mesh
        /// </summary>
        /// <param name="attr"></param>
        /// <returns></returns>
        virtual Mesh    triangular(const Attrs& inputs)    =   0;
    };
    template<typename   TParam>
    class   TFEGeometryParam :public FEGeometryParam
    {
    public:
        TFEGeometryParam(FEContext& ctx)
            :FEGeometryParam(ctx)
        {}
        TFEGeometryParam(const TFEGeometryParam& other) = default;
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
            UNUSED(chk,version,ctx);
            writer.write(_param);
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
            UNUSED(chk,version,ctx);
            reader.read(_param);
        }
    protected:
        TParam      _param  =   {};
    };
}

