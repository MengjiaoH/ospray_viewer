#pragma once 

#include <iostream>
#include <mutex>
#include <vector> 
#include <fstream>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "json.hpp"

using json = nlohmann::json;

struct Bar
{
    float birth;
    float death;
    Bar(float b, float d){
        birth = b;
        death = d;
    }
};

std::vector<Bar> getBarcode();

class Widget{
    // float preTimeStep = 1;
    // float currentTimeStep = 1;
    // float beginTimeStep = 1;
    // float endTimeStep = 1;
    // bool timeStepChanged = false;
    float range_start = 0;
    float range_end = 0;
    float iso = 0;
    float pre_iso = 0;
    bool isoValueChanged = false;
    std::vector<Bar> bars;

    // bool doUpdate{false}; // no initial update
    // std::shared_ptr<tfn::tfn_widget::TransferFunctionWidget> widget;
    std::mutex lock;

    public:
        bool show_isosurfaces = true;
        Widget(float begin, float end, float default_iso, std::vector<Bar> bars);
        void draw();
        bool changed();
        float getIsoValue();   
};

