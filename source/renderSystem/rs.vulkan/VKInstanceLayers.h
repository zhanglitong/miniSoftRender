#pragma     once


#include    "FEVulkan.h"
namespace   FE
{
    using   LayerProps      =   std::vector<VkLayerProperties>;

    class   VKInstanceLayers
    {
    public:
        LayerProps  _layerInfors;
    public:
        VKInstanceLayers()
        {
            uint allLayerCount = 0;
            vkEnumerateInstanceLayerProperties(&allLayerCount, nullptr);

            if (allLayerCount > 0)
            {
                _layerInfors.resize(allLayerCount);
                vkEnumerateInstanceLayerProperties(&allLayerCount, _layerInfors.data());
            }
        }
        ~VKInstanceLayers()
        {}

        bool        hasLayer(const char *name) const
        {
            for (auto& var : _layerInfors)
            {
                if (0 == strcmp(var.layerName, name))
                    return true;
            }
            return false;
        }

        PCSTRs      defaults()
        {
            PCSTRs  layers;
            if (hasLayer("VK_LAYER_KHRONOS_validation"))
            {
                layers.push_back("VK_LAYER_KHRONOS_validation");
            }
            else if (hasLayer("VK_LAYER_LUNARG_standard_validation"))
            {
                layers.push_back("VK_LAYER_LUNARG_standard_validation");
            }
            return  layers;
        }
    };
}