#pragma once

namespace Chilli
{
    class VulkanSwapChainKHR
    {
    public:
        VulkanSwapChainKHR() {}
        ~VulkanSwapChainKHR() {}

        void Destroy(VulkanDevice& device);
        void Init(VulkanDevice& device, VkSurfaceKHR SurfaceKHR, int Width, int Height);
        void Recreate(VulkanDevice& device, VkSurfaceKHR SurfaceKHR, int Width, int Height);

        VkFormat GetFormat() const { return _Format; }
        VkExtent2D GetExtent() const { return _Extent; }
        const std::vector<VkImageView> GetImageViewss() const { return _ImageViews; }

    private:
        void _CreateSwapChainKHR(VulkanDevice& device, VkSurfaceKHR SurfaceKHR, int Width, int Height);
        void _CreateSwapChainImageViews(VulkanDevice& device);

    private:
        VkSwapchainKHR _SwapChain;
        VkFormat _Format;
        VkExtent2D _Extent;

        std::vector<VkImage> _Images;
        std::vector<VkImageView> _ImageViews;
    };
} // namespace VEngine
