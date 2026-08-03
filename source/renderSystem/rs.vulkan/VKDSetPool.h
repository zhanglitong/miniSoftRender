#pragma     once

#include    "graphic/FEDSetPool.h"

#include    "FEVulkan.h"

namespace   FE
{
    class   VKDSetPool :public TRSObject<VkDescriptorPool,FEDSetPool>
    {
    public:
        VKDSetPool(FEContext& ctx)
            :TRSObject<VkDescriptorPool,FEDSetPool>(ctx)
        {
        }
        VKDSetPool(const VKDSetPool& other)
            :TRSObject<VkDescriptorPool,FEDSetPool>(other)
        {}

        virtual ~VKDSetPool();

        virtual bool    create(const CreateInfo& cInf)  override;
        virtual bool    hasFree() 
        {
            return  _hasAlloc < _cInfo._maxSets;
        }
        virtual void    alloc()
        {
            ++_hasAlloc;
        }
        virtual void    free()
        {
            --_hasAlloc;
        }
    protected:
        /// <summary>
        /// �Ѿ�����
        /// </summary>
        uint32_t    _hasAlloc       =   0;
    };

    using   DSetPoolVK  =   SharedPtr<VKDSetPool>;


}