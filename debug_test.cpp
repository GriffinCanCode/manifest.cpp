#include <iostream>
#include <vulkan/vulkan.hpp>

// Quick test to see if Vulkan is working at all on your system
int main() {
    std::cout << "=== VULKAN DIAGNOSTIC TEST ===" << std::endl;
    
    try {
        // Create minimal Vulkan instance
        vk::ApplicationInfo app_info{};
        app_info.pApplicationName = "Vulkan Test";
        app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.pEngineName = "Test Engine";
        app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.apiVersion = VK_API_VERSION_1_2;

        // Add required extensions for MoltenVK on macOS
        std::vector<const char*> extensions = {
            "VK_KHR_surface",
            "VK_KHR_portability_enumeration",  // Required for MoltenVK
            "VK_EXT_metal_surface"             // Metal surface for macOS
        };

        vk::InstanceCreateInfo create_info{};
        create_info.flags = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;  // Required for MoltenVK
        create_info.pApplicationInfo = &app_info;
        create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        create_info.ppEnabledExtensionNames = extensions.data();

        auto instance = vk::createInstance(create_info);
        std::cout << "✓ Vulkan instance created successfully" << std::endl;

        // List physical devices
        auto devices = instance.enumeratePhysicalDevices();
        std::cout << "✓ Found " << devices.size() << " physical devices:" << std::endl;
        
        for (size_t i = 0; i < devices.size(); ++i) {
            auto props = devices[i].getProperties();
            std::cout << "  Device " << i << ": " << props.deviceName << " (Type: ";
            switch (props.deviceType) {
                case vk::PhysicalDeviceType::eIntegratedGpu: std::cout << "Integrated GPU"; break;
                case vk::PhysicalDeviceType::eDiscreteGpu: std::cout << "Discrete GPU"; break;
                case vk::PhysicalDeviceType::eCpu: std::cout << "CPU"; break;
                default: std::cout << "Other"; break;
            }
            std::cout << ")" << std::endl;
        }

        if (!devices.empty()) {
            auto selected = devices[0];
            auto props = selected.getProperties();
            std::cout << "✓ Selected device: " << props.deviceName << std::endl;
            std::cout << "  Driver version: " << props.driverVersion << std::endl;
            std::cout << "  Vulkan version: " << VK_VERSION_MAJOR(props.apiVersion) << "." 
                      << VK_VERSION_MINOR(props.apiVersion) << "." << VK_VERSION_PATCH(props.apiVersion) << std::endl;
        }

        instance.destroy();
        std::cout << "✓ Vulkan diagnostic test PASSED" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cout << "✗ Vulkan diagnostic test FAILED: " << e.what() << std::endl;
        return 1;
    }
}
