#pragma once 

#include <iostream>
#include <time.h>
#include <vector>
#include <fstream>
#include "vtkAutoInit.h"
VTK_MODULE_INIT(vtkRenderingOpenGL2); 
#include "vtkCamera.h"
#include "ParamReader.h"

#include "rkcommon/math/vec.h"
#include "rkcommon/math/box.h"

using namespace rkcommon::math;

struct Camera {
    vec3f pos;
    vec3f dir;
    vec3f up;
    float fovy = 45;

    Camera(const vec3f &pos, const vec3f &dir, const vec3f &up);
    void setFovy(float f){
        fovy = f;
    }
};

Camera::Camera(const vec3f &pos, const vec3f &dir, const vec3f &up)
    : pos(pos), dir(dir), up(up)
{
}

Camera gen_cameras_from_vtk(VolParam param, Volume volume)
{
    float vol_max[3];
    vol_max[0] = volume.origin[0] + volume.spacing[0] * volume.dims[0];
    vol_max[1] = volume.origin[1] + volume.spacing[1] * volume.dims[1];
    vol_max[2] = volume.origin[2] + volume.spacing[2] * volume.dims[2];

    float vol_cen[3];
    vol_cen[0] = 0.5f * (volume.origin[0] + vol_max[0]);
    vol_cen[1] = 0.5f * (volume.origin[1] + vol_max[1]);
    vol_cen[2] = 0.5f * (volume.origin[2] + vol_max[2]);
    
    vtkSmartPointer<vtkCamera> vtk_cam = vtkCamera::New();
    // std::cout << "sub debug 0" << std::endl;
    double pos[3] = {vol_cen[0], 2 * -(vol_max[1] - vol_cen[1]), vol_cen[2]};
    vtk_cam->SetPosition(pos);
    // std::cout << "sub debug 1" << std::endl;
    double foc[3] = {vol_cen[0], vol_cen[1], vol_cen[2]};
    vtk_cam->SetFocalPoint(foc);
    double up[3] = {-1, 0, 0};
    vtk_cam->SetViewUp(up);
    vtk_cam->SetViewAngle(75.);

    vtk_cam->Elevation(-85.f);

    vtk_cam->Elevation(param.view_param[0]);
    vtk_cam->Azimuth(param.view_param[1]);
    vtk_cam->Roll(param.view_param[2]);
    vtk_cam->Zoom(param.view_param[3]);
    // std::cout << "zoom " << param.view_param[3] << "\n";

    vtk_cam->GetPosition(pos);
    vtk_cam->GetFocalPoint(foc);
    vtk_cam->OrthogonalizeViewUp();
    vtk_cam->GetViewUp(up);

    double fov = vtk_cam->GetViewAngle();

    float cam_pos[3] = {static_cast<float>(pos[0]), static_cast<float>(pos[1]),
                        static_cast<float>(pos[2])};
    float cam_foc[3] = {static_cast<float>(foc[0]), static_cast<float>(foc[1]),
                        static_cast<float>(foc[2])};
    float cam_up[3] = {static_cast<float>(up[0]), static_cast<float>(up[1]),
                       static_cast<float>(up[2])};
    float cam_dir[3] = {cam_foc[0] - cam_pos[0], cam_foc[1] - cam_pos[1],
                        cam_foc[2] - cam_pos[2]};
    // std::cout << "sub debug 1" << std::endl;
    Camera camera = Camera(cam_pos, cam_dir, cam_up);
    camera.setFovy(fov);
    return camera;
}