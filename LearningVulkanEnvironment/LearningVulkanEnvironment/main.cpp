#include <glm/glm.hpp>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <functional>

const int WIDTH = 800;
const int HEIGHT = 600;

class HelloTriangleApplication
{
public:
  void run() 
  {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
  }
private:
  GLFWwindow* window;
  void initWindow()
  {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
  }
  void initVulkan()
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