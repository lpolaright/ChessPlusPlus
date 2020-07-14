#include <iostream>
#include <Windows.h>

#include <optional>
#include <iostream>
#include <string>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#define VK_USE_PLATFORM_WIN32_KHR
#define NDEBUG

#include <vulkan/vulkan.hpp>
#include "Renderer.h"

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;


static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity
                                                    , VkDebugUtilsMessageTypeFlagsEXT messageType
                                                    , const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData
                                                    , void *pUserData) {
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

int main() {
    // Initial loading of Vulkan instance
    vk::DynamicLoader dl;
    auto vkGetInstanceProcAddr =
            dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

    Renderer renderer = Renderer();

    vk::ApplicationInfo appInfo =
            vk::ApplicationInfo("Chess++", VK_MAKE_VERSION(1, 0, 0), "Snek", VK_MAKE_VERSION(1, 0, 0)
                                , VK_API_VERSION_1_2);
    if (!renderer.areExtensionsAvailable()) {
        std::cout << "Extensions are not available!" << std::endl;
    }

    const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
    vk::InstanceCreateInfo info = vk::InstanceCreateInfo({}, &appInfo, 1, &validationLayers[0]
                                                         , renderer.instance_wanted_extensions.size()
                                                         , &renderer.instance_wanted_extensions[0]);
    vk::Instance instance = vk::createInstance(info, nullptr);
    VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);

    vk::DebugUtilsMessengerEXT debugMessenger;
    vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo = vk::DebugUtilsMessengerCreateInfoEXT({},
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning,
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral, debugCallback);
    Renderer::printOrContinue(instance.createDebugUtilsMessengerEXT(&debugCreateInfo, nullptr, &debugMessenger));

    // Create Surface
    HMODULE hinstance = GetModuleHandle(nullptr);
    HWND hwnd = renderer.createWindow(hinstance);
    vk::Win32SurfaceCreateInfoKHR surfaceCreateInfo = vk::Win32SurfaceCreateInfoKHR({}, hinstance, hwnd);
    vk::SurfaceKHR surface;
    Renderer::printOrContinue(instance.createWin32SurfaceKHR(&surfaceCreateInfo, nullptr, &surface));

    // Getting a device and queue
    std::vector<vk::PhysicalDevice> physicalDevices =
            instance.enumeratePhysicalDevices();

    SelectedPhysicalDevice selectedDeviceProps = renderer.selectPhysicalDevice(physicalDevices, surface);
    vk::PhysicalDevice selectedDevice = selectedDeviceProps.selected_physical_device;
    std::vector<float> queuePriorities = {1.0f};
    std::vector<vk::DeviceQueueCreateInfo> deviceQueueCreateInfos;
    deviceQueueCreateInfos.push_back(vk::DeviceQueueCreateInfo({}
                                                               , selectedDeviceProps.graphics_queue_family_index
                                                               , 1, &queuePriorities[0]));
    if (selectedDeviceProps.graphics_queue_family_index != selectedDeviceProps.present_queue_family_index) {
        deviceQueueCreateInfos.push_back(vk::DeviceQueueCreateInfo({}
                                                                   , selectedDeviceProps.present_queue_family_index
                                                                   , 1, &queuePriorities[0]));
    }

    vk::DeviceCreateInfo deviceCreateInfo = vk::DeviceCreateInfo({}
                                                                 , static_cast<uint32_t>(deviceQueueCreateInfos.size())
                                                                 , &deviceQueueCreateInfos[0], 0, {}
                                                                 , static_cast<uint32_t>(renderer.device_wanted_extensions.size())
                                                                 , &renderer.device_wanted_extensions[0], {});
    vk::Device device = selectedDevice.createDevice(deviceCreateInfo);
    vk::Queue graphicsQueue = device.getQueue(selectedDeviceProps.graphics_queue_family_index, 0);
    vk::Queue presentQueue = device.getQueue(selectedDeviceProps.present_queue_family_index, 0);


    // Semaphore creation for sync
    vk::SemaphoreCreateInfo semaphoreCreateInfo = vk::SemaphoreCreateInfo();

    vk::Semaphore imageAvailableSemaphore;
    Renderer::printOrContinue(device.createSemaphore(&semaphoreCreateInfo, nullptr, &imageAvailableSemaphore));
    vk::Semaphore renderingFinishedSemaphore;
    Renderer::printOrContinue(device.createSemaphore(&semaphoreCreateInfo, nullptr
                                                     , &renderingFinishedSemaphore));

    const std::vector<vk::SurfaceFormatKHR> &surfaceFormats = selectedDevice.getSurfaceFormatsKHR(surface);
    const std::vector<vk::PresentModeKHR> &presentModes = selectedDevice.getSurfacePresentModesKHR(surface);
    vk::SurfaceCapabilitiesKHR surfaceCapabilities = selectedDevice.getSurfaceCapabilitiesKHR(surface);

    uint32_t image_count = renderer.getImageCount(surfaceCapabilities);
    vk::SurfaceFormatKHR surfaceFormat = renderer.getSurfaceFormat(surfaceFormats);
    vk::Extent2D swapchain_extent = renderer.getSwapChainExtent(surfaceCapabilities);
    vk::ImageUsageFlags image_usage_flags = renderer.getImageUsageFlags(surfaceCapabilities);
    vk::PresentModeKHR presentMode = renderer.getPresentMode(presentModes);
    vk::SwapchainKHR oldSwapchain;
    vk::SwapchainKHR newSwapchain;

    vk::SwapchainCreateInfoKHR swapchainCreateInfo = vk::SwapchainCreateInfoKHR(
            {}, surface, image_count, surfaceFormat.format, surfaceFormat.colorSpace, swapchain_extent, 1
            , image_usage_flags, vk::SharingMode::eExclusive, 0, nullptr, vk::SurfaceTransformFlagBitsKHR::eIdentity
            , vk::CompositeAlphaFlagBitsKHR::eOpaque, presentMode, true, oldSwapchain
    );
    Renderer::printOrContinue(device.createSwapchainKHR(&swapchainCreateInfo, nullptr, &newSwapchain));

    vk::CommandPoolCreateInfo commandPoolCreateInfo = vk::CommandPoolCreateInfo({}
                                                                                , selectedDeviceProps.present_queue_family_index);
    vk::CommandPool presentCommandPool;
    Renderer::printOrContinue(device.createCommandPool(&commandPoolCreateInfo, nullptr, &presentCommandPool));

    vk::CommandBufferAllocateInfo bufferAllocateInfo = vk::CommandBufferAllocateInfo(presentCommandPool
                                                                                     , vk::CommandBufferLevel::ePrimary
                                                                                     , image_count);
    std::vector<vk::CommandBuffer> presentQueueCmdBuffer;
    presentQueueCmdBuffer.resize(image_count);
    Renderer::printOrContinue(device.allocateCommandBuffers(&bufferAllocateInfo, &presentQueueCmdBuffer[0]));

//
    std::vector<vk::Image> swapchainImages = device.getSwapchainImagesKHR(newSwapchain);

    vk::ResultValue<uint32_t> image_index = device.acquireNextImageKHR(newSwapchain, UINT64_MAX
                                                                       , imageAvailableSemaphore, VK_NULL_HANDLE);

    auto commandBufferBeginInfo = vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse);
    std::array<float, 4> clear_value = {1.0f, 0.8f, 0.4f, 0.0f};
    auto clear_color = vk::ClearColorValue(clear_value);

    vk::ImageSubresourceRange image_subresource_range = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

    switch (image_index.result) {
        case vk::Result::eSuccess:
        case vk::Result::eSuboptimalKHR:
            break;
        case vk::Result::eErrorOutOfDateKHR:
            break;
        default:
            std::cout << "Problem during swap chain image acquisition" << std::endl;
            return 1;
    }

    uint32_t actual_image_index = image_index.value;

    // Record command buffers
    for (uint32_t i = 0; i < image_count; ++i) {
        vk::ImageMemoryBarrier barrier_from_present_to_clear = {
                vk::AccessFlagBits::eMemoryRead, vk::AccessFlagBits::eTransferWrite, vk::ImageLayout::eUndefined
                , vk::ImageLayout::eTransferDstOptimal, selectedDeviceProps.present_queue_family_index
                , selectedDeviceProps.present_queue_family_index, swapchainImages[i], image_subresource_range
        };
        vk::ImageMemoryBarrier barrier_from_clear_to_present = {
                vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eMemoryRead
                , vk::ImageLayout::eTransferDstOptimal
                , vk::ImageLayout::ePresentSrcKHR, selectedDeviceProps.present_queue_family_index
                , selectedDeviceProps.present_queue_family_index, swapchainImages[i], image_subresource_range
        };

        Renderer::printOrContinue(presentQueueCmdBuffer[i].begin(&commandBufferBeginInfo));
        presentQueueCmdBuffer[i].pipelineBarrier(vk::PipelineStageFlagBits::eTransfer
                                                 , vk::PipelineStageFlagBits::eTransfer
                                                 , vk::DependencyFlagBits::eViewLocal, 0
                                                 , nullptr, 0, nullptr
                                                 , 1, &barrier_from_present_to_clear);

        presentQueueCmdBuffer[i].clearColorImage(swapchainImages[i], vk::ImageLayout::eTransferDstOptimal, &clear_color
                                                 , 1, &image_subresource_range);

        presentQueueCmdBuffer[i].pipelineBarrier(vk::PipelineStageFlagBits::eTransfer
                                                 , vk::PipelineStageFlagBits::eBottomOfPipe
                                                 , vk::DependencyFlagBits::eViewLocal, 0
                                                 , nullptr, 0, nullptr
                                                 , 1, &barrier_from_clear_to_present);

        presentQueueCmdBuffer[i].end();
    }

    vk::PipelineStageFlags wait_dst_stage_mask = vk::PipelineStageFlagBits::eTransfer;
    vk::SubmitInfo submit_info = vk::SubmitInfo(1, &imageAvailableSemaphore, &wait_dst_stage_mask, 1
                                                , &presentQueueCmdBuffer[actual_image_index], 1
                                                , &renderingFinishedSemaphore);
    Renderer::printOrContinue(presentQueue.submit(1, &submit_info, VK_NULL_HANDLE));

    vk::PresentInfoKHR presentInfo = vk::PresentInfoKHR(1, &renderingFinishedSemaphore, 1, &newSwapchain
                                                        , &actual_image_index);
    Renderer::printOrContinue(presentQueue.presentKHR(presentInfo));

    // Wait!
    std::getchar();

    // Destruction
    if (device) {
        device.waitIdle();
        device.freeCommandBuffers(presentCommandPool, presentQueueCmdBuffer);
        device.destroy(presentCommandPool);
        device.destroy(newSwapchain);
        device.destroy(imageAvailableSemaphore);
        device.destroy(renderingFinishedSemaphore);
        device.destroy();
    }

    if (instance) {
        instance.destroy(debugMessenger);
        instance.destroy(surface);
        instance.destroy();
    }

    std::cout << "Hello, World! " << std::endl;

    return 0;
}

