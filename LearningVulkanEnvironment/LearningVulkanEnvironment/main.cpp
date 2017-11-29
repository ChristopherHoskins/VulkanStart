#include <glm/glm.hpp>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <functional>

#include <vector> // extension property list

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

// Preproccesor for whether to application is in debug or release. We dont want
// error checking during release
#ifdef NDEBUG
  const bool enableValidationLayers = false;
#else
  const bool enableValidationLayers = true;
#endif

class HelloTriangleApplication
{
public:
  struct QueueFamilyIndices
  {
    int graphicsFamily = -1;
    bool isComplete()
    {
      return graphicsFamily >= 0;
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
  VkQueue graphicsQueue;
  VkDebugReportCallbackEXT callback;
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

    /*
      //Checking for extension support
      {
        uint32_t extensionCount = 0;
        // Used to retrieve a list of supported extensions before creating an
        // instance. 
        vkEnumerateInstanceExtensionProperties(nullptr, 
                                              &extensionCount, 
                                               nullptr);
    
        // List to hold the extension details
        std::vector<VkExtensionProperties> extensions(extensionCount);
    
        // Query the extensions details
        vkEnumerateInstanceExtensionProperties(nullptr, 
                                               &extensionCount, 
                                                extensions.data());
    
        // Loop through the names of all the extentions available
        std::cout << "available extensions:" << std::endl;
    
        for (const auto& extension : extensions)
          std::cout << "\t" << extension.extensionName << std::endl;
      }
    */

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

  void createLogicalDevice()
  {
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    
    // This structure describes the number of queues we want for a single queue 
    // family
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = indices.graphicsFamily;
    queueCreateInfo.queueCount = 1;

    // Vulkan lets you assign priorities using a floating point number which
    // will influence the schedule of buffer execution (This is required even
    // for a small queue)
    float queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures deviceFeatures = {};

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = 0;

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
      throw std::runtime_error("failed to create logical device!");

    // Retrieve queue handles for each queue family, since we are only creating
    // a single queue from this family we simply use index 0
    vkGetDeviceQueue(device, indices.graphicsFamily, 0, &graphicsQueue);
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

  void setupDebugCallback()
  {
    if (!enableValidationLayers) return;

    VkDebugReportCallbackCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    // What types of messages I would like to receive
    createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT 
                     | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    createInfo.pfnCallback = debugCallback;
  }

  void initVulkan()
  {
    createInstance();
    pickPhysicalDevice();
    createLogicalDevice();
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
      if (queueFamily.queueCount > 0
       && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
      {
        indices.graphicsFamily = i;
      }
      if (indices.isComplete())
      {
        break;
      }
      i++;
    }

    return indices;
  }

  bool isDeviceSuitable(VkPhysicalDevice device)
  {
    QueueFamilyIndices indices = findQueueFamilies(device);
    return indices.isComplete();

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
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

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
    vkDestroyDevice(device, nullptr);
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

  return EXIT_SUCCESS;
}