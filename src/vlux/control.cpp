#include "control.h"

namespace vlux {
Keyboard::Keyboard(GLFWwindow* window) : window_(window) {}

int Keyboard::GetState(int key) {
    int state = glfwGetKey(window_, key);
    if (state == GLFW_PRESS) {
        return 1;
    } else {
        return 0;
    }
}

KeyboardInput Keyboard::GetInput() {
    auto input = KeyboardInput();
    input.move_up = GetState(GLFW_KEY_E) - GetState(GLFW_KEY_Q);
    input.move_forward = GetState(GLFW_KEY_W) - GetState(GLFW_KEY_S);
    input.move_right = GetState(GLFW_KEY_D) - GetState(GLFW_KEY_A);
    input.cursor_up = GetState(GLFW_KEY_UP) - GetState(GLFW_KEY_DOWN);
    input.cursor_right = GetState(GLFW_KEY_RIGHT) - GetState(GLFW_KEY_LEFT);
    input.exit = GetState(GLFW_KEY_ESCAPE);
    return input;
}
}  // namespace vlux