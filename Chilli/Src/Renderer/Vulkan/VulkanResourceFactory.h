#pragma once

#include "ResourceFactory.h"

namespace Chilli
{
    struct VulkanResourceFactorySpec
    {
        VulkanDevice* Device;
        VkInstance Instance;
    };

    struct VulkanGraphicsPipelineSpec
    {
    };

    class VulkanGraphicsPipeline : public GraphicsPipeline
    {
    public:
        VulkanGraphicsPipeline(const VulkanGraphicsPipelineSpec& Spec);
        ~VulkanGraphicsPipeline() {}

        void Destroy();
        void Init(const VulkanGraphicsPipelineSpec& Spec);

        virtual void Bind() override{}
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

        virtual Ref<GraphicsPipeline> CreateGraphicsPipeline() override;
        virtual void DestroyGraphicsPipeline(Ref<GraphicsPipeline>& Pipeline) override;

    private:
        VmaAllocator _Allocator;
    };
} // namespace VEngine
