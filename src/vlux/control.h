#ifndef CONTROL_H
#define CONTROL_H
#include <GLFW/glfw3.h>

#include "pch.h"

namespace vlux {
struct KeyboardInput {
    int move_forward;
    int move_up;
    int move_right;
    int cursor_up;
    int cursor_right;
    int exit;
};

class Keyboard {
   public:
    Keyboard(GLFWwindow* window);

    KeyboardInput GetInput() const;
    int GetState(int key) const;

   private:
    GLFWwindow* window_;
};

struct MouseButtonInput {
    bool left;
    bool middle;
    bool right;
};

class Mouse {
   public:
    using Position = std::pair<double, double>;
    Mouse() = delete;
    explicit Mouse(GLFWwindow* window) : window_(window) {}
    Position GetMouseDisplacement() const;
    MouseButtonInput GetButtonChanges() const;
    void RecenterCursor() const;
    void MakeCursorHidden() const;
    void MakeCursorVisible() const;
    void UpdateButton();

   private:
    GLFWwindow* window_;
    MouseButtonInput last_button_input_;
    MouseButtonInput current_button_input_;

    MouseButtonInput GetButtonInput() const;
};

class Control {
   public:
    Control() = delete;
    explicit Control(GLFWwindow* window) : keyboard_(window), mouse_(window) {}
    Keyboard& MutableKeyboard() { return keyboard_; }
    Mouse& MutableMouse() { return mouse_; }

   private:
    Keyboard keyboard_;
    Mouse mouse_;
};

}  // namespace vlux

#endif