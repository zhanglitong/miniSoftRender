#pragma     once

#include    <type_traits>
#include    <functional>
#include    "../FEObject.h"
#include    "../FEBuffer.hpp"
#include    "../FEUserObject.hpp"
#include    "../FEObjectHelper.hpp"
#include    "../FEPickup.hpp"
#include    "../FEColor.hpp"
#include    "../FEFactory.hpp"

#include    "graphic/FECmdDraw.h"
#include    "FEDrawArray.hpp"
#include    "FEDrawElementUint8.hpp"
#include    "FEDrawElementUint16.hpp"
#include    "FEDrawElementUint32.hpp"

#include    "FEAttribute.hpp"
#include    "FEMeshBuffer.hpp"
namespace   FE
{
    /// <summary>
    /// 根据 mesh 信息 生成的唯一类型key
    /// </summary>
    struct  MeshKey 
    {
        /// <summary>
        /// 图元类型 _primitive + _drawType + _slotBits 决定类型
        /// </summary>
        EPrimitive      _primitive  =   PRI_POINTS;
        /// <summary>
        /// 绘制类型 
        /// </summary>
        EDrawType       _drawType   =   DRAW_ARRAY;
        /// <summary>
        /// 有哪些槽
        /// </summary>
        InputSlotBits   _slotBits   =   InputSlotBits();
        /// <summary>
        /// 生成Key
        /// </summary>
        /// <returns></returns>
        uint64  key() const
        {
            uint64  value   =   0;
            uint64  size    =   (std::min)(sizeof(value),sizeof(MeshKey));
            memcpy(&value,this,size);
            return  value;
        }
        /// <summary>
        /// 从一个 key中萃取对应值
        /// </summary>
        /// <param name="value"></param>
        void    setValue(uint64 value)
        {
            uint64  size    =   (std::min)(sizeof(value),sizeof(MeshKey));
            memcpy(this,&value,size);
        }
    };

    /// <summary>
    /// 一个属性对应一个缓冲区，例如位置属性，对应一个FEMeshBuffer
    /// 采用独立属性缓冲区方式
    /// 也支持交错缓冲区方式(即给属性一个符合值)
    /// </summary>
    DEFINE_CLASS_UUID(FEMesh,"{5D2E9492-6AE3-451F-9539-E44CF817C351}");
    class   FEMesh :public FEObject
    {
        IMPLEMENT_CLASS_REFLECT(FEMesh)
    public:
        using   MeshBuffers =   std::vector<FEMeshBuffer> ;
    public:
        FEMesh(FEContext& ctx)
            :FEObject(ctx)
        {}
        FEMesh(const FEMesh& other)
            :FEObject(other)
        {
            _aabb       =   other._aabb;
            _primitives =   other._primitives;
            _buffers    =   other._buffers;
        }
        virtual ~FEMesh()
        {}
        /// <summary>
        /// 获取所有图元数据
        /// </summary>
        /// <returns></returns>
        const   auto&   primitives() const
        {
            return  _primitives;
        }
        /// <summary>
        /// 设置图元数据
        /// </summary>
        /// <param name="data"></param>
        /// <returns></returns>
        inline  auto&   setPrimitives(const Primitives& data)
        {
            _primitives  =   data;
            return  *this;
        }
        InputSlotBits   slotBits() const
        {   
            InputSlotBits   result;
            for (auto& var: _buffers)
                result.addFlag(var.attr().slot());
            return  result;
        }
        const   auto&   buffers() const
        {
            return  _buffers;
        }
        inline  auto&   buffers()
        {
            return  _buffers;
        }
        /// <summary>
        /// 获取，有可能返回nullptr
        /// </summary>
        /// <param name="attrId"></param>
        /// <returns></returns>
        inline  auto    get(FEAttribute attr)
        {
            for (auto& var : _buffers)
            {
                if (var.attr().slot() == attr.slot())
                    return  &var;
            }
            return  (FEMeshBuffer*)nullptr;
        }
        /// <summary>
        /// 获取或者插入
        /// </summary>
        /// <param name="attrId"></param>
        /// <returns></returns>
        FEMeshBuffer&   getOrCreate(FEAttribute attr)
        {
            auto    result  =   get(attr);
            if (result)
                return  *result;
            FEMeshBuffer    newBuf  =  {_ctx,nullptr,attr};
            _buffers.emplace_back(newBuf);
            return  _buffers.back();
        }
        
        /// <summary>
        /// 清除所有数据
        /// </summary>
        /// <returns></returns>
        inline  auto&   clearBuffers()
        {
            _buffers.clear();
            return  *this;
        }
        /// <summary>
        /// 包围盒信息
        /// </summary>
        /// <returns></returns>
        const   aabb3f& aabb() const
        {
            return  _aabb;
        }
        /// <summary>
        /// 会更新包围盒信息并返回
        /// </summary>
        /// <returns></returns>
        const   aabb3f& updateAabb()
        {
            auto    ptr =   get(IS_VERTEX_POS);
            if (ptr)
                _aabb   =   ptr->calcAabb();
            return  _aabb;
        }
        /// <summary>
        /// 获取mesh的所有属性组
        /// </summary>
        /// <returns></returns>
        inline  Attrs   inputs() const
        {
            Attrs   result;
            for (auto& var : _buffers)
            {
                result.push_back(var.attr());
            }
            return  result;
        }
        /// <summary>
        /// 计算key
        /// </summary>
        /// <returns></returns>
        inline  uint64  key(Primitive pri)   const
        {
            MeshKey key     =   {};
            key._drawType   =   pri ? pri->type()     :   DRAW_ARRAY;
            key._primitive  =   pri ? pri->primitive():   PRI_TRIANGLES;
            key._slotBits   =   0;
            for (auto& var: _buffers)
                key._slotBits.addFlag(var.attr().slot());
            return  key.key();
        }
        /// <summary>
        /// 拾取函数
        /// </summary>
        /// <param name="ray"></param>
        /// <param name="result"></param>
        /// <returns></returns>
        bool    intersect(const Ray& ray,const mat4r& mat,FEPickup& result) const;
        /// <summary>
        /// 0:error
        /// 1:drawElemtnt
        /// 2:drawArray
        /// </summary>
        /// <param name="result"></param>
        /// <param name="idx"></param>
        /// <param name="instId"></param>
        /// <returns></returns>
        int     drawCmd(FECmdIndex& result,uint idx,uint instId);

    protected:
        bool    intersect(const Ray& ray, const FEDrawArray*,          const mat4r& mat, FEPickup& result);
        bool    intersect(const Ray& ray, const FEDrawElementUint8*,   const mat4r& mat, FEPickup& result);
        bool    intersect(const Ray& ray, const FEDrawElementUint16*,  const mat4r& mat, FEPickup& result);
        bool    intersect(const Ray& ray, const FEDrawElementUint32*,  const mat4r& mat, FEPickup& result);
    protected:
        /// <summary>
        /// 子类实现
        /// </summary>
        /// <param name="writer"></param>
        /// <param name="chunk">数据头，子类可根据情况修改(flags字段)，实现一些优化处理</param>
        /// <param name="version">版本号</param>
        /// <param name="ctx">上下文对象</param>
        /// <returns></returns>
        virtual void        serializeTraits(FEWriter& writer,FEChunkInf& chk ,uint version,FESerializeCtx& ctx) const override;
        /// <summary>
        /// 子类实现,只关注自己需要读取的数据
        /// </summary>
        /// <param name="reader"></param>
        /// <param name="chk">数据头，子类可根据chk._flags字段读取控制</param>
        /// <param name="ctx"></param>
        /// <returns></returns>
        virtual void        deserializeTraits(FEReader& reader,const FEChunkInf& chk,uint version,FESerializeCtx& ctx) override;
    protected:
        /// <summary>
        /// 包围盒信息,通过顶点属性槽计算得到
        /// </summary>
        aabb3f          _aabb;
        /// <summary>
        /// 图元列表
        /// </summary>
        Primitives      _primitives;
        /// <summary>
        /// 缓冲区数据
        /// </summary>
        MeshBuffers     _buffers;
       
    };
    using   Mesh        =   SharedPtr<FEMesh>;
    using   Meshs       =   TVector<Mesh>;
    using   MeshPtr     =   SharedPtr<FEMesh>;

    using   MeshUSet    =   std::unordered_set<Mesh>;
}


namespace   std
{
    template<>
    class   hash<FE::Mesh>
    {
    public:
        inline  size_t operator()(const FE::Mesh& key) const noexcept
        {
            std::hash<void*>    data;
            return  data((void*)key.get());
        }
    };

    template<>
    class   hash<FE::FEMesh*>
    {
    public:
        inline  size_t operator()(const FE::FEMesh* key) const noexcept
        {
            std::hash<void*>    data;
            return  data((void*)key);
        }
    };
}

