#ifndef WINDOW_H
#define WINDOW_H

#include "pch.h"

namespace vlux {

class Window {
   public:
    Window() = delete;
    /**
     * @brief Construct a new Window object
     *
     * @param width width of the window
     * @param height height of the window
     * @param title title of the window
     */
    Window(const int width, const int height, const std::string_view title) {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window_ = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);
    }

    /**
     * @brief Destroy the Window object and teminate GLFW library
     */
    ~Window() {
        glfwDestroyWindow(window_);
        glfwTerminate();
    }

    GLFWwindow* GetGLFWwindow() const { return window_; }

    std::pair<uint32_t, uint32_t> GetWindowSize() const {
        int width, height;
        glfwGetWindowSize(window_, &width, &height);
        return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
    }

   private:
    GLFWwindow* window_;
};

}  // namespace vlux

#endif