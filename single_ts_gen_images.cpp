// cpp
#include <iostream> 
#include <vector>
#include <dirent.h>
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
#include "ParamReader.h"
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
    //! Load Raw Data
    Volume volume = load_raw_volume(args.filename, args.dims, args.dtype);
    // image size
    vec2i imgSize;
    imgSize.x = 512; // width
    imgSize.y = 512; // height

    box3f worldBound = box3f(-volume.dims / 2 * volume.spacing, volume.dims / 2 * volume.spacing);;
    vec2f range = vec2f{-1, 1};
    const int num = args.n_samples;
    std::vector<Camera> cameras = gen_cameras(num, worldBound);
    std::cout << "camera pos:" << cameras.size() << std::endl;
    std::ofstream outfile;
    // save camera to file
    outfile.open("/home/mengjiao/Desktop/projects/ospray_viewer/input.txt");
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
    
    ArcballCamera arcballCamera(worldBound, imgSize);
    TransferFunctionWidget transferFcnWidget;

    // Read Meta Files 
    std::unique_ptr<ParamReader> p_reader(new ParamReader(args.view_file, args.opacity_file, args.color_file));
    std::cout << p_reader->params.size() << std::endl;
    std::vector<VolParam> params = p_reader->params;
    std::cout << "opacity size: " << params[0].opacity_tf.size() << std::endl;
    std::cout << "color size: " << params[0].color_tf.size() << std::endl;
// initialize OSPRay; OSPRay parses (and removes) its commandline parameters,
    // e.g. "--osp:debug"
    OSPError init_error = ospInit(&argc, argv);
    if (init_error != OSP_NO_ERROR)
        return init_error;

    {
        //! Create and Setup Light for Ambient Occlusion
        std::vector<ospray::cpp::Light> lights;
        {
            ospray::cpp::Light light("ambient");
            light.setParam("intensity", 0.4f);
            // light.setParam("color", vec3f(1.f));
            // light.setParam("visible", true);
            light.commit();
            lights.push_back(light);
        }
        {
            ospray::cpp::Light light("distant");
            light.setParam("intensity", 0.8f);
            light.setParam("direction", vec3f(150.f));
            light.commit();
            lights.push_back(light);
        }

        //! Create Renderer, Choose Scientific Visualization Renderer
        ospray::cpp::Renderer renderer("scivis");

        //! Complete Setup of Renderer
        renderer.setParam("aoSamples", 10);
        renderer.setParam("shadows", true);
        renderer.setParam("pixelSamples", 10);
        renderer.setParam("backgroundColor", vec4f(1.f)); // white, transparent
        //renderer.setParam("pixelFilter", "gaussian");
        renderer.commit();

        //! Volume
        ospray::cpp::Volume osp_volume = createStructuredVolume(volume);
        //! Volume Model
        ospray::cpp::VolumetricModel volume_model(osp_volume);
        volume_model.commit();

        ospray::cpp::Group volume_group;
        volume_group.setParam("volume", ospray::cpp::CopiedData(volume_model));
        volume_group.commit();
        ospray::cpp::Instance volume_instance(volume_group);
        volume_instance.commit();
        //! Put the Instance in the World
        ospray::cpp::World world;
        world.setParam("instance", ospray::cpp::CopiedData(volume_instance));
        world.setParam("light", ospray::cpp::CopiedData(lights));
        world.commit();
        
        auto colormap = transferFcnWidget.get_colormap();
        // for(int m = 0; m < params.size(); m++){ // loop transfer function
            for (int c = 0; c < cameras.size(); c++){
                // Camera c = gen_cameras_from_vtk(params[m], volume); // c.pos
                ospray::cpp::Camera camera("perspective");
                camera.setParam("aspect", imgSize.x / (float)imgSize.y);
                camera.setParam("position",cameras[c].pos );
                camera.setParam("direction", cameras[c].dir);
                camera.setParam("up", cameras[c].up);
                // camera.setParam("fovy", c.fovy);
                camera.commit(); // commit each object to indicate modifications are done
                std::vector<float> opacities = params[33].opacity_tf;
                // std::vector<float> colors = params[m].color_tf;

                ospray::cpp::TransferFunction transfer_function = loadTransferFunctionWithColormap(colormap, opacities, range);
                volume_model.setParam("transferFunction", transfer_function);
                volume_model.commit();
                volume_group.commit();

                //! Create and Setup Framebuffer
                ospray::cpp::FrameBuffer framebuffer(imgSize.x, imgSize.y, OSP_FB_SRGBA, OSP_FB_COLOR | OSP_FB_ACCUM);
                framebuffer.clear();

                for(int i = 0; i < 10; i++){
                    framebuffer.renderFrame(renderer, camera, world);
                }
                uint32_t *fb = (uint32_t *)framebuffer.map(OSP_FB_COLOR);
                std::string filename = "/home/sci/mengjiao/Desktop/data/images/2008/meta_" + std::to_string(33) + "_view_" + std::to_string(c) + ".png";
                stbi_write_png(filename.c_str(), imgSize.x, imgSize.y, 4, fb, imgSize.x * 4);
                framebuffer.unmap(fb);
            }
        // }
    }
}
