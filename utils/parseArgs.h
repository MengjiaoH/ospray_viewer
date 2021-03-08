#pragma once


#include <iostream>
#include <vector>
// #include "rkcommon/math/vec.h"
#include "rkcommon/math/vec.h"

// using namespace rkcommon::math;
using namespace rkcommon::math;
struct Args
{
    std::string extension;
    std::string filename;
    std::vector<std::string> time_series_filename;
    vec3i dims;
    std::string dtype;
    std::string vtype;
    std::string view_file;
    std::string opacity_file;
    std::string color_file;
    std::string write_camera_to;
    int n_samples;
};

std::string getFileExt(const std::string& s);

void parseArgs(int argc, const char **argv, Args &args);
