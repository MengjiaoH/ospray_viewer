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

std::vector<vec3f> generate_fibonacci_sphere(const size_t n_points, const float radius)
{
    const float increment = M_PI * (3.f - std::sqrt(5.f));
    const float offset = 2.f / n_points;
    std::vector<vec3f> points;
    points.reserve(n_points);
    for (size_t i = 0; i < n_points; ++i) {
        const float y = ((i * offset) - 1.f) + offset / 2.f;
        const float r = std::sqrt(1.f - y * y);
        const float phi = i * increment;
        const float x = r * std::cos(phi);
        const float z = r * std::sin(phi);
        // std::cout << i << " " << x << " " << y << " " << z << "\n";
        points.emplace_back(x * radius, y * radius, z * radius);
    }
    return points;
}

std::vector<Camera> gen_cameras(const int num, const box3f &world_bounds){
    std::vector<Camera> cameras;
    std::srand((unsigned)time(NULL));
    // int part = num / 500 ;
    int part = num;
    float minimun = 0.8;
    float diff = 1 - minimun;
    for (int i = 0; i < part; i++){
        float scale = ((float) rand()/RAND_MAX) * diff + minimun;
        // float scale = 0.9;
        const float orbit_radius = length(world_bounds.size()) * scale;
        std::cout << "orbit radius: " << scale << " " << orbit_radius << std::endl;
        auto orbit_points = generate_fibonacci_sphere(10, orbit_radius);
        // std::cout << "points " << orbit_points.size() << "\n";
        for (const auto &p : orbit_points) {
            cameras.emplace_back(p + world_bounds.center(), normalize(-p), vec3f(0, -1, 0));
        }
    }

    // std::cout << "camera pos has " << cameras.size() << std::endl;

    

    return cameras;
}
