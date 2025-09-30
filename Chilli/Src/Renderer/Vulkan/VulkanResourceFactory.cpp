#include "ChV_PCH.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan/vulkan.h"
#include "VulkanDevice.h"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
#include "VulkanRenderer.h"
#include "VulkanResourceFactory.h"

namespace Chilli
{
    VulkanResourceFactory::VulkanResourceFactory(const VulkanResourceFactorySpec& Spec)
    {
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = Spec.Device->GetPhysicalDevice()->PhysicalDevice;
        allocatorInfo.device = Spec.Device->GetHandle();
        allocatorInfo.instance = Spec.Instance;
        _Device = Spec.Device;
        _SwapChain = Spec.SwapChain;

        VULKAN_SUCCESS_ASSERT(vmaCreateAllocator(&allocatorInfo, &_Allocator), "VMA Allocator failed to initialize!");
        VULKAN_PRINTLN("VMA Created!!");
        VULKAN_PRINTLN("Resource Factory Created!")
    }

    void VulkanResourceFactory::Destroy()
    {
        vmaDestroyAllocator(_Allocator);
        VULKAN_PRINT("Resource Factory Desytoyed!")
    }

    Ref<GraphicsPipeline> VulkanResourceFactory::CreateGraphicsPipeline(const GraphicsPipelineSpec& Spec)
    {
        VulkanGraphicsPipelineSpec VulkanSpec{}; 
        VulkanSpec.Paths[0] = Spec.Paths[0];
        VulkanSpec.Paths[1] = Spec.Paths[1];
        VulkanSpec.Device = this->_Device->GetHandle();
        VulkanSpec.SwapChainFormat = _SwapChain->GetFormat();

        return std::make_shared<VulkanGraphicsPipeline>(VulkanSpec);
    }

    void VulkanResourceFactory::DestroyGraphicsPipeline(Ref<GraphicsPipeline>& Pipeline)
    {
        auto VulkanPipeline = std::static_pointer_cast<VulkanGraphicsPipeline>(Pipeline);
        VulkanPipeline->Destroy(_Device->GetHandle());
    }

    void _ReadFile(const char* FilePath, std::vector<char>& CharData)
    {
        std::ifstream file(FilePath, std::ios::ate | std::ios::binary);

        if (!file.is_open())
            throw std::runtime_error("failed to open file!");

        size_t fileSize = (size_t)file.tellg();
        CharData.resize(fileSize);

        file.seekg(0);
        file.read(CharData.data(), fileSize);
        file.close();
    }

    VkShaderModule _CreateShaderModule(VkDevice device, std::vector<char>& code)
    {
        VkShaderModuleCreateInfo createinfo{};
        createinfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createinfo.codeSize = code.size();
        createinfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createinfo, nullptr, &shaderModule) != VK_SUCCESS)
            throw std::runtime_error("failed to create shader module!");

        return shaderModule;
    }

    VkPipelineShaderStageCreateInfo _CreateShaderStages(VkShaderModule module, VkShaderStageFlagBits type, const char* Name)
    {
        VkPipelineShaderStageCreateInfo stage{};
        stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stage.module = module;
        stage.pName = Name;
        stage.stage = type;

        return stage;
    }

    VulkanGraphicsPipeline::VulkanGraphicsPipeline(const VulkanGraphicsPipelineSpec& Spec)
    {
        Init(Spec);
    }

    void VulkanGraphicsPipeline::Destroy(VkDevice Device)
    {
        vkDestroyPipelineLayout(Device, _PipelineLayout, nullptr);
        vkDestroyPipeline(Device, _Pipeline, nullptr);
    }

    void VulkanGraphicsPipeline::Init(const VulkanGraphicsPipelineSpec& Spec)
    {
        std::vector<char> VertShaderCode, FragShaderCode;
        _ReadFile(Spec.Paths[0].c_str(), VertShaderCode);
        _ReadFile(Spec.Paths[1].c_str(), FragShaderCode);
        // Create shader modules
        VkShaderModule vertShaderModule = _CreateShaderModule(Spec.Device, VertShaderCode);
        VkShaderModule fragShaderModule = _CreateShaderModule(Spec.Device, FragShaderCode);

        // Shader stages
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages(2);
        shaderStages[0] = _CreateShaderStages(vertShaderModule, VK_SHADER_STAGE_VERTEX_BIT, "main");
        shaderStages[1] = _CreateShaderStages(fragShaderModule, VK_SHADER_STAGE_FRAGMENT_BIT, "main");

        // Define vertex input state - no bindings since vertices are in shader
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.pVertexBindingDescriptions = nullptr;
        vertexInputInfo.vertexAttributeDescriptionCount = 0;
        vertexInputInfo.pVertexAttributeDescriptions = nullptr;

        // Input assembly
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // Viewport and scissor (dynamic)
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        // Rasterizer
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        // Multisampling
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // Color blending
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;

        // Dynamic state
        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        // Dynamic rendering
        VkPipelineRenderingCreateInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachmentFormats = &Spec.SwapChainFormat;

        // Create pipeline layout
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pSetLayouts = nullptr;
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;

        vkCreatePipelineLayout(Spec.Device, &pipelineLayoutInfo, nullptr, &_PipelineLayout);

        // Create graphics pipeline
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.pNext = &renderingInfo; // Dynamic rendering
        pipelineInfo.stageCount = shaderStages.size();
        pipelineInfo.pStages = shaderStages.data();
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = nullptr;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = _PipelineLayout;
        pipelineInfo.renderPass = VK_NULL_HANDLE; // No render pass with dynamic rendering
        pipelineInfo.subpass = 0;

        vkCreateGraphicsPipelines(Spec.Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_Pipeline);

        // Cleanup shader modules
        vkDestroyShaderModule(Spec.Device, vertShaderModule, nullptr);
        vkDestroyShaderModule(Spec.Device, fragShaderModule, nullptr);
    }
}
