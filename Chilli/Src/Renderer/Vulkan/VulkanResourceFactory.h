#pragma once

#include "ResourceFactory.h"

namespace Chilli
{
    struct VulkanResourceFactorySpec
    {
        VulkanDevice* Device;
        VulkanSwapChainKHR* SwapChain;
        VkInstance Instance;
    };

    struct VulkanGraphicsPipelineSpec
    {
        std::string Paths[2];
        VkDevice Device;
        VkFormat SwapChainFormat;
    };

    class VulkanGraphicsPipeline : public GraphicsPipeline
    {
    public:
        VulkanGraphicsPipeline(const VulkanGraphicsPipelineSpec& Spec);
        ~VulkanGraphicsPipeline() {}

        void Destroy(VkDevice Device);
        void Init(const VulkanGraphicsPipelineSpec& Spec);

        virtual void Bind() override{}

        VkPipeline GetHandle() const { return _Pipeline; }
    private:
        VkPipeline _Pipeline;
        VkPipelineLayout _PipelineLayout;
    };

    class VulkanResourceFactory : public ResourceFactory
    {
    public:
        VulkanResourceFactory(const VulkanResourceFactorySpec& Spec);
        virtual ~VulkanResourceFactory() {}

        void Destroy();

        virtual Ref<GraphicsPipeline> CreateGraphicsPipeline(const GraphicsPipelineSpec& Spec) override;
        virtual void DestroyGraphicsPipeline(Ref<GraphicsPipeline>& Pipeline) override;

    private:
        VmaAllocator _Allocator;
        VulkanDevice* _Device;
        VulkanSwapChainKHR* _SwapChain;
    };
} // namespace VEngine
