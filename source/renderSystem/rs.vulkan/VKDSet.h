#pragma     once

#include    "graphic/FEDSet.h"

#include    "FEVulkan.h"

namespace   FE
{
    class   VKDSet :public TRSObject<VkDescriptorSet,FEDSet>
    {
        using   VKWDSs  =   std::vector<VkWriteDescriptorSet>;
        using   DSBIs   =   std::vector<VkDescriptorBufferInfo>;
    public:
        VKDSet(FEContext& ctx)
            :TRSObject<VkDescriptorSet,FEDSet>(ctx)
        {
        }
        VKDSet(const VKDSet& other)
            :TRSObject<VkDescriptorSet,FEDSet>(other)
        {}
        virtual~VKDSet();

        virtual bool    create(const CreateInfo& cInf)  override;
        virtual bool    update()    override;
    protected:
        VKWDSs  _wdss;
        DSBIs   _buffers;
    };
}
