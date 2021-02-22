#pragma once

#include <vector>

#include "ospray/ospray_cpp.h"
#include "ospray/ospray_util.h"

#include "rkcommon/math/vec.h"
#include "rkcommon/math/box.h"

#include "dataLoader.h"

using namespace rkcommon::math;

ospray::cpp::Volume createStructuredVolume(const Volume volume)
{
  ospray::cpp::Volume osp_volume("structuredRegular");

  auto voxels = *(volume.voxel_data);
// vec3f(-volume.dims.x/ 2.f, -volume.dims.y/2.f, -volume.dims.z/2.f)
  osp_volume.setParam("gridOrigin", vec3f(0.f));
  osp_volume.setParam("gridSpacing", vec3f(1.f));
  osp_volume.setParam("data", ospray::cpp::CopiedData(voxels.data(), volume.dims));
  osp_volume.commit();
  return osp_volume;
}

ospray::cpp::TransferFunction makeTransferFunction(const std::vector<uint8_t> colormap, const vec2f &valueRange)
{
    ospray::cpp::TransferFunction transferFunction("piecewiseLinear");

    // std::vector<vec3f> colors;
    // std::vector<float> opacities;

    // if (tfColorMap == "jet") {
        // colors.emplace_back(0, 0, 0.562493);
        // colors.emplace_back(0, 0, 1);
        // colors.emplace_back(0, 1, 1);
        // colors.emplace_back(0.500008, 1, 0.500008);
        // colors.emplace_back(1, 1, 0);
        // colors.emplace_back(1, 0, 0);
        // colors.emplace_back(0.500008, 0, 0);
    // } else if (tfColorMap == "rgb") {
    //     colors.emplace_back(0, 0, 1);
    //     colors.emplace_back(0, 1, 0);
    //     colors.emplace_back(1, 0, 0);
    // } else {
    //     colors.emplace_back(0.f, 0.f, 0.f);
    //     colors.emplace_back(1.f, 1.f, 1.f);
    // }

    // std::string tfOpacityMap = "linear";

    // if (tfOpacityMap == "linear") {
    //     opacities.emplace_back(0.f);
    //     opacities.emplace_back(1.f);
    // }

    std::vector<vec3f> colors;
    std::vector<float> opacities;

    // std::cout << "color map size: " << params << std::endl;

    for (size_t i = 0; i < colormap.size() / 4; ++i) {
        vec3f c(colormap[i * 4] / 255.f, colormap[i * 4 + 1] / 255.f, colormap[i * 4 + 2] / 255.f);
        colors.push_back(c);
        if(colormap[i * 4 + 3] / 255.f < 0.1f){
            opacities.push_back(0.f);
        }else{
            opacities.push_back(colormap[i * 4 + 3] / 255.f);
        }
    }

    transferFunction.setParam("color", ospray::cpp::CopiedData(colors));
    transferFunction.setParam("opacity", ospray::cpp::CopiedData(opacities));
    transferFunction.setParam("valueRange", valueRange);
    transferFunction.commit();

    return transferFunction;
}

ospray::cpp::TransferFunction loadTransferFunction(std::vector<float> colors, std::vector<float> opacities, const vec2f &valueRange)
{
    ospray::cpp::TransferFunction transferFunction("piecewiseLinear");
    std::vector<vec3f> vec_colors;
    
    for(size_t i = 0; i < colors.size()/3; i++){
        vec3f c (colors[i * 3], colors[i * 3 + 1], colors[i * 3 + 2]);
        vec_colors.push_back(c);
    }
    transferFunction.setParam("color", ospray::cpp::CopiedData(vec_colors));
    transferFunction.setParam("opacity", ospray::cpp::CopiedData(opacities));
    transferFunction.setParam("valueRange", valueRange);
    transferFunction.commit();

    return transferFunction;
}

