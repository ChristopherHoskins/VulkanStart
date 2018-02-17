#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_EXPOSE_NATIVE_WIN32

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <algorithm>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <iostream>
#include <stdexcept>
#include <functional>
#include <vector> // extension property list
#include <set>
#include <fstream>

const int WIDTH = 800;
const int HEIGHT = 600;

/*
  Implicity enables a whole range of useful diagnostic layers.
  Example: 
    if (pCreateInfo == nullptr || instance == nullptr) 
    {
        log("Null pointer passed to required parameter!");
        return VK_ERROR_INITIALIZATION_FAILED;
    }
*/
const std::vector<const char*> validationLayers =
{
  "VK_LAYER_LUNARG_standard_validation"
};

const std::vector<const char*> deviceExtensions = {
  VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

std::vector<VkImageView> swapChainImageViews;

// Preproccesor for whether to application is in debug or release. We dont want
// error checking during release
#ifdef NDEBUG
  const bool enableValidationLayers = false;
#else
  const bool enableValidationLayers = true;
#endif

struct SwapChainSupportDetails
{
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

bool checkValidationLayerSupport()
{
  uint32_t layerCount;
  /*
    General pattern for obtaining available layers, extensions devices, etc.
    Count all avaiable things
    Initialize list to contain counted layers
    Fill list with available things to search
  */
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

  // Now check if all implicit layers in VK_LAYER_LUNARG_standard_validation
  // are in the list of available layers from our search
  for (const char* layerName : validationLayers)
  {
    bool layerFound = false;

    for (const auto& layerProperties : availableLayers)
    {
      if (strcmp(layerName, layerProperties.layerName) == 0)
      {
        layerFound = true;
        break;
      }
    }

    if (!layerFound)
      return false;
  }

  return true;
}

class HelloTriangleApplication
{
public:
  struct QueueFamilyIndices
  {
    int graphicsFamily = -1;
    int presentFamily = -1;
    // Just because the physical device supports drawing commands doesnt mean it
    // necessarily supports presenting results onto a surface
    bool isComplete()
    {
      return graphicsFamily >= 0 && presentFamily >= 0;
    }
  };
  void run()
  {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
  }
private:
  GLFWwindow* window;
  VkInstance instance;
  VkPhysicalDevice physicalDevice;
  VkDevice device;
  VkQueue presentQueue;
  VkQueue graphicsQueue;
  VkSwapchainKHR swapChain;
  std::vector<VkImage> swapChainImages;
  VkFormat swapChainImageFormat;
  VkExtent2D swapChainExtent;
  VkDebugReportCallbackEXT callback;
  VkSurfaceKHR surface;
  void initWindow()
  {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
  }

  std::vector<const char*> getRequiredExtensions()
  {
    std::vector<const char*> extensions;

    unsigned int glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    for (unsigned i = 0; i < glfwExtensionCount; ++i)
      extensions.push_back(glfwExtensions[i]);

    // If we are in debug we want to pop this extension in to allow us to add
    // debugging validation layers into the current vulkan instance
    if (enableValidationLayers)
      extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

    return extensions;
  }

  void createInstance()
  {
    // The validation layers that we want arent available on this computer
    if (enableValidationLayers && !checkValidationLayerSupport())
      throw std::runtime_error("validation layers requested, but not available!");

    VkApplicationInfo appInfo = {};
    // Required to be explicitly specified. It's done so for backwards
    // compatibility.
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;
    // There is also a pNext member which we are leaving to default, but pNext
    // is used to point to extension information in the future.

    // This struct is not optional, this tells the vulkan driver which global
    // extensions and validation layers we want to use
    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    auto extensions = getRequiredExtensions();

    // These determine the global validations layers to enable
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    // Enable the list of validation layer we want
    if (enableValidationLayers)
    {
      createInfo.enabledLayerCount = validationLayers.size();
      createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else
    {
      createInfo.enabledLayerCount = 0;
    }

    /*
    General pattern of object creation is
      Pointer to struct with creation info
      Pointer to custom allocator callbacks, always nullptr in this tutorial
      Pointer to the variable that stores the handle to the new object
    */
    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
      throw std::runtime_error("failed to create instance!");
  }


  static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugReportFlagsEXT flags,
    VkDebugReportObjectTypeEXT objType,
    uint64_t obj,
    size_t location,
    int32_t code,
    const char* layerPrefix,
    const char* msg,
    void* userData)
  {
    std::cerr << "validation layer: " << msg << std::endl;

    return VK_FALSE;
  }

  void createSurface()
  {
    //VkWin32SurfaceCreateInfoKHR createInfo = {};
    //createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    //// Window handle
    //createInfo.hwnd = glfwGetWin32Window(window);
    //// HINSTANCE
    //createInfo.hinstance = GetModuleHandle(nullptr);

    //auto CreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR");

    //if (!CreateWin32SurfaceKHR || CreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface) != VK_SUCCESS)
    //  throw std::runtime_error("failed to create window surface!");

    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
      throw std::runtime_error("failed to create window surface!");
  }

  void setupDebugCallback()
  {
    if (!enableValidationLayers) return;

    VkDebugReportCallbackCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    // What types of messages I would like to receive
    createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT
      | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    createInfo.pfnCallback = debugCallback;

    if (CreateDebugReportCallbackEXT(instance, &createInfo, nullptr, &callback) != VK_SUCCESS)
      throw std::runtime_error("failed to set up debug callback!");
  }

  VkResult CreateDebugReportCallbackEXT(VkInstance instance,
    const VkDebugReportCallbackCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugReportCallbackEXT* pCallback)
  {
    // Since this is an extension function, it is not automatically loaded. We
    // have to look up its address using vkGetInstanceProcAddr.
    auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance,
      "vkCreateDebugReportCallbackEXT");
    if (func != nullptr)
      return func(instance, pCreateInfo, pAllocator, pCallback);
    else
      return VK_ERROR_EXTENSION_NOT_PRESENT;
  }

  void initVulkan()
  {
    createInstance();
    setupDebugCallback();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createGraphicsPipeline();
  }

  void createImageViews()
  {
    swapChainImageViews.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); ++i)
    {
      VkImageViewCreateInfo createInfo = {};
      createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      createInfo.image = swapChainImages[i];
      createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
      createInfo.format = swapChainImageFormat;

      createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
      createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY; 
      createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY; 
      createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

      createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      createInfo.subresourceRange.baseMipLevel = 0;
      createInfo.subresourceRange.levelCount = 1;
      createInfo.subresourceRange.baseArrayLayer = 0;
      createInfo.subresourceRange.layerCount = 1;

      if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
        throw std::runtime_error("failed to create image views!");
    }
  }

  void createSwapChain()
  {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
      imageCount = swapChainSupport.capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1; // Should always be unless doing 3d steroscopics 
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    uint32_t queueFamilyIndicies[] = { (uint32_t)indices.graphicsFamily, (uint32_t)indices.presentFamily };

    if (indices.graphicsFamily != indices.presentFamily)
    {
      createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT; // Can be used across multiple queue families
      createInfo.queueFamilyIndexCount = 2;
      createInfo.pQueueFamilyIndices = queueFamilyIndicies;
    }
    else
    {
      createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
      createInfo.queueFamilyIndexCount = 0;
      createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VkSwapchainKHR swapChain;
    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
      throw std::runtime_error("failed to create swap chain!");
    
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
  }
  
  /*
    Everything that we do in Vulkan requires commands such as obtaining texture
    information and drawing, but these commands are required to be submitted to
    a queue. There are different types of queue families and each family allows
    only a subset of commands. Since we need a queue for our graphics device we
    search for a suitable queue for graphics commands
  */
  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
  {
    QueueFamilyIndices indices;

    // Standard find count then make a vector with the family count and fill up 
    // vector with queuried queue family information
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    // VkQueueFamilyProperties contains details about types of operations that 
    // are supported and the number of queues that can be created based on family
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device,
      &queueFamilyCount,
      queueFamilies.data());

    // We need a queue family that supports VK_QUEUE_GRAPHICS_BIT
    int i = 0;
    for (const auto& queueFamily : queueFamilies)
    {
      if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        indices.graphicsFamily = i;

      // Check if queue family supports presenting to surface
      // (Optimization) Find queue that supports graphics and presenting
      VkBool32 presentSupport = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

      if (queueFamily.queueCount > 0 && presentSupport)
        indices.presentFamily = i;

      if (indices.isComplete())
        break;

      i++;
    }

    return indices;
  }

  bool checkDeviceExtensionSupport(VkPhysicalDevice device)
  {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto& extension : availableExtensions)
      requiredExtensions.erase(extension.extensionName);

    return requiredExtensions.empty();
  }

  SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device)
  {
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (formatCount != 0)
    {
      details.formats.resize(formatCount);
      vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModecount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModecount, nullptr);

    if (presentModecount != 0)
    {
      details.presentModes.resize(presentModecount);
      vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModecount, details.presentModes.data());
    }

    return details;
  }

  VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
  {
    // Best case scenario we get to choose our own format
    if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
      return{ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };

    for (const auto& availableFormat : availableFormats)
    {
      if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        return availableFormat;
    }

    return availableFormats[0];
  }

  VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes)
  {
    VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

    for (const auto& availablePresentMode : availablePresentModes)
    {
      if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        return availablePresentMode;
      else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
        bestMode = availablePresentMode;
    }

    return bestMode;
  }

  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
  {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
      return capabilities.currentExtent;
    }
    else
    {
      VkExtent2D actualExtent = { WIDTH, HEIGHT };

      actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
      actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
      return actualExtent;
    }
  }

  bool isDeviceSuitable(VkPhysicalDevice device)
  {
    QueueFamilyIndices indices = findQueueFamilies(device);
    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported)
    {
      SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
      swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    return indices.isComplete() && extensionsSupported && swapChainAdequate;

    /*
      If we did want to find devices with particularly desired properties
      you could querie the physical information such as name, types and
      supported vulkan versions
    */

    /*
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    // Checks for optional feature support such as texture compression, 
    // 64 bit floats and multi viewport rendering
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    // Consider that our device is only usable with graphics cards that
    // supports geometry shaders
    return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
        && deviceFeatures.geometryShader;
    
     There are several ways of finding the best physical device or even
     scoring the best possible device
    */
  }

  // Find the physical device or the graphics card in the system that supports
  // the features we need. (We can select more than one)
  void pickPhysicalDevice()
  {
    // Gets implicitly destroyed when VkInstance is destroyed
    physicalDevice = VK_NULL_HANDLE;

    uint32_t deviceCount = 0;
    // Count the number of available physical devices
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0)
      throw std::runtime_error("failed to find GPUs with Vulkan support!");
    std::vector<VkPhysicalDevice> devices(deviceCount);
    // Stores all the available devices within our list
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (const auto& device : devices)
    {
      if (isDeviceSuitable(device))
      {
        physicalDevice = device;
        break;
      }
    }

    if (physicalDevice == VK_NULL_HANDLE)
      throw std::runtime_error("failed to find a suitable GPU!");
  }

  void createGraphicsPipeline()
  {
    auto vertShaderCode = readFile("shaders/vert.spv");
    auto fragShaderCode = readFile("shaders/frag.spv");

    VkShaderModule vertShaderModule;
    VkShaderModule fragShaderModule;

    vertShaderModule = createShaderModule(vertShaderCode);
    fragShaderModule = createShaderModule(fragShaderCode);

    vkDestroyShaderModule(device, vertShaderModule, nullptr);
    vkDestroyShaderModule(device, fragShaderModule, nullptr);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };
  }

  VkShaderModule createShaderModule(const std::vector<char>& code)
  {
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    // pointer to buffer with bytecode takes in a uint32_t but our buffer is a char*
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
      throw std::runtime_error("failed to create shader module!");

    return shaderModule;
  }

  static std::vector<char> readFile(const std::string& filename)
  {
    // ate: starts reading at end of file for buffer reasons
    // binary: read the file as a binary file
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
      throw std::runtime_error("failed to open file!");

    size_t fileSize = (size_t)file.tellg(); // use read position to determine size of file for buffer
    std::vector<char> buffer(fileSize);
    file.seekg(0); // start at beginning
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
  }

  void createLogicalDevice()
  {
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    // This structure describes the number of queues we want for a single queue 
    // family
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<int> uniqueQueueFamilies = { indices.graphicsFamily, indices.presentFamily };

    // Vulkan lets you assign priorities using a floating point number which
    // will influence the schedule of buffer execution (This is required even
    // for a small queue)
    float queuePriority = 1.0f;
    for (int queueFamily : uniqueQueueFamilies)
    {
      VkDeviceQueueCreateInfo queueCreateInfo = {};
      queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queueCreateInfo.queueFamilyIndex = queueFamily;
      queueCreateInfo.queueCount = 1;
      queueCreateInfo.pQueuePriorities = &queuePriority;
      queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures = {};
    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (enableValidationLayers)
    {
      createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
      createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else
    {
      createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
      throw std::runtime_error("failed to create logical device!");

    // Retrieve queue handles for each queue family, since we are only creating
    // a single queue from this family we simply use index 0
    vkGetDeviceQueue(device, indices.graphicsFamily, 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily, 0, &presentQueue);
  }

  void DestroyDebugReportCallbackEXT(VkInstance instance,
    VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator)
  {
    auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance,
      "vkDestroyDebugReportCallbackEXT");
    if (func != nullptr)
      func(instance, callback, pAllocator);
  }

  // Run while checking for events like pressing xuntil the window itself has
  // been closed
  void mainLoop()
  {
    while (!glfwWindowShouldClose(window))
    {
      glfwPollEvents();
    }
  }

  void cleanup()
  {
    for (auto imageView : swapChainImageViews)
      vkDestroyImageView(device, imageView, nullptr);
    vkDestroySwapchainKHR(device, swapChain, nullptr);
    vkDestroyDevice(device, nullptr);
    DestroyDebugReportCallbackEXT(instance, callback, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    // Instance should be destroyed right before program exits.
    vkDestroyInstance(instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
  }
};

int main()
{
  HelloTriangleApplication app;

  try
  {
    app.run();
  }
  catch (const std::runtime_error& e)
  {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  int n;
  std::cin >> n;

  return EXIT_SUCCESS;
}