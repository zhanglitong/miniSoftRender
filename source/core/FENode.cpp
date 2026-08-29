
#include    "../inc/node/FENode.hpp"
#include    "../inc/graphic/FEScene.h"
#include    "../inc/FEPropertyIndex.hpp"
#include    "../inc/FEEulerObject.hpp"

namespace   FE
{

    FENode::FENode(FEContext& ctx)
        :FEItem<FENode>(ctx)
        ,_dataFlag(0)
    {
        _scale      =   float3(1,1,1);
        _trans      =   real3(0,0,0);
        _rotate     =   quatf(1,0,0,0);
        /// 默认情况下颜色会 color x fragment 
        _color      =   Rgba8(255,255,255,255);
        _transform  =   FE::makeTransform<real>(_trans,_scale,_rotate);
        _renderBits =   RF_VISIBLE;
    }

    FENode::FENode(const FENode& other)
        :FEItem<FENode>(other)
    {
        _trans      =   other._trans        ;
        _scale      =   other._scale        ;
        _rotate     =   other._rotate       ;
        _renderBits =   other._renderBits   ;
        _aabb       =   other._aabb         ;

        _transform  =   other._transform    ;
        _material   =   other._material     ; 
        _mesh       =   other._mesh         ; 
        _dataFlag   =   other._dataFlag     ;
        _color      =   other._color        ;
        _name       =   other._name         ;
        _coms       =   FEObjectHelper::clone(other._coms);
    }

    void    FENode::update(const real& tmDelta)
    {
        if (!flags().hasFlags(ModifyValue))
            return;
        updateTransform(true);
        updateAabb(true);
        for (auto& var: _coms)
        {
            if (!var->isEnable() || var->isActor())
                continue;
            else
                var->update(tmDelta);
        }
    }
    void   FENode::fireChanged()
    {
        if ( flags().hasFlags(ModifyValue) )
        {   
            if (_ctx.scene() == nullptr)
                return;
            _ctx.scene()->onNodePropChanged(this);
            auto&   childs  =   _childs;
            for (auto& var : _childs)
            {
                var->fireChanged();
            }
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
        auto    pParent =   parent() ? parent()->as<FENode>() : nullptr;
        if (pParent != nullptr)
            _transform  =   pParent->_transform * makeTransform(_trans, real3(_scale), FE::quatr(_rotate));
        else
            _transform  =   makeTransform(_trans, real3(_scale), FE::quatr(_rotate));

        if (!recursion || children().empty() )
            return ;
        auto&   chs =   children();
        for(auto& child : chs)
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
            if(_mesh->intersect(ray,_transform,result))
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
            _mesh->intersect(ray,_transform,result);
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
        return  uset.size() - vSize;
    }

    void    FENode::beginSetProp() 
    {}

    bool    FENode::setProperty(int prop,const KFValue& value) 
    {
        UNUSED(prop);
        UNUSED(value);
        switch(prop)
        {
        case PROP_TRANSFORM_X:
            _trans.x    =   std::get<real>(value);
            flags().addFlag(FLAG_PROP_TRANS);
            return  true;
        case PROP_TRANSFORM_Y:
            _trans.y    =   std::get<real>(value);
            flags().addFlag(FLAG_PROP_TRANS);
            return  true;
        case PROP_TRANSFORM_Z:
            _trans.z    =   std::get<real>(value);
            flags().addFlag(FLAG_PROP_TRANS);
            return  true;
        case PROP_TRANSFORM_XYZ:
            _trans      =   std::get<real3>(value);
            flags().addFlag(FLAG_PROP_TRANS);
            return  true;

        case PROP_SCALE_X:
            _scale.x    =   (float)std::get<float>(value);
            flags().addFlag(FLAG_PROP_SCALE);
            return  true;
        case PROP_SCALE_Y:
            _scale.y    =   (float)std::get<float>(value);
            flags().addFlag(FLAG_PROP_SCALE);
            return  true;
        case PROP_SCALE_Z:
            _scale.z    =   (float)std::get<float>(value);
            flags().addFlag(FLAG_PROP_SCALE);
            return  true;
        case PROP_SCALE_XYZ:
            _scale      =   std::get<float3>(value);
            flags().addFlag(FLAG_PROP_SCALE);
            return  true;

        case PROP_ROTATE_X:
        case PROP_ROTATE_Y:
        case PROP_ROTATE_Z:
        case PROP_ROTATE_XYZ:
            return  false;
        case PROP_QUAT:
            flags().addFlag(FLAG_PROP_ROT); 
            _rotate     =   std::get<quatf>(value);
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

    void    FENode::endSetProp(bool bModify)
    {
        UNUSED(bModify);
        if (bModify)
        {
            update();
            fireChanged();
        }
    }
}

