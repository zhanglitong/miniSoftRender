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

    class   FE_API  FENode :public FEItem<FENode>
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
            FLAG_ADD_CHILD      =   ((FE::FLAG_LAST)        <<1),
            /// <summary>
            /// 移除对象
            /// </summary>
            FLAG_REMOVE_CHILD   =   ((FLAG_ADD_CHILD)   <<1),

            /// <summary>
            /// 影响绘制的 vbo,ibo,ito
            /// 该状态会在节点所在工厂中消费后移除
            /// </summary>
            FLAG_PROP_MESH      =   (FLAG_REMOVE_CHILD  <<1),

            FLAG_PROP_TRANS     =   (FLAG_PROP_MESH     <<1),
            FLAG_PROP_SCALE     =   (FLAG_PROP_TRANS    <<1),
            /// <summary>
            /// 数据是四元数
            /// </summary>
            FLAG_PROP_ROT       =   (FLAG_PROP_SCALE    <<1),
            /// <summary>
            /// 节点颜色修改,影响绘制的instance,触发更新:IS_INSTANCE_COLOR
            /// </summary>
            FLAG_PROP_COLOR     =   (FLAG_PROP_ROT      <<1),
            /// <summary>
            /// 渲染状态发生变化，触发更新 IS_INSTANCE_FLAG，当修改了renderBits()
            /// 需要增加该状态，该状态会在节点所在工厂中消费后移除
            /// </summary>
            FLAG_PROP_STATE     =   (FLAG_PROP_COLOR    <<1),
            /// <summary>
            /// mesh 有可能存在lod信息,有可能没有，如果么有lod LOD_INDEX == -1,其他则是有效值
            /// 系统会收集所有lod信息到一个统一的数组中,
            /// </summary>
            FLAG_PROP_LOD       =   (FLAG_PROP_STATE    <<1),
            /// <summary>
            /// 产生阴影,触发更新 IS_INSTANCE_FLAG,当修改了renderBits()需要增加该状态，该状态会在节点所在工厂中消费后移除
            /// </summary>
            FLAG_CAST_SHADOW    =   (FLAG_PROP_LOD      << 1),
            /// <summary>
            /// 接收阴影,触发更新 IS_INSTANCE_FLAG,当修改了renderBits()需要增加该状态，该状态会在节点所在工厂中消费后移除
            /// </summary>
            FLAG_RECV_SHADOW    =   (FLAG_CAST_SHADOW   << 1),
            /// <summary>
            /// 接收灯光,触发更新 IS_INSTANCE_FLAG,当修改了renderBits()需要增加该状态，该状态会在节点所在工厂中消费后移除
            /// </summary>
            FLAG_RECV_LIGHTING  =   (FLAG_RECV_SHADOW   << 1),
            FLAG_EFFECT_AO      =   (FLAG_RECV_LIGHTING << 1),
            FLAG_EFFECT_BLOOM   =   (FLAG_EFFECT_AO << 1),
        };
        enum    EModify
        {
            ModifyValue     =   FLAG_ADD_CHILD 
                                | FLAG_REMOVE_CHILD
                                | FLAG_PROP_MESH
                                | FLAG_PROP_TRANS
                                | FLAG_PROP_SCALE
                                | FLAG_PROP_ROT
                                | FLAG_PROP_COLOR
                                | FLAG_PROP_LOD
                                | FLAG_PROP_STATE
                                | FLAG_UPDATE,

            InstanceProps   =   FLAG_PROP_TRANS
                                | FLAG_PROP_SCALE
                                | FLAG_PROP_ROT
                                | FLAG_PROP_COLOR
                                | FLAG_PROP_LOD
                                | FLAG_PROP_STATE,
        };   
        using   RenderFlags =   FEFlags<RenderFlag,uint32>;
        using   Node        =   SharedPtr<FENode>;
    public:
        IMPLEMENT_CLASS_REFLECT(FENode)
    public:
        FENode(FEContext& ctx);

        FENode(const FENode& other);
        virtual~FENode();
        /// <summary>
        /// 获取当前节点的根节点
        /// </summary>
        /// <returns></returns>
        inline  FENode* root()
        {
            if (_parent == nullptr)
                return  this;
            else
                return  _parent->root();
        }
        inline  auto    color() const
        {
            return  _color;
        }
        inline  FENode& setColor(const Rgba8& color)
        {
            _color  =   color;
            flags().addFlag(FLAG_PROP_COLOR);
            return  *this;
        }
        /// <summary>
        /// 获取渲染状态
        /// </summary>
        /// <returns></returns>
        inline  auto    renderBits()
        {
            if (flags().hasFlag(FLAG_VISIBLE))          _renderBits.addFlag(RF_VISIBLE);
            if (flags().hasFlag(FLAG_PROP_COLOR))       _renderBits.addFlag(RF_COLOR);
            if (flags().hasFlag(FLAG_SELECTED))         _renderBits.addFlag(RF_SELECTED);
            if (flags().hasFlag(FLAG_CAST_SHADOW))      _renderBits.addFlag(RF_CAST_SHADOW);
            if (flags().hasFlag(FLAG_RECV_SHADOW))      _renderBits.addFlag(RF_RECV_SHADOW);
            if (flags().hasFlag(FLAG_RECV_LIGHTING))    _renderBits.addFlag(RF_RECV_LIGHTING);
            if (flags().hasFlag(FLAG_EFFECT_AO))        _renderBits.addFlag(RF_EFFECT_AO);
            if (flags().hasFlag(FLAG_EFFECT_BLOOM))     _renderBits.addFlag(RF_EFFECT_BLOOM);

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
            return this->globalTransform() * real4(0,0,0,1);
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
        inline  bool    nameIsValid() const
        {
            return  !(_name.empty());
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
        /// <summary>
        /// 获取所有T类型对象
        /// </summary>
        /// <typeparam name="T">模板参数</typeparam>
        /// <returns>T*类型对象数组</returns>
        template<class T>
        inline  auto    objects() const
        {
            std::vector<T*>   objs;
            for (auto& var :_coms)
            {
                const T*    obj =   dynamic_cast<const T*>(var.get());
                if (obj)    objs.push_back((T*)obj);
            }
            return  objs;
        }
        virtual void    setMaterial(FEMaterial* pMat)
        {
            _material   =   pMat;
        }
        virtual void    setMesh(FEMesh* mesh)
        {
            if (_mesh.get() == mesh)
                return;
            _mesh   =   mesh;
            flags().addFlag(FLAG_PROP_MESH);
        } 
        /// <summary>
        /// 添加组件
        /// </summary>
        /// <param name="com"></param>
        /// <returns></returns>
        virtual bool    addComponent(Component com)
        {
            if (com == nullptr)
                return  false;
            auto    itr =   std::find(_coms.begin(),_coms.end(),com);
            if (itr != _coms.end()) 
                return  false;
            com->attach(this);
            _coms.push_back(com);
            return  true;
        }
        /// <summary>
        /// 移除组件
        /// </summary>
        /// <param name="com"></param>
        /// <returns></returns>
        virtual bool    removeComponent(Component com)
        {
            if (com == nullptr)
                return  false;
            auto    itr =   std::find(_coms.begin(),_coms.end(),com);
            if (itr == _coms.end()) 
                return  false;
            else
                _coms.erase(itr);
            com->detach();
            return  true;
        }
        /// <summary>
        /// 移除所有组件
        /// </summary>
        /// <param name="recursion"></param>
        virtual void    clearComponent(bool recursion = true)
        {
            _mesh       =   nullptr;
            _material   =   nullptr;
            for (auto& var : _coms)
            {
                var->detach();
            }
            _coms.clear();
            if (recursion)
            {
                for (auto& var : _childs)
                {
                    var->clearComponent(recursion);
                }
            }
        }
        /// <summary>
        /// 清空节点数据，包含子孙节点数据,以及子孙后代的组件数据
        /// </summary>
        virtual void    clear()
        {
            clearComponent(true);
            removeAllChildren();
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
        /// 修改节点属性后调用该函数刷新本地缓存
        /// 检测 ModifyValue 标志,有修改时递归更新 transform 与 aabb,并用 tmDelta 推进本节点组件
        /// 不清除 ModifyValue 标志(由所在工厂消费时清除),也不发送变更通知
        /// 典型用法:
        ///     setLocalTranslation(...);
        ///     update();
        ///     fireChanged();   // 需要外部手动调用以发送变更通知
        /// </summary>
        virtual void    update();
        /// <summary>
        /// 是否递归通知
        /// </summary>
        /// <param name="recursion"></param>
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
    public:
        /// <summary>
        /// 返回子对象个数，重写FEObject
        /// return  childs().size() + coms.size() + mesh(1) + material(1);
        /// 配合traverseObject 使用
        /// </summary>
        /// <returns></returns>
        virtual size_t  objectCount() const override;
        /// <summary>
        /// 重写FEObject,childs() + coms() + mesh() + material();
        /// 遍历子对象,如果遍历中断，返回false,否则返回true
        /// 隐藏内部细节：调用者不需要知道 Object 是如何存储的(是 std::vector、std::list，还是复杂的平衡树)。
        /// 可以在不改变外部调用的情况下，随时更换底层的存储结构。
        /// 控制访问权限：通过传递一个 const 引用或特定的 Object 副本，你可以严格控制外部代码对内部数据的修改权限。
        /// 提前退出：std::function 的返回值允许回调函数控制遍历过程。
        /// 返回 true：继续下一个。
        /// 返回 false：立即停止遍历(类似于 break)
        /// </summary>
        /// <param name=""></param>
        /// <returns>true/false</returns>
        virtual bool    traverseObject(const ObjectVisitor&,uint depth = 0,bool recur = false) const override;
    protected:
        virtual void    onAddChild(Node) override;
        virtual void    onRemoveChild(Node) override;
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
        virtual size_t  queryDepends(ObjectUSet& uset) const override;
        /// <summary>
        /// 通用设置对象属性接口，子类实现
        /// </summary>
        virtual void    beginSetProp() override;
        /// <summary>
        /// 设置属性
        /// </summary>
        /// <param name="prop">属性索引(别名)</param>
        /// <param name="value">属性值</param>
        /// <returns>true,表示修改成功,否则没有修改</returns>
        virtual bool    setProperty(int prop,const KFValue& value) override;
        /// <summary>
        /// @ref setProperty 返回结果作为输入参数，用来决定是否需要更新操作
        /// </summary>
        /// <param name="bModify"></param>
        virtual void    endSetProp(bool bModify) override;

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
        /// <summary>
        /// 经常被访问的组件
        /// </summary>
        Mesh        _mesh;
        Material    _material;
        FEString    _name;
        Components  _coms;
        Rgba8       _color;
    };

    using   Node    =   SharedPtr<FENode>;
    using   Nodes   =   std::vector<Node>;


   
}

