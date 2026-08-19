#pragma     once

#include    <type_traits>
#include    <functional>
#include    "../FEObject.h"
#include    "../FEUserObject.hpp"
#include    "../FEMeshLibrary.hpp"
#include    "../FEWriterHelper.hpp"
#include    "../FEReaderHelper.hpp"
#include    "../mesh/FEAttributeLibrary.hpp"
namespace   FE
{
    DEFINE_CLASS_UUID(FEGeometry,"{13D495E7-5B19-4B4D-9A71-8AD7AC8033B2}");

    class   FEGeometry :public FEObject
    {
        IMPLEMENT_CLASS_REFLECT(FEGeometry)
    public:
        FEGeometry(FEContext& ctx)
            :FEObject(ctx)
        {}

        FEGeometry(const FEGeometry& other)
            :FEObject(other)
        {
            _mesh   =   other._mesh;
        }
        virtual ~FEGeometry()
        {}

        inline  Mesh    mesh() const
        {
            return  _mesh;
        }
        inline  FEMesh* meshPtr()
        {
            return  _mesh.get();
        }
        const   FEMesh* meshPtr() const
        {
            return  _mesh.get();
        }
        inline  auto&   setMesh(Mesh mesh)
        {
            _mesh   =   mesh;
            return  *this;
        }
        /// <summary>
        /// 是否是参数化模型,参数化子类重写
        /// </summary>
        /// <returns></returns>
        virtual bool    isParam() const
        {
            return  false;
        }
        /// <summary>
        /// 获取依赖的对象,子类实现
        /// </summary>
        /// <param name="uset"></param>
        /// <returns>返回以来的对象个数</returns>
        virtual size_t  queryDepends(ObjectUSet& uSet) const
        {
            if (_mesh == nullptr)
                return  0;
            size_t  nPrev   =   uSet.size();
            if (isParam())
            {
            } 
            else
            {
                uSet.emplace(_mesh.get());
            }
            return  uSet.size() - nPrev;
        }
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
        /// </summary>
        /// <param name="reader"></param>
        /// <param name="ctx"></param>
        /// <returns></returns>
        virtual void    deserializeTraits(FEReader& reader,const FEChunkInf& chk ,uint version,FESerializeCtx& ctx) override;
    protected:
        mutable Mesh    _mesh;
    };
    using   Geometry    =   SharedPtr<FEGeometry>;
    using   Geometrys   =   std::vector<Geometry>;
}

