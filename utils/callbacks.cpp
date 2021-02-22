#include "callbacks.h"

void cursorPosCallback(GLFWwindow *window, double x, double y)
{
    if (ImGui::GetIO().WantCaptureKeyboard) {
        return;
    }
    // std::cout << "debug0" << std::endl;
    App *app = static_cast<App*>(glfwGetWindowUserPointer(window));
    // std::cout << "mouse prev pos x " << app ->preMousePos.x << " y " << app ->preMousePos.y << std::endl;
    // std::cout << "debug1" << std::endl;
    
    const rkcommon::math::vec2f mouse(x, y);
    // std::cout << "mouse pos x " << x << " y " << y << std::endl;
    // std::cout << "mouse prev pos x " << app ->preMousePos.x << " y " << app ->preMousePos.y << std::endl;
    if(app ->preMousePos != rkcommon::math::vec2f(-1)){
        // std::cout << "debug3" << std::endl;
        const bool leftDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        const bool rightDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
        const bool middleDown = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
        const rkcommon::math::vec2f prev = app->preMousePos;
        app->isCameraChanged = leftDown || rightDown || middleDown;
        if (leftDown) {
            const rkcommon::math::vec2f mouseFrom(rkcommon::math::clamp(prev.x * 2.f / app ->fbSize.x - 1.f,  -1.f, 1.f),
                                  rkcommon::math::clamp(prev.y * 2.f / app ->fbSize.y - 1.f,  -1.f, 1.f));
            const rkcommon::math::vec2f mouseTo(rkcommon::math::clamp(mouse.x * 2.f / app ->fbSize.x - 1.f,  -1.f, 1.f),
                                rkcommon::math::clamp(mouse.y * 2.f / app ->fbSize.y - 1.f,  -1.f, 1.f));
            app->camera.rotate(mouseFrom, mouseTo);
        } else if (rightDown) {
            // std::cout << "Right down zoom " << mouse.y - prev.y << std::endl;

            app->camera.zoom(mouse.y - prev.y);
        } else if (middleDown) {
            app->camera.pan(rkcommon::math::vec2f(prev.x - mouse.x, prev.y - mouse.y));
        }
    }
    // std::cout << "debug2" << std::endl;
    app->preMousePos = mouse;
}
