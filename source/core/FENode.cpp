
#include    "../inc/node/FENode.hpp"
#include    "../inc/graphic/FEScene.h"

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
        if (!flags().hasFlag(FLAG_UPDATE))
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
        /// 清除标记
        /// flags().removeFlags(ModifyValue);
    }
    void   FENode::fireChanged()
    {
        if ( flags().hasFlags(FLAG_PROP_TRANS | FLAG_PROP_SCALE | FLAG_PROP_ROT) )
        {
            if (_ctx.scene())
            {
                _ctx.scene()->onNodePropChanged(this);
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
            tmp.merge(node->updateAabb(true));
        }
        _aabb.merge(tmp);
        return  _aabb;
    }
    
    void    FENode::updateTransform(bool recursion)
    {
        if (!flags().hasFlag(FLAG_UPDATE))
            return;
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
            node->flags().addFlag(FLAG_UPDATE);
            node->updateTransform(true);
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
}

