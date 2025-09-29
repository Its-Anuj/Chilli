#pragma once

namespace Chilli
{
    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    enum QueueFamilies
    {
        GRAPHICS,
        PRESENT,
        TRANSFER,
        COMPUTE,
        COUNT
    };

    struct QueueFamilyIndicies
    {
        std::optional<uint32_t> Queues[int(QueueFamilies::COUNT)];

        inline bool Complete() const {
            return Queues[int(QueueFamilies::GRAPHICS)].has_value() &
                Queues[int(QueueFamilies::PRESENT)].has_value() & Queues[int(QueueFamilies::TRANSFER)].has_value();
        }
    };

    struct VulkanPhysiscalDeviceInfo
    {
        VkPhysicalDeviceProperties properties;
        VkPhysicalDeviceFeatures features;
        VkPhysicalDeviceVulkan11Features features11; // Vulkan 1.1
        VkPhysicalDeviceVulkan12Features features12; // Vulkan 1.2
        VkPhysicalDeviceVulkan13Features features13; // Vulkan 1.3
        VkPhysicalDeviceMemoryProperties memoryProperties;
        QueueFamilyIndicies QueueIndicies;
        SwapChainSupportDetails SwapChainDetails;
        bool SupportsExtensions = true;
    };

    struct VulkanPhysicalDevice
    {
        VkPhysicalDevice PhysicalDevice;
        VulkanPhysiscalDeviceInfo Info;
    };

    void _FillVulkanPhysicalDevice(VulkanPhysicalDevice& Physicaldevice, VkSurfaceKHR SurfaceKHR);
    bool CheckVulkanPhysicalDeviceExtensions(VulkanPhysicalDevice& Physicaldevice, std::vector<const char*>& ReExts);

    class VulkanDevice
    {
    public:
        VulkanDevice() {}
        ~VulkanDevice() {}

        void Init(VulkanPhysicalDevice* PDevice, std::vector<const char*>& ReqExts);
        void Destroy();

        inline VkQueue GetQueue(QueueFamilies Queuefamily) const { return _Queues[int(Queuefamily)]; }
        inline VkDevice GetHandle() const { return _Device; }
        inline VulkanPhysicalDevice* GetPhysicalDevice() const { return _UsedPhysicalDevice; }

    private:
        VkDevice _Device;
        VkQueue _Queues[int(QueueFamilies::COUNT)];
        VulkanPhysicalDevice* _UsedPhysicalDevice;
        // All like pipeline everythign resources created is tied to this
    };
}
