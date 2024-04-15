#include "control.h"

#include "common/utils.h"
namespace vlux {
Keyboard::Keyboard(GLFWwindow* window) : window_(window) {}

int Keyboard::GetState(int key) const {
    int state = glfwGetKey(window_, key);
    if (state == GLFW_PRESS) {
        return 1;
    } else {
        return 0;
    }
}

KeyboardInput Keyboard::GetInput() const {
    auto input = KeyboardInput();
    input.move_up = GetState(GLFW_KEY_E) - GetState(GLFW_KEY_Q);
    input.move_forward = GetState(GLFW_KEY_W) - GetState(GLFW_KEY_S);
    input.move_right = GetState(GLFW_KEY_D) - GetState(GLFW_KEY_A);
    input.cursor_up = GetState(GLFW_KEY_UP) - GetState(GLFW_KEY_DOWN);
    input.cursor_right = GetState(GLFW_KEY_RIGHT) - GetState(GLFW_KEY_LEFT);
    input.exit = GetState(GLFW_KEY_ESCAPE);
    return input;
}

Mouse::Position Mouse::GetMouseDisplacement() const {
    double xpos, ypos;
    glfwGetCursorPos(window_, &xpos, &ypos);
    const auto [width, height] = GetWindowSize(window_);
    const auto center_width = static_cast<double>(width) / 2.0;
    const auto center_height = static_cast<double>(height) / 2.0;
    return std::make_pair(xpos - center_width, ypos - center_height);
}

void Mouse::RecenterCursor() const {
    const auto [width, height] = GetWindowSize(window_);
    const auto center_width = static_cast<double>(width) / 2.0;
    const auto center_height = static_cast<double>(height) / 2.0;
    glfwSetCursorPos(window_, center_width, center_height);
}

void Mouse::MakeCursorHidden() const {
    glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}
void Mouse::MakeCursorVisible() const {
    glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void Mouse::UpdateButton() {
    last_button_input_ = current_button_input_;
    current_button_input_ = GetButtonInput();
}

MouseButtonInput Mouse::GetButtonInput() const {
    const auto check_button_release = [&](const int button) {
        return glfwGetMouseButton(window_, button) == GLFW_PRESS;
    };
    return {
        .left = check_button_release(GLFW_MOUSE_BUTTON_LEFT),
        .middle = check_button_release(GLFW_MOUSE_BUTTON_MIDDLE),
        .right = check_button_release(GLFW_MOUSE_BUTTON_RIGHT),
    };
}

MouseButtonInput Mouse::GetButtonChanges() const {
    return {
        .left = current_button_input_.left && !last_button_input_.left,
        .middle = current_button_input_.middle && !last_button_input_.middle,
        .right = current_button_input_.right && !last_button_input_.right,
    };
}

}  // namespace vlux