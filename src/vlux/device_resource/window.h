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
    Window(const int width, const int height, const std::string_view title)
        : width_(width), height_(height) {
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
    uint32_t GetWidth() const { return width_; }
    uint32_t GetHeight() const { return height_; }

   private:
    GLFWwindow* window_;
    const uint32_t width_;
    const uint32_t height_;
};

}  // namespace vlux

#endif