// cpp
#include <iostream> 
#include <vector>
#include <random>
#include <sstream>
// OpenGL
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
// ospray
#include "ospray/ospray_cpp.h"
#include "ospray/ospray_cpp/ext/rkcommon.h"
// imgui 
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
// utils 
#include "callbacks.h"
#include "transfer_function_widget.h"
#include "shader.h"
#include "ArcballCamera.h"
#include "parseArgs.h"
#include "dataLoader.h"
#include "ospray_volume.h"
#include "ospray_camera.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

using namespace rkcommon::math;

const std::string fullscreen_quad_vs = R"(
#version 420 core
const vec4 pos[4] = vec4[4](
	vec4(-1, 1, 0.5, 1),
	vec4(-1, -1, 0.5, 1),
	vec4(1, 1, 0.5, 1),
	vec4(1, -1, 0.5, 1)
);
void main(void){
	gl_Position = pos[gl_VertexID];
}
)";

const std::string display_texture_fs = R"(
#version 420 core
layout(binding=0) uniform sampler2D img;
out vec4 color;
void main(void){ 
	ivec2 uv = ivec2(gl_FragCoord.xy);
	color = texelFetch(img, uv, 0);
})";

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

int main(int argc, const char **argv)
{
    //! Read Arguments
    Args args;
    parseArgs(argc, argv, args);

    // image size
    vec2i imgSize;
    imgSize.x = 1024; // width
    imgSize.y = 1024; // height

	box3f worldBound = box3f(-args.dims / 2, args.dims/ 2);

    Volume volume = load_raw_volume(args.filename, args.dims, args.dtype);

    std::vector<Camera> cameras = gen_cameras(args.n_samples, worldBound);
    std::cout << "camera pos:" << cameras.size() << std::endl;
    std::ofstream outfile;
    // save camera to file
    outfile.open("/home/mengjiao/Desktop/projects/ospray_viewer/ispsurface_camers.txt");
    for(int i = 0; i < cameras.size(); i++){
        outfile << cameras[i].pos.x << " " <<
                   cameras[i].pos.y << " " <<
                   cameras[i].pos.z << " " <<
                   cameras[i].dir.x << " " <<
                   cameras[i].dir.y << " " <<
                   cameras[i].dir.z << " " <<
                   cameras[i].up.x  << " " <<
                   cameras[i].up.y  << " " <<
                   cameras[i].up.z  << "\n";
    }
    outfile.close();

    OSPError init_error = ospInit(&argc, argv);
    if (init_error != OSP_NO_ERROR)
        return init_error;

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    {
        // create ospray volume 
        float isovalues = 9.5f;
        ospray::cpp::Volume osp_volume = createStructuredVolume(volume);
        osp_volume.commit();

        for(float iso = isovalues; iso <= 158.f; iso += 1.5){

            ospray::cpp::Geometry isosurfaces("isosurface");
            isosurfaces.setParam("isovalue", ospray::cpp::CopiedData(iso));
            isosurfaces.setParam("volume", osp_volume);
            isosurfaces.commit();
            
            // Geometry Model 
            ospray::cpp::GeometricModel geo_model(isosurfaces);
            geo_model.setParam("color", ospray::cpp::CopiedData(vec4f(0.023f, 0.224f, 0.439f, 1.f)));
            // std::cout << "debug" << std::endl;
            geo_model.commit();
            
            // Group
            ospray::cpp::Group group;
            group.setParam("geometry", ospray::cpp::CopiedData(geo_model));
            group.commit();
            // Instance
            ospray::cpp::Instance instance(group);
            instance.commit();
            // World
            ospray::cpp::World world;
            world.setParam("instance", ospray::cpp::CopiedData(instance));
            // Lights
            //! Create and Setup Light for Ambient Occlusion
            std::vector<ospray::cpp::Light> lights;
            {
                ospray::cpp::Light light("ambient");
                light.setParam("intensity", 0.3f);
                light.setParam("color", vec3f(1.f));
                light.setParam("visible", true);
                light.commit();
                lights.push_back(light);
            }
            {
                ospray::cpp::Light light("distant");
                light.setParam("intensity", 3.f);
                light.setParam("direction", vec3f(0.f));
                light.setParam("color", vec3f(0.023f, 0.224f, 0.439f));
                light.setParam("angularDiameter", 1.0f);
                light.commit();
                lights.push_back(light);
            }
            world.setParam("light", ospray::cpp::CopiedData(lights));
            world.commit();

            //! Create Renderer, Choose Scientific Visualization Renderer
            ospray::cpp::Renderer renderer("scivis");

            //! Complete Setup of Renderer
            renderer.setParam("shadows", true);
            // renderer.setParam("visibleLights", true);
            renderer.setParam("aoSamples", 1);
            renderer.setParam("pixelSamples", 10);
            renderer.setParam("backgroundColor", vec4f(0.f, 0.f, 0.f, 1.f)); // white, transparent
            renderer.setParam("pixelFilter", "gaussian");
            renderer.commit();

            //! Create and Setup Framebuffer
            ospray::cpp::FrameBuffer framebuffer(imgSize.x, imgSize.y, OSP_FB_SRGBA, OSP_FB_COLOR | OSP_FB_ACCUM);
            framebuffer.clear();

            for(int c = 0; c < cameras.size(); c++){
                ospray::cpp::Camera camera("perspective");
                camera.setParam("aspect", imgSize.x / (float)imgSize.y);
                camera.setParam("position",cameras[c].pos );
                camera.setParam("direction", cameras[c].dir);
                camera.setParam("up", cameras[c].up);
                // camera.setParam("fovy", c.fovy);
                camera.commit();
                
                framebuffer.clear();
                for(int i = 0; i < 10; i++){
                    framebuffer.renderFrame(renderer, camera, world);
                }
                uint32_t *fb = (uint32_t *)framebuffer.map(OSP_FB_COLOR);
                std::ostringstream stream;
                stream << iso;
                std::string filename = "/home/mengjiao/Desktop/data/images/tangle_volume/tangle_volume_camera_" + std::to_string(c) + "_iso_" + stream.str() + ".png";
                stbi_write_png(filename.c_str(), imgSize.x, imgSize.y, 4, fb, imgSize.x * 4);
                framebuffer.unmap(fb);
            }
    }

    }
}