//
// Created by DanTheMan on 10-Jul-20.
//

#include <cstdint>
#include <iostream>
#include "Renderer.h"

SelectedPhysicalDevice
Renderer::selectPhysicalDevice(const std::vector<vk::PhysicalDevice> &devices, vk::SurfaceKHR surface) {
    SelectedPhysicalDevice selected = SelectedPhysicalDevice();
    for (vk::PhysicalDevice candidate : devices) {
        vk::PhysicalDeviceProperties properties = candidate.getProperties();
        vk::PhysicalDeviceFeatures features = candidate.getFeatures();

        uint32_t major_version = VK_VERSION_MAJOR(properties.apiVersion);

        if ((major_version < 1) && (
                properties.limits.maxImageDimension2D < 4096
        )) {
            std::cout << "Physical device " << properties.deviceName << " doesn't support required parameters!"
                      << std::endl;
            continue;
        }

        std::vector<vk::QueueFamilyProperties> familyProperties = candidate.getQueueFamilyProperties();

        for (uint32_t i = 0; i < static_cast<uint32_t >(familyProperties.size()); ++i) {

            const auto &queues = familyProperties[i];

            if ((queues.queueCount > 0) && (queues.queueFlags & vk::QueueFlagBits::eGraphics) &&
                areExtensionsAvailable(candidate)) {
                selected.graphics_queue_family_index = i;
            }
            if ((queues.queueCount > 0) && candidate.getSurfaceSupportKHR(i, surface)) {
                selected.present_queue_family_index = i;
            }
        }

        if (selected.graphics_queue_family_index != UINT32_MAX && selected.present_queue_family_index != UINT32_MAX) {
            selected.selected_physical_device = candidate;
        } else {
            continue;
        }
    }
    return selected;
}

bool Renderer::areExtensionsAvailable() {
    const std::vector<vk::ExtensionProperties> &extensions = vk::enumerateInstanceExtensionProperties();
    for (auto &wanted_extension : instance_wanted_extensions) {
        bool is_extension_available = false;
        for (const auto &extension : extensions) {
            if (strcmp(extension.extensionName, wanted_extension) == 0) {
                is_extension_available = true;
                break;
            }
        }
        if (!is_extension_available) return false;
    }
    return true;
}

bool Renderer::areExtensionsAvailable(vk::PhysicalDevice device) {
    const std::vector<vk::ExtensionProperties> &extensions = device.enumerateDeviceExtensionProperties();
    for (auto &wanted_extension : device_wanted_extensions) {
        bool is_extension_available = false;
        for (const auto &extension : extensions) {
            if (strcmp(extension.extensionName, wanted_extension) == 0) {
                is_extension_available = true;
                break;
            }
        }
        if (!is_extension_available) return false;
    }
    return true;
}

uint32_t Renderer::getImageCount(const vk::SurfaceCapabilitiesKHR &capabilities) {
    uint32_t image_count = capabilities.minImageCount + 1;
    if ((capabilities.maxImageCount > 0) && (image_count > capabilities.maxImageCount)) {
        image_count = capabilities.maxImageCount;
    }
    return image_count;
}

vk::SurfaceFormatKHR Renderer::getSurfaceFormat(std::vector<vk::SurfaceFormatKHR> formats) {
    if ((formats.size() == 1) && (formats[0].format == vk::Format::eUndefined)) {
        return {vk::Format::eR8G8B8A8Unorm, vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear};
    }

    for (vk::SurfaceFormatKHR &format : formats) {
        if (format.format == vk::Format::eR8G8B8A8Unorm) {
            return format;
        }
    }

    return formats[0];
}

vk::Extent2D Renderer::getSwapChainExtent(const vk::SurfaceCapabilitiesKHR &capabilities) {
    if (capabilities.currentExtent.width == -1) {
        vk::Extent2D swapChainExtent = {640, 480};
        if (capabilities.minImageExtent.width > swapChainExtent.width) {
            swapChainExtent.width = capabilities.minImageExtent.width;
        }
        if (capabilities.minImageExtent.height > swapChainExtent.height) {
            swapChainExtent.height = capabilities.minImageExtent.height;
        }
        if (capabilities.maxImageExtent.width < swapChainExtent.width) {
            swapChainExtent.width = capabilities.maxImageExtent.width;
        }
        if (capabilities.maxImageExtent.height < swapChainExtent.height) {
            swapChainExtent.height = capabilities.maxImageExtent.height;
        }
        return swapChainExtent;
    }

    return capabilities.currentExtent;
}

vk::ImageUsageFlags Renderer::getImageUsageFlags(const vk::SurfaceCapabilitiesKHR &capabilities) {
    if (capabilities.supportedUsageFlags & vk::ImageUsageFlagBits::eTransferDst) {
        return vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;
    }

    return vk::ImageUsageFlags();
}

vk::PresentModeKHR Renderer::getPresentMode(const std::vector<vk::PresentModeKHR> &presentModes) {
    for (vk::PresentModeKHR presentMode : presentModes) {
        if (presentMode == vk::PresentModeKHR::eMailbox) {
            return presentMode;
        }
    }
    for (vk::PresentModeKHR presentMode : presentModes) {
        if (presentMode == vk::PresentModeKHR::eFifo) {
            return presentMode;
        }
    }
    return presentModes[0];
}

HWND Renderer::createWindow(HINSTANCE hinstance) {
    vk::Extent2D windowSize = {1280, 720};
    bool fullscreen = false;
    // Console
    AllocConsole();
    AttachConsole(GetCurrentProcessId());
    FILE *stream;
    freopen_s(&stream, "CONOUT$", "w+", stdout);
    SetConsoleTitle(TEXT("VULKAN_TUTORIAL"));

    WNDCLASSEX wndClass;

    wndClass.cbSize = sizeof(WNDCLASSEX);
    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc = WndProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = hinstance;
    wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
    wndClass.lpszMenuName = NULL;
    wndClass.lpszClassName = "VULKAN_TUTORIAL";
    wndClass.hIconSm = LoadIcon(NULL, IDI_WINLOGO);

    if (!RegisterClassEx(&wndClass)) {
        std::cout << "Could not register window class!\n";
        fflush(stdout);
        exit(1);
    }

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);


    DWORD dwExStyle;
    DWORD dwStyle;

    dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
    dwStyle = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

    RECT windowRect;
    windowRect.left = 0L;
    windowRect.top = 0L;
    windowRect.right = (long) windowSize.width;
    windowRect.bottom = (long) windowSize.height;

    AdjustWindowRectEx(&windowRect, dwStyle, FALSE, dwExStyle);

    HWND window = CreateWindowEx(0, "VULKAN_TUTORIAL", "VULKAN TUTORIAL 01", dwStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN
                                 , 0, 0, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, NULL
                                 , NULL, hinstance, NULL);


    uint32_t x = (GetSystemMetrics(SM_CXSCREEN) - windowRect.right) / 2;
    uint32_t y = (GetSystemMetrics(SM_CYSCREEN) - windowRect.bottom) / 2;
    SetWindowPos(window, 0, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);


    if (!window) {
        printf("Could not create window!\n");
        fflush(stdout);
        return 0;
        exit(1);
    }

    ShowWindow(window, SW_SHOW);
    SetForegroundWindow(window);
    SetFocus(window);

    return window;
}

void Renderer::printOrContinue(const vk::Result result) {
    if (result != vk::Result::eSuccess) {
        std::cout << "An error has occured: " << result << std::endl;
    }
}
