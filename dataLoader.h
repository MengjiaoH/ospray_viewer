#pragma once

#include <iostream>
#include <fstream>
#include <iterator>
#include <vector>
#include <algorithm>

#include "rkcommon/math/vec.h"

using namespace rkcommon::math;

struct Volume {
    vec3i dims;
    vec2f range;
    vec3f spacing{1.f};
    vec3f origin{0.f};
    int timestep;
    std::shared_ptr<std::vector<float>> voxel_data = nullptr;

    size_t n_voxels() const
    {
        return size_t(dims.x) * size_t(dims.y) * size_t(dims.z);
    }
};

Volume load_raw_volume(const std::string &fname,
                       const vec3i &dims,
                       const std::string &voxel_type)
{
    Volume volume;
    volume.dims = dims;

    size_t voxel_size = 0;
    if (voxel_type == "uint8") {
        voxel_size = 1;
    } else if (voxel_type == "uint16") {
        voxel_size = 2;
    } else if (voxel_type == "float32") {
        voxel_size = 4;
    } else if (voxel_type == "float64") {
        voxel_size = 8;
    } else {
        throw std::runtime_error("Unrecognized voxel type " + voxel_type);
    }

    std::ifstream fin(fname.c_str(), std::ios::binary);
    auto voxel_data = std::make_shared<std::vector<uint8_t>>(volume.n_voxels() * voxel_size, 0);

    if (!fin.read(reinterpret_cast<char *>(voxel_data->data()), voxel_data->size())) {
        throw std::runtime_error("Failed to read volume " + fname);
    }

    volume.voxel_data = std::make_shared<std::vector<float>>(volume.n_voxels(), 0.f);

    // Temporarily convert non-float data to float
    // TODO will native support for non-float voxel types
    if (voxel_type == "uint8") {
        std::transform(voxel_data->begin(),
                       voxel_data->end(),
                       volume.voxel_data->begin(),
                       [](const uint8_t &x) { return float(x); });
    } else if (voxel_type == "uint16") {
        std::transform(reinterpret_cast<uint16_t *>(voxel_data->data()),
                       reinterpret_cast<uint16_t *>(voxel_data->data()) + volume.n_voxels(),
                       volume.voxel_data->begin(),
                       [](const uint16_t &x) { return float(x); });
    } else if (voxel_type == "float32") {
        std::transform(reinterpret_cast<float *>(voxel_data->data()),
                       reinterpret_cast<float *>(voxel_data->data()) + volume.n_voxels(),
                       volume.voxel_data->begin(),
                       [](const float &x) { return x; });
    } else {
        std::transform(reinterpret_cast<double *>(voxel_data->data()),
                       reinterpret_cast<double *>(voxel_data->data()) + volume.n_voxels(),
                       volume.voxel_data->begin(),
                       [](const double &x) { return float(x); });
    }
    
    // find the range
    volume.range.x = *std::min_element(volume.voxel_data->begin(), volume.voxel_data->end());
    volume.range.y = *std::max_element(volume.voxel_data->begin(), volume.voxel_data->end());
    std::cout << "volume range: " << volume.range << std::endl;
    // convert to [-1, 1]
    float old_range = volume.range.y - volume.range.x;
    float new_range = 2.f;

    float b = 1.f / (volume.range.y - volume.range.x) * 255.f;
    std::vector<float> &voxels = *volume.voxel_data;
    for(int i = 0; i < volume.n_voxels(); ++i){
        float a = (((voxels[i] - volume.range.x) * new_range) / old_range) + -1.f;
        voxels[i] = a;
    }
    volume.range.x = *std::min_element(volume.voxel_data->begin(), volume.voxel_data->end());
    volume.range.y = *std::max_element(volume.voxel_data->begin(), volume.voxel_data->end());
    std::cout << "volume new range: " << volume.range << std::endl;

 
    return volume;
}

struct timesteps
{
    int timeStep;
    std::string fileDir;
    timesteps(const int timestep, const std::string &fileDir);
};

timesteps::timesteps(const int timeStep, const std::string &fileDir)
    : timeStep(timeStep), fileDir(fileDir)
{}

struct sort_timestep
{
    inline bool operator() (const timesteps &a, const timesteps &b) {
        return a.timeStep < b.timeStep;
    }
};




// struct UnstructedVolume
// {
//     std::shared_ptr<std::vector<float>> cell_data = nullptr;
//     std::shared_ptr<std::vector<float>> vertex_position = nullptr;
//     std::shared_ptr<std::vector<float>> indices = nullptr;
//     std::shared_ptr<std::vector<float>> cell_index = nullptr;

// };

// UnstructedVolume load_VTK_Unstructed_Volume(std::string filename)
// {
//     UnstructedVolume volume;
//     vtkSmartPointer<vtkXMLUnstructuredGridReader> reader = vtkSmartPointer<vtkXMLUnstructuredGridReader>::New();
//     reader->SetFileName(filename.c_str());
//     reader->Update();

//     // vtkUnstructuredGrid *unstructured_grid = reader ->GetOutput();


//     return volume;

// }



