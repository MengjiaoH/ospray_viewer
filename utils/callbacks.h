#pragma once 
#include <iostream>
#include <algorithm>
#include <GLFW/glfw3.h>

// #include "rkcommon/math/vec.h"
#include "rkcommon/math/vec.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "ArcballCamera.h"

struct App
{
    rkcommon::math::vec2f preMousePos = rkcommon::math::vec2f(-1);
    bool isCameraChanged = false;
    rkcommon::math::vec2i fbSize;
    ArcballCamera camera; 
    bool isIsoValueChanged = false;
    bool showIsosurfaces = false;
    bool showVolume = false;
    bool isTransferFcnChanged = false;

    App(rkcommon::math::vec2i imgSize, ArcballCamera &camera) 
        : fbSize(imgSize), isCameraChanged(false), camera(camera)
    {}
};

void cursorPosCallback(GLFWwindow *window, double x, double y);