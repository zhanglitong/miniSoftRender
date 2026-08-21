#pragma     once

#include    "../FEItem.hpp"
#include    "../FEObjectHelper.hpp"
#include    "../FEString.hpp"
#include    "../FEColor.hpp"
#include    "../material/FEMaterial.hpp"
#include    "../geometry/FEGeometry.hpp"
#include    "../FEPickup.hpp"
#include    "../FEMathUtil.hpp"
#include    "../FEComponent.hpp"
  

namespace   FE
{
    DEFINE_CLASS_UUID(FENode,"{4ED0DD0C-AC55-4C7F-86FC-6E3BA46FFA97}");

    class   FENode :public FEItem<FENode>
    {
    public:
        enum    NodeFlag
        {
            /// <summary>
            /// 影响绘制的instance
            /// 会触发更新 IS_INSTANCE_MAT_C0,IS_INSTANCE_MAT_C1,IS_INSTANCE_MAT_C2,IS_INSTANCE_MAT_C3
            /// 该状态会在节点所在工厂中消费后移除
            /// </summary>
            /// <summary>
            /// 添加对象
            /// </summary>
            FLAG_ADD_CHILD      =   ((FLAG_LAST)<<1),
            /// <summary>
            /// 移除对象
            /// </summary>
            FLAG_REMOVE_CHILD   =   ((FLAG_ADD_CHILD)<<1),
            /// <summary>
            /// 对象修改
            /// </summary>
            FLAG_MODIFY_CHILD   =   ((FLAG_REMOVE_CHILD)<<1),

            FLAG_PROP_TRANS     =   ((FLAG_MODIFY_CHILD)<<1),
            FLAG_PROP_SCALE     =   (FLAG_PROP_TRANS<<1),
            FLAG_PROP_ROT       =   (FLAG_PROP_SCALE<<1),
            /// <summary>
            /// 节点颜色修改,影响绘制的instance,触发更新:IS_INSTANCE_COLOR
            /// </summary>
            FLAG_PROP_COLOR     =   (FLAG_PROP_ROT<<1),
            /// <summary>
            /// 渲染状态发生变化，触发更新 IS_INSTANCE_FLAG，当修改了renderBits()
            /// 需要增加该状态，该状态会在节点所在工厂中消费后移除
            /// </summary>
            FLAG_PROP_STATE     =   (FLAG_PROP_COLOR<<1),
            /// <summary>
            /// mesh 有可能存在lod信息,有可能没有，如果么有lod LOD_INDEX == -1,其他则是有效值
            /// 系统会收集所有lod信息到一个统一的数组中,
            /// </summary>
            FLAG_PROP_LOD       =   (FLAG_PROP_STATE<<1),
            /// <summary>
            /// 影响绘制的 vbo,ibo,ito
            /// 该状态会在节点所在工厂中消费后移除
            /// </summary>
            FLAG_PROP_MESH      =   (FLAG_PROP_STATE<<1),
        };
        enum    EModify
        {
            ModifyValue = FLAG_ADD_CHILD 
                        | FLAG_REMOVE_CHILD
                        | FLAG_MODIFY_CHILD 
                        | FLAG_PROP_TRANS
                        | FLAG_PROP_SCALE
                        | FLAG_PROP_ROT
                        | FLAG_PROP_COLOR
                        | FLAG_PROP_LOD
                        | FLAG_PROP_MESH
        };   
        using   RenderFlags =   FEFlags<RenderFlag,uint32>;
    public:
        IMPLEMENT_CLASS_REFLECT(FENode)
    public:
        FENode(FEContext& ctx);

        FENode(const FENode& other);

        inline  auto    color() const
        {
            return  _color;
        }
        
        inline  auto&   renderBits()
        {
            return  _renderBits;
        }
        const   auto&   renderBits() const
        {
            return  _renderBits;
        }
        inline  auto    localTranslation() const
        {
            return  _trans;
        }
        inline  auto    localScaling() const
        {
            return  _scale;
        }
        inline  auto    localRotation() const
        {
            return  _rotate;
        }
        inline  FENode& setLocalTranslation(const real3& trans)
        {
            _trans  =   trans;
            flags().addFlag(FLAG_PROP_TRANS);
            return  *this;
        }
        inline  FENode& setLocalScaling(const real3& scale)
        {
            _scale  =   scale;
            flags().addFlag(FLAG_PROP_SCALE);
            return  *this;
        }
        inline  FENode& setLocalRotation(const quatr& rot)
        {
            _rotate =   rot;
            flags().addFlag(FLAG_PROP_ROT);
            return  *this;
        }
        /// <summary>
        /// 设置 从世界到节点的位置变换
        /// </summary>
        /// <param name="vec">从世界到节点的位置变换</param>
        inline  auto&   setGlobalTranslation(const real3& vec)
        {
            if (_parent != nullptr)
                _trans  =   FE::inverse(_parent->as<FENode>()->globalTransform()) * real4(vec,1.0);
            else
                _trans  =   vec;
            flags().addFlag(FLAG_PROP_TRANS);
            return  *this;
        }
        /// <summary>
        /// 获取 从世界到节点的位置变换
        /// </summary>
        /// <returns>从世界到节点的位置变换</returns>
        inline  real3   globalTranslation() const
        {
            return this->globalTransform() * real4(0,0,0,0);
        }
        inline  quatr   globalRotation() const
        {
            real3   pos;
            real3   scale;
            quatr   rot;
            FE::decompose<real>(_transform, pos, scale, rot);
            return  rot;
        }
        /// <summary>
        /// 设置 从世界到节点的旋转变换
        /// </summary>
        /// <param name="quat">q从世界到节点的旋转变换</param>
        inline  auto&   setGlobalRotation(const quatr& quat)
        {
            if (_parent != nullptr)
            {
                mat4r   rMat    =   mat4_cast(quat);
                        rMat    =   FE::inverse(rMat * _parent->as<FENode>()->globalRSTransform());
                real3   pos;
                real3   scale;
                quatr   rot;
                FE::decompose<real>(rMat, pos, scale, rot);
                _rotate =   rot;
            }
            else
            {
                _rotate =   quat;
            }
            flags().addFlag(FLAG_PROP_TRANS);
            return  *this;
        }
        inline  quatr   globalScaling() const
        {
            real3   pos;
            real3   scale;
            quatr   rot;
            FE::decompose<real>(_transform, pos, scale, rot);
            return  scale;
        }

        inline  void    setName(const String& name)
        {
            _name   =   name;
        }
        inline  PCSTR   name() const
        {
            return  _name.c_str();
        }
        /// <summary>
        /// 材质
        /// </summary>
        /// <returns></returns>
        inline  auto    material() const
        {
            return  _material;
        }
        /// <summary>
        /// 几何体
        /// </summary>
        /// <returns></returns>
        inline  auto    mesh() const
        {
            return  _mesh;
        }
        inline  auto&   components() const
        {
            return  _coms;
        }
        virtual void    setMaterial(FEMaterial* pMat)
        {
            _material   =   pMat;
        }
        virtual void    setMesh(FEMesh* mesh)
        {
            _mesh   =   mesh;
        } 
        virtual void    addComponent(Component object)
        {
            _coms.push_back(object);
        }
        /// <summary>
        /// 添加需要更新标记
        /// </summary>
        /// <returns></returns>
        inline  FENode& makeDirty()
        {
            flags().addFlag(FLAG_UPDATE);
            return  *this;
        }
        /// <summary>
        /// 检测更新
        /// </summary>
        virtual void    update(const real& tmDelta = 0.0);
        virtual void    fireChanged();
        /// <summary>
        /// 更新包围盒信息，不检测是否需要，直接计算
        /// </summary>
        /// <param name="recursion"></param>
        /// <returns></returns>
        virtual aabb3dr updateAabb(bool recursion = true);
        /// <summary>
        /// 更新矩阵信息，不检测是否需要，直接计算
        /// </summary>
        /// <param name="recursion"></param>
        virtual void    updateTransform(bool recursion = true);
        /// <summary>
        /// 拾取函数
        /// </summary>
        /// <param name="ray"></param>
        /// <param name="result"></param>
        /// <returns></returns>
        virtual size_t  intersect(const Ray& ray,Pickups& result) const;
        virtual size_t  intersect(const Ray& ray,FEPickup& result) const;
        /// <summary>
        /// 局部坐标矩阵
        /// </summary>
        /// <returns></returns>
        inline  mat4r   localTransform() const
        {
            return  FE::makeTransform<real>(_trans,_scale,_rotate);
        }
        /// <summary>
        /// 没有平移信息
        /// </summary>
        /// <returns></returns>
        inline  mat4r   globalRSTransform() const
        {
            mat4r   tMat    =   globalTransform();
                    tMat[3] =   real4(real3(0.0), real(1.0));
            return  tMat;
        }
        /// <summary>
        /// 世界坐标矩阵
        /// </summary>
        /// <returns></returns>
        inline  mat4r   globalTransform() const
        {
            return  _transform;
        }
        inline  aabb3dr globalAabb() const
        {
            return  _aabb;
        }
    protected:
        virtual void    onAddChildren() override
        {
            flags().addFlag(FENode::FLAG_ADD_CHILD);
        }
        virtual void    onRemoveChildren() override
        {
            flags().addFlag(FLAG_REMOVE_CHILD);
        }
        /// <summary>
        /// 子类实现
        /// </summary>
        /// <param name="writer"></param>
        /// <param name="version">版本号</param>
        /// <param name="ctx">上下文对象</param>
        /// <returns></returns>
        virtual void    serializeTraits(FEWriter& writer,FEChunkInf& chunk,uint version,FESerializeCtx& ctx) const override;
        /// <summary>
        /// 子类实现,只关注自己需要读取的数据
        /// </summary>
        /// <param name="reader"></param>
        /// <param name="ctx"></param>
        /// <returns></returns>
        virtual void    deserializeTraits(FEReader& reader,const FEChunkInf& chunk,uint version,FESerializeCtx& ctx) override;
        /// <summary>
        /// 获取依赖的对象,子类实现
        /// </summary>
        /// <param name="uset"></param>
        /// <returns>返回以来的对象个数</returns>
        virtual size_t  queryDepends(ObjectUSet& uset) const
        {
            const auto  vSize   =   uset.size();
            FEObject::queryDepends(uset);
            for (auto var : _coms)
            {
                uset.emplace(var);
            }
            return  uset.size() - vSize;
        }
    protected:
        real3       _trans;
        /// <summary>
        /// 这两个值精度够用
        /// </summary>
        float3      _scale;
        quatf       _rotate;
        RenderFlags _renderBits;
        aabb3r      _aabb;
        mat4r       _transform;
        uint32      _dataFlag;
        Rgba8       _color;
        /// <summary>
        /// 经常被访问的组件
        /// </summary>
        Mesh        _mesh;
        Material    _material;
        FEString    _name;
        Components  _coms;
    };

    using   Node    =   SharedPtr<FENode>;
    using   Nodes   =   std::vector<Node>;


   
}

