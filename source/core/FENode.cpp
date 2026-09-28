
#include    "../inc/node/FENode.hpp"
#include    "../inc/graphic/FEScene.h"
#include    "../inc/FEPropertyIndex.hpp"
#include    "../inc/FEEulerObject.hpp"

namespace   FE
{

    FENode::FENode(FEContext& ctx)
        :FEItem<FENode>(ctx)
    {
        _local._scale       =   float3(1,1,1);
        _local._position    =   real3(0,0,0);
        _local._rotation    =   quatf(1,0,0,0);
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

    void    FENode::update()
    {
        if (!flags().hasFlags(ModifyValue))
            return;
        updateTransform(true);
        updateAabb(true);
    }
    void    FENode::fireChanged()
    {
        if ( flags().hasFlags(ModifyValue) )
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
        _ctx.scene()->nodeTree().eventsAddNode().fireNotify(node);
    }
    void    FENode::onRemoveChild(Node node) 
    {
        flags().addFlag(FLAG_REMOVE_CHILD);
        _ctx.scene()->nodeTree().eventsRemoveNode().fireNotify(node);
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
    
    void    FENode::updateTransform(bool recursion)
    {
        FETransform blended;
        blend(blended);

        /// 全局变换 = 父全局 * (局部 * 混合组件变换)
        if (parent() != nullptr)
            _gloabal    =   (parent()->_gloabal * (_local * blended));
        else
            _gloabal    =   (_local * blended);

        if (!recursion || children().empty())
            return;
        auto&   chs =   children();
        for (auto& child : chs)
        {
            auto    node    =   child->as<FENode>();
            node->flags().addFlag(FLAG_PROP_TRANS | FLAG_PROP_SCALE | FLAG_PROP_ROT);
            node->updateTransform(recursion);
        }
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
        /// 欧拉角实现
        case PROP_ROTATE_X:
        case PROP_ROTATE_Y:
        case PROP_ROTATE_Z:
        case PROP_ROTATE_XYZ:
            return  false;
        case PROP_QUAT:
            flags().addFlag(FLAG_PROP_ROT); 
            _local._rotation    =   std::get<quatf>(value);
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
        case PROP_ROTATE_X:         return  RAD2DEG(quatToEuler(_local._rotation).x);
        case PROP_ROTATE_Y:         return  RAD2DEG(quatToEuler(_local._rotation).y);
        case PROP_ROTATE_Z:         return  RAD2DEG(quatToEuler(_local._rotation).z);
        case PROP_ROTATE_XYZ:       return  quatToEuler(_local._rotation);
        case PROP_QUAT:             return  _local._rotation;
        case PROP_COLOR_RGB:        return  _color.value();
        case PROP_COLOR_ALPHA:      return  _color.value().a;
        case PROP_G_TRANSFORM_XYZ:  return  globalTranslation();
        default:
            assert(0!=0);
            return  {};
        }
    }
    void    FENode::endSetProp(bool bModify)
    {
        UNUSED(bModify);
        update();
        if (bModify)
        {
            fireChanged();
        }
    }

    void    FENode::blend(FETransform& blended)
    {
        /// 第一遍: 计算权重总和 (weight <= 0 不参与)
        float   allW    =   0;
        int     cnt     =   0;
        for (auto& var : _coms)
        {
            if (!var->getTransform())   continue;
            float   w   =   var->weight();
            if (w <= 0.0f)              continue;
            allW    +=  w;
            ++cnt;
        }

        /// 计算混合后的组件变换
        if (cnt == 1)
        {
            /// 单组件: 直接使用
            for (auto& var : _coms)
            {
                auto*   tf  =   var->getTransform();
                if (tf) { blended = *tf; break; }
            }
        }
        else if (cnt > 1)
        {
            /// 多组件: 按权重混合 position/scale/rotation
            real3   pos(0);
            float3  scl(0);
            quatf   rot(0,0,0,0);
            for (auto& var : _coms)
            {
                auto*   tf  =   var->getTransform();
                if (!tf)        continue;
                float   w   =   var->weight();
                if (w <= 0.0f)  continue;
                float   wn  =   w / allW;
                pos     +=  tf->position() * real(wn);
                scl     +=  tf->scale()    * wn;
                rot     +=  tf->rotation() * wn;
            }
            blended.setPosition(pos);
            blended.setScale(scl);
            blended.setRotation(FE::normalize(rot));
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

