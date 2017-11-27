#include <glm/glm.hpp>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <functional>

#include <vector> // extension property list

const int WIDTH = 800;
const int HEIGHT = 600;

class HelloTriangleApplication
{
public:
  void run() 
  {
    initWindow();
    createInstance();
    initVulkan();
    pickPhysicalDevice();
    mainLoop();
    cleanup();
  }
private:
  GLFWwindow* window;
  VkInstance instance;
  void initWindow()
  {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
  }
  void createInstance()
  {
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

    unsigned int glfwExtensionCount = 0;
    const char** glfwExtensions;

    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    {
      // Checking for extension support
      uint32_t extensionCount = 0;
      // Used to retrieve a list of supported extensions before creating an
      // instance. 
      vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

      // List to hold the extension details
      std::vector<VkExtensionProperties> extensions(extensionCount);

      // Query the extensions details
      vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

      // Loop through the names of all the extentions available
      std::cout << "available extensions:" << std::endl;

      for (const auto& extension : extensions)
        std::cout << "\t" << extension.extensionName << std::endl;
    }

    // These determine the global validations layers to enable
    createInfo.enabledExtensionCount = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;
    createInfo.enabledLayerCount = 0;

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

  void initVulkan()
  {
    createInstance();
  }

  void pickPhysicalDevice()
  {

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