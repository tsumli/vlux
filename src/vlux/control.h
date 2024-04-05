#ifndef CONTROL_H
#define CONTROL_H
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

    KeyboardInput GetInput();
    int GetState(int key);

   private:
    GLFWwindow* window_;
};

class Control {
   public:
    Control(GLFWwindow* window) : keyboard_(window) {}
    KeyboardInput GetKeyboardInput() { return keyboard_.GetInput(); }

   private:
    Keyboard keyboard_;
};

}  // namespace vlux

#endif