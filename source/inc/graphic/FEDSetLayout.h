#pragma     once

#include    "RSObject.h"
#include    "FEGraphicEnums.h"

namespace   FE
{
    class   FEDSet;
    using   DSet    =   SharedPtr<FEDSet>;

    struct  FEDSetBinding
    {
        uint16_t        _binding;
        uint16_t        _descriptorCount;
        /// <summary>
        /// 描述UBO大小
        /// </summary>
        uint32_t        _size;
        /// <summary>
        /// 类型
        /// </summary>
        FEDescType      _descriptorType;
        ShaderTypes     _stageFlags;
        /// <summary>
        /// 实例名称
        /// </summary>
        String          _name;
        /// <summary>
        /// 类型名称,唯一标识,材质中使用它建立对应关系
        /// </summary>
        String          _typeName;
    };
    using   FEDSetBindings  =   std::vector<FEDSetBinding>;
    class   FEDSetLayout :public RSObject
    {
    public:
        struct  CreateInfo
        {
            uint32_t        _setNumber     =   0;
            FEDSetBindings  _bindings;
        };
    public:
        FEDSetLayout(FEContext& ctx)
            :RSObject(ctx)
        {}
        FEDSetLayout(const FEDSetLayout& other)
            :RSObject(other)
        {
        }
        const   auto&   cInfo() const
        {
            return  _cInfo;
        }
        virtual bool    create(const CreateInfo& info)  =   0;

        virtual DSet    createDSet()    =   0;
    protected:
        CreateInfo  _cInfo;
    };

    using   DSetLayout  =   SharedPtr<FEDSetLayout>;
    using   DSetLayouts =   std::vector<DSetLayout>;
}
