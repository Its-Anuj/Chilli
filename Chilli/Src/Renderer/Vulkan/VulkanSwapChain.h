#pragma once

namespace Chilli
{
    class VulkanSwapChainKHR
    {
    public:
        VulkanSwapChainKHR() {}
        ~VulkanSwapChainKHR() {}

        void Destroy(VulkanDevice& device);
        void Init(VulkanDevice& device, VkSurfaceKHR SurfaceKHR, int Width, int Height, bool VSync);
        void Recreate(VulkanDevice& device, VkSurfaceKHR SurfaceKHR, int Width, int Height, bool VSync);

        VkFormat GetFormat() const { return _Format; }
        VkExtent2D GetExtent() const { return _Extent; }
        const std::vector<VkImageView> GetImageViews() const { return _ImageViews; }
        const std::vector<VkImage> GetImages() const { return _Images; }

        VkSwapchainKHR GetHandle() { return _SwapChain; }

    private:
        void _CreateSwapChainKHR(VulkanDevice& device, VkSurfaceKHR SurfaceKHR, int Width, int Height, bool VSync);
        void _CreateSwapChainImageViews(VulkanDevice& device);

    private:
        VkSwapchainKHR _SwapChain;
        VkFormat _Format;
        VkExtent2D _Extent;

        std::vector<VkImage> _Images;
        std::vector<VkImageView> _ImageViews;
    };

    SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
} // namespace VEngine