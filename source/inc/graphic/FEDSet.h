#pragma     once

#include    "RSObject.h"
#include    "FEDSetLayout.h"
#include    "FEGraphicEnums.h"

namespace   FE
{
    class   FEDSetPool;
    using   DSetPool    =   SharedPtr<FEDSetPool>;

    struct  Binding
    {
        uint16_t    _binding    =   0;
        uint64s     _offsets    =   {};
        uint64s     _ranges     =   {};
        FEDescType  _type       =   DT_UNIFORM_BUFFER;
        String      _name;
        String      _typeName;
        Objects     _objects;
    };

    using   Bindings    =   std::vector<Binding>;
    class   FEDSet :public RSObject
    {
    public:
        struct  CreateInfo
        {
            DSetLayout  _layout;
            DSetPool    _pool;
            Bindings    _binds;
            uint        _set    =   0;
        };
    public:
        FEDSet(FEContext& ctx)
            :RSObject(ctx)
        {}
        FEDSet(const FEDSet& other)
            :RSObject(other)
        {
            _cInfo  =   other._cInfo;
        }
        /// <summary>
        /// 获取创建信息
        /// </summary>
        /// <returns></returns>
        const   auto&   cInfo() const
        {
            return  _cInfo;
        }
        virtual bool        create(const CreateInfo& info)  =   0;
        virtual bool        update()    =   0;
        /// <summary>
        /// 查询 Binding
        /// </summary>
        /// <param name="binding"></param>
        /// <returns></returns>
        inline  Binding*    queryBindings(uint16 binding)
        {
            for (auto& var: _cInfo._binds)
            {
                if (var._binding != binding)
                    continue;
                else
                    return  &var;
            }
            return  nullptr;
        }
        /// <summary>
        /// 关联对象
        /// </summary>
        /// <param name="binding"></param>
        /// <param name="objects"></param>
        /// <returns></returns>
        inline  bool        setBinding(uint16 binding,const Objects& objects,const uint64s& ranges = {},const uint64s& offsets = {})
        {
            Binding*    pBind   =   queryBindings(binding);
            if (pBind == nullptr)
                return  false;
            
            pBind->_objects =   objects;
            pBind->_offsets =   offsets;
            pBind->_ranges  =   ranges;
            _dirty  =   true;
            return  true;
        }
    protected:
        CreateInfo  _cInfo;
        bool        _dirty  =   false;
    };


    using  DSet     =   SharedPtr<FEDSet>;
    using  DSets    =   std::vector<DSet>;
}
