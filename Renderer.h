//
// Created by DanTheMan on 10-Jul-20.
//

#ifndef CHESSPLUSPLUS_RENDERER_H
#define CHESSPLUSPLUS_RENDERER_H

#define VK_USE_PLATFORM_WIN32_KHR

#include <vulkan/vulkan.hpp>

struct SelectedPhysicalDevice {
    uint32_t selected_queue_family_index = UINT32_MAX;
    uint32_t graphics_queue_family_index = UINT32_MAX;
    uint32_t present_queue_family_index = UINT32_MAX;
    vk::PhysicalDevice selected_physical_device = nullptr;
};


class Renderer {
public:
    SelectedPhysicalDevice selectPhysicalDevice(const std::vector<vk::PhysicalDevice> &devices, vk::SurfaceKHR surface);

    bool areExtensionsAvailable();

    bool areExtensionsAvailable(vk::PhysicalDevice device);

    uint32_t getImageCount(const vk::SurfaceCapabilitiesKHR &capabilities);

    vk::Extent2D getSwapChainExtent(const vk::SurfaceCapabilitiesKHR &capabilities);

    vk::ImageUsageFlags getImageUsageFlags(const vk::SurfaceCapabilitiesKHR &capabilities);

    vk::SurfaceFormatKHR getSurfaceFormat(std::vector<vk::SurfaceFormatKHR> formats);

    vk::PresentModeKHR getPresentMode(const std::vector<vk::PresentModeKHR> &presentModes);

    static void printOrContinue(const vk::Result result);

    HWND createWindow(HINSTANCE hinstance);

    std::vector<const char *> instance_wanted_extensions = {
            VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME, VK_EXT_DEBUG_UTILS_EXTENSION_NAME
    };
    std::vector<const char *> device_wanted_extensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,};


    static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

        switch (uMsg) {
            case WM_CLOSE:
                DestroyWindow(hWnd);
                PostQuitMessage(0);
                break;
        }

        return (DefWindowProc(hWnd, uMsg, wParam, lParam));
    }
};

#endif //CHESSPLUSPLUS_RENDERER_H
