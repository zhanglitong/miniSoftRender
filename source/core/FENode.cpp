
#include    "../inc/node/FENode.hpp"
#include    "../inc/graphic/FEScene.h"
#include    "../inc/FEPropertyIndex.hpp"
#include    "../inc/FEEulerObject.hpp"
#include    "../inc/constraint/FEConstraint.hpp"

namespace   FE
{

    FENode::FENode(FEContext& ctx)
        :FEItem<FENode>(ctx)
    {
        _local._scale       =   float3(1,1,1);
        _local._position    =   real3(0,0,0);
        _local.setRotation(quatf(1,0,0,0));
        /// 默认情况下颜色会 color x fragment 
        _color      =   Rgba8(255,255,255,255);
        _gloabal    =   _local;
        _renderBits =   RF_VISIBLE;
    }

    FENode::FENode(const FENode& other)
        :FEItem<FENode>(other)
    {
        _local  =   other._local    ;
        _renderBits =   other._renderBits   ;
        _aabb       =   other._aabb         ;

        _gloabal    =   other._gloabal      ;
        _material   =   other._material     ; 
        _mesh       =   other._mesh         ; 
        _color      =   other._color        ;
        _name       =   other._name         ;
        _coms       =   FEObjectHelper::clone(other._coms);
    }
    FENode::~FENode()
    {
    }

    void    FENode::update(const real& tm)
    {
        if (!flags().hasFlags(ModifyValue))
            return;
        updateTransform(tm,true);
        
        updateAabb(true);
    }
    void    FENode::fireChanged()
    {
        /// 无窗口/无活动场景时(_ctx.scene() 为空)跳过事件通知,与 reparent 的守卫一致
        if ( flags().hasFlags(ModifyValue) && _ctx.scene() != nullptr )
        {
            _ctx.scene()->nodeTree().eventsChangedNode().fireNotify(this);
            for (auto& var : _childs)
            {
                var->fireChanged();
            }
        }
    }
    void    FENode::onAddChild(Node node)
    {
        flags().addFlag(FENode::FLAG_ADD_CHILD);
        if (_ctx.scene() != nullptr)
            _ctx.scene()->nodeTree().eventsAddNode().fireNotify(node);
    }
    void    FENode::onRemoveChild(Node node)
    {
        flags().addFlag(FLAG_REMOVE_CHILD);
        if (_ctx.scene() != nullptr)
            _ctx.scene()->nodeTree().eventsRemoveNode().fireNotify(node);
    }

    void    FENode::reparent(FENode* node, size_t index)
    {
        FENode* newParent    =   node;
        FENode* oldParent    =   _parent.get();
        /// 防环: 新父节点不能是 this 自身,也不能是 this 的子孙节点
        /// 同父场景(排序)不拦截: newParent==oldParent 表示同父重排(同父时后面流程会先摘再挂)
        if (newParent != nullptr)
        {
            for (FENode* p = newParent; p != nullptr; p = p->_parent.get())
            {
                if (p == this)
                    return;
            }
        }
        /// 3. 保存当前世界变换,稍后据此反推新本地变换
        mat4r   global      =   globalTransform();
        Node    selfNode(this);

        /// 4. 从旧父节点摘除: 维护 children 结构并触发移除通知
        ///    同父场景也走这一步(先摘再挂到新位置)
        if (oldParent != nullptr)
        {
            auto    itr     =   std::find(oldParent->_childs.begin(),
                                         oldParent->_childs.end(), selfNode);
            if (itr != oldParent->_childs.end())
            {
                oldParent->_childs.erase(itr);
                _parent     =   nullptr;
                oldParent->onRemoveChild(selfNode);
            }
        }

        /// 5. 挂到新父节点: 按 index 插入(与 V3 CELLNode::reparent(parent,index) 一致)
        ///    index=SIZE_MAX 或越界时追加到末尾; 手动 setParent+onAddChild 等效于 addChild
        if (newParent != nullptr)
        {
            auto&   chs =   newParent->_childs;
            if (index == SIZE_MAX || index >= chs.size())
                chs.emplace_back(selfNode);
            else
                chs.insert(chs.begin() + index, selfNode);
            _parent     =   newParent;
            newParent->onAddChild(selfNode);
        }

        /// 6. 反推新本地变换,使 parentGlobal_new * local_new = global
        ///    新父节点为空(顶层)时 parentGlobal 取单位矩阵
        mat4r   parentGlobal    =   newParent ? newParent->globalTransform()
                                             : mat4r(real(1.0));
        mat4r   localMat        =   FE::inverse(parentGlobal) * global;
        real3   pos;
        real3   scl;
        quatr   rot;
        FE::decompose<real>(localMat, pos, scl, rot);
        _local._position    =   pos;
        _local._scale       =   float3(scl);
        _local.setRotation(quatf(rot));
        flags().addFlag(FLAG_PROP_TRANS | FLAG_PROP_SCALE | FLAG_PROP_ROT);
        /// _gloabal 保持为保存的 global 不变,globalTransform() 立即可读;
        /// 下一帧 update() 按 parentGlobal * local 重算,结果仍为 global

        /// 7. 立即刷新受影响节点: 重算全局变换与包围盒并发送变更通知
        ///    更新列表已下沉到组件系统,节点层级变更不属于组件系统,故此处直接刷新
        selfNode->update();
        selfNode->fireChanged();
        if (newParent != nullptr)
        {
            newParent->update();
            newParent->fireChanged();
        }
        if (oldParent != nullptr)
        {
            oldParent->update();
            oldParent->fireChanged();
        }
    }

    aabb3dr FENode::updateAabb(bool recursion)
    {
        auto    mesh    =   _mesh;
        if (mesh)
        {
            _aabb       =   mesh->aabb();
            _aabb.transform(globalTransform());
        }
        else
        {
            ///无 mesh 节点(如纯组节点)重置为空包围盒,避免脏数据参与合并
            _aabb.setNull();
        }
        if (!recursion || children().empty() )
            return  _aabb;

        aabb3dr tmp;
        auto&   chs =   children();
        /// 先计算所有子节点的包围盒
        for(auto& child : chs)
        {
            auto    node    =   child->as<FENode>();
            tmp.merge(node->updateAabb(recursion));
        }
        _aabb.merge(tmp);
        return  _aabb;
    }
    
    void    FENode::updateTransform(const real& ,bool recursion)
    {
        /// 本地变换由动画系统/约束系统直接写入,节点不再反向查询组件
        /// 全局变换 = 父全局 * 本地变换
        if (parent() != nullptr)
            _gloabal    =   (parent()->_gloabal * _local);
        else
            _gloabal    =   _local;
        
        if (!recursion || children().empty())
            return;
        auto&   chs =   children();
        for (auto& child : chs)
        {
            auto    node    =   child->as<FENode>();
            node->flags().addFlag(FLAG_PROP_TRANS | FLAG_PROP_SCALE | FLAG_PROP_ROT);
            node->updateTransform(recursion);
        }
        /// 处理约束
        /// solve(tm);
    }

    size_t  FENode::intersect(const Ray& ray,Pickups& results) const
    {
        size_t  nOld    =   results.size();
        if (_aabb.isNull())
            return  0;
        if(!ray.intersects(_aabb).first)
            return  0;
        if (_mesh)
        {
            FEPickup    result  =   {};
            if(_mesh->intersect(ray,_gloabal.toMatrix(),result))
            {
                result.object   =   const_cast<FENode*>(this);
                result.point    =   ray.getPoint(result.time);
                results.emplace_back(result);
            }
        }
            
        /// 递归所有子孙节点
        auto&   chs  =   children();
        for (auto& child: chs)
        {
            auto    node    =   child->as<FENode>();
            if (node)
                node->intersect(ray,results);
        }
        return  results.size() - nOld;
    }

    size_t  FENode::intersect(const Ray& ray,FEPickup& result) const
    {
        if (_aabb.isNull())
            return  0;
        if(!ray.intersects(_aabb).first)
            return  0;
        if (_mesh)
        {
            _mesh->intersect(ray,_gloabal.toMatrix(),result);
        }
        /// 递归所有子孙节点
        auto&   chs  =   children();
        for (auto& child: chs)
        {
            auto    node    =   child->as<FENode>();
            if (node)
                node->intersect(ray,result);
        }
        return  result.object != nullptr ? 1: 0;
    }
    size_t  FENode::queryDepends(ObjectUSet& uset) const 
    {
        const auto  vSize   =   uset.size();
        FEObject::queryDepends(uset);
        for (auto var : _coms)
        {
            uset.emplace(var);
        }
        Object  mesh    =   const_cast<FEMesh*>(_mesh.get());
        Object  mat     =   const_cast<FEMaterial*>(_material.get());
        if (mesh)   uset.emplace(mesh);
        if (mat)    uset.emplace(mat);

        return  uset.size() - vSize;
    }
    void    FENode::beginSetProp() 
    {}

    bool    FENode::setProperty(int prop,const KFValue& value) 
    {
        
        switch(prop)
        {
        case PROP_TRANSFORM_X:
            _local._position.x    =   std::get<real>(value);
            flags().addFlag(FLAG_PROP_TRANS);
            return  true;
        case PROP_TRANSFORM_Y:
            _local._position.y    =   std::get<real>(value);
            flags().addFlag(FLAG_PROP_TRANS);
            return  true;
        case PROP_TRANSFORM_Z:
            _local._position.z    =   std::get<real>(value);
            flags().addFlag(FLAG_PROP_TRANS);
            return  true;
        case PROP_TRANSFORM_XYZ:
            _local._position      =   std::get<real3>(value);
            flags().addFlag(FLAG_PROP_TRANS);
            return  true;
        case PROP_SCALE_X:
            _local._scale.x    =   (float)std::get<real>(value);
            flags().addFlag(FLAG_PROP_SCALE);
            return  true;
        case PROP_SCALE_Y:
            _local._scale.y    =   (float)std::get<real>(value);
            flags().addFlag(FLAG_PROP_SCALE);
            return  true;
        case PROP_SCALE_Z:
            _local._scale.z    =   (float)std::get<real>(value);
            flags().addFlag(FLAG_PROP_SCALE);
            return  true;
        case PROP_SCALE_XYZ:
            _local._scale      =   std::get<real3>(value);
            flags().addFlag(FLAG_PROP_SCALE);
            return  true;
        /// 欧拉角实现:写入累计欧拉角账本,不做 ±180° 归一化
        case PROP_ROTATE_X:
            {
                real3   euler   =   localEuler();
                euler.x         =   std::get<real>(value);
                _local.setEuler(float3(euler));
                flags().addFlag(FLAG_PROP_ROT);
            }
            return  true;
        case PROP_ROTATE_Y:
            {
                real3   euler   =   localEuler();
                euler.y         =   std::get<real>(value);
                _local.setEuler(float3(euler));
                flags().addFlag(FLAG_PROP_ROT);
            }
            return  true;
        case PROP_ROTATE_Z:
            {
                real3   euler   =   localEuler();
                euler.z         =   std::get<real>(value);
                _local.setEuler(float3(euler));
                flags().addFlag(FLAG_PROP_ROT);
            }
            return  true;
        case PROP_ROTATE_XYZ:
            _local.setEuler(float3(std::get<real3>(value)));
            flags().addFlag(FLAG_PROP_ROT);
            return  true;
        case PROP_QUAT:
            flags().addFlag(FLAG_PROP_ROT); 
            _local.setRotation(std::get<quatf>(value));
            return  true;
         
        case PROP_COLOR_RGB:
            {
                auto    color   =   std::get<float3>(value);
                _color._value.r =   (uint8)(color.r * 255.0f);
                _color._value.g =   (uint8)(color.g * 255.0f);
                _color._value.b =   (uint8)(color.b * 255.0f);
                flags().addFlag(FLAG_PROP_COLOR);
            }
            return  true;
        case PROP_COLOR_ALPHA:
            {
                auto    alpha   =   std::get<float>(value);
                _color._value.a =   (uint8)(alpha * 255.0f);
                flags().addFlag(FLAG_PROP_COLOR);
            }
            return  true;
        case PROP_VISIBLE:
            /// 显隐由 FLAG_VISIBLE 控制,变化后需要通知渲染刷新
            if (std::get<uint8>(value) != 0)
                flags().addFlag(FLAG_VISIBLE | FLAG_UPDATE);
            else
                flags().removeFlag(FLAG_VISIBLE);
            flags().addFlag(FLAG_PROP_STATE);
            return  true;
        default:
            assert(0!=0);
            return  false;
        }
    }

    KFValue FENode::getProperty(int prop) const
    {
        switch(prop)
        {
        case PROP_TRANSFORM_X:      return  _local._position.x;
        case PROP_TRANSFORM_Y:      return  _local._position.y;
        case PROP_TRANSFORM_Z:      return  _local._position.z;
        case PROP_TRANSFORM_XYZ:    return  _local._position;
        case PROP_SCALE_X:          return  _local._scale.x;
        case PROP_SCALE_Y:          return  _local._scale.y;
        case PROP_SCALE_Z:          return  _local._scale.z;
        case PROP_SCALE_XYZ:        return  _local._scale;
        /// localEuler() 已经是角度(度),不能再做一次 RAD2DEG(会放大 57 倍)
        case PROP_ROTATE_X:         return  localEuler().x;
        case PROP_ROTATE_Y:         return  localEuler().y;
        case PROP_ROTATE_Z:         return  localEuler().z;
        case PROP_ROTATE_XYZ:       return  localEuler();
        case PROP_QUAT:             return  _local.rotation();
        case PROP_COLOR_RGB:        return  _color.value();
        case PROP_COLOR_ALPHA:      return  _color.value().a;
        case PROP_VISIBLE:          return  uint8(flags().hasFlag(FLAG_VISIBLE) ? 1 : 0);
        case PROP_G_TRANSFORM_XYZ:  return  globalTranslation();
        default:
            assert(0!=0);
            return  {};
        }
    }
    void    FENode::endSetProp(const real& tm,bool bModify)
    {
        UNUSED(bModify);
        update(tm);
        if (bModify)
        {
            fireChanged();
        }
    }

    void    FENode::solve(const real& tm)
    {
        for (auto& var : _coms)
        {
            auto    constr  =   dynamic_cast<FEConstraint*>(var.get());
            if (constr == nullptr)
                continue;
            FETransform solved  =   constr->solve(_gloabal, tm);
            setGlobalTranslation(solved.position());
            setGlobalRotation(quatr(solved.rotation()));
        }
    }

    size_t  FENode::objectCount() const 
    {
        return  children().size() + _coms.size() + (_mesh ? 1 : 0) + (_material ? 1 : 0);
    }
    bool    FENode::traverseObject(const ObjectVisitor& fun,uint depth,bool recur) const 
    {
        if (!fun)
        {
            return  false;
        }
        FETrvsCtx trvsCtx(fun);

        if (_mesh)
        {
            if(!fun(*_mesh,*this,trvsCtx,depth))
                return  false;
            if(recur && _mesh->objectCount()!= 0)
                _mesh->traverseObject(fun,depth+1,recur);
        }
        if (_material)
        {
            if(!fun(*_material,*this,trvsCtx,depth))
                return  false;
            if(recur && _material->objectCount()!= 0)
                _material->traverseObject(fun,depth+1,recur);
        }
        for (auto&  var : _coms)
        {
            if(!fun(*var,*this,trvsCtx,depth))
                return  false;
            if(recur && var->objectCount()!= 0)
                var->traverseObject(fun,depth+1,recur);
        }
        auto&   cs      =   children();
        for (auto&  var : cs)
        {
            if(!fun(*var,*this,trvsCtx,depth))
                return  false;
            if(recur && var->objectCount()!= 0)
                if (!var->traverseObject(fun,depth+1,recur))
                    return  false;
        }
        return  true;
    }


}

