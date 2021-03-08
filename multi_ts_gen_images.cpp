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
    std::cout << "start" << std::endl;
    // load raw data
    std::vector<timesteps> files;
    for(const auto &dir : args.time_series_filename){
        std::cout << "here" << std::endl;
        DIR *dp = opendir(dir.c_str());
        if (!dp) {
            throw std::runtime_error("failed to open directory: " + dir);
        }
        std::cout << "debug0" <<std::endl;
        for(dirent *e = readdir(dp); e; e = readdir(dp)){
            std::string name = e ->d_name;
            if(name.length() > 3){
                std::cout << "name: " << name << " " << name.substr(11, 4) <<  std::endl;
                // 10, name.find(".") - 19
                const int timestep = std::stoi(name.substr(16, 19));
                const std::string filename = dir + "/" + name;
                std::cout << filename << " " << timestep << std::endl;
                timesteps t(timestep - 1000, filename);
                files.push_back(t);
            }
        }
    }

    // Sort time steps 
    std::sort(files.begin(), files.end(), sort_timestep());
    // std::cout << "debug" << std::endl;

    // load volumes 
    int count = 100;
    std::vector<Volume> volumes;
    for(auto f : files){
        if(f.timeStep <= count){
            std::cout << "f.timestep: " << f.timeStep << std::endl;
            Volume volume = load_raw_volume(f.fileDir, args.dims, args.dtype);
            volume.timestep = f.timeStep;
            volumes.push_back(volume);
        }else{
            break;
        }
    }    

    // !DEBUG 
    // for(int i = 0; i < volumes.size(); i++){
    //     std::cout << volumes[i].range << std::endl;
    // }

    // image size
    vec2i imgSize;
    imgSize.x = 1024; // width
    imgSize.y = 1024; // height

    box3f worldBound = box3f(-volumes[0].dims / 2 * volumes[0].spacing, volumes[0].dims / 2 * volumes[0].spacing);
    // std::vector<vec2f> ranges;
    vec2f range = vec2f{-1, 1};
    const int num = args.n_samples;
    std::vector<Camera> cameras = gen_cameras(num, worldBound);
    std::cout << "camera pos:" << cameras.size() << std::endl;
    
    std::ofstream outfile;
    // save camera to file
    outfile.open("/home/mengjiao/Desktop/projects/ospray_viewer/channel_flow_camera.txt");
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

    // vec2f range = volumes[0].range; 

    // vec2f range_temp = volumes[v * 10 + (v % 10)].range;
    //     if(range_temp.x < range.x){
    //         range.x = range_temp.x;
    //     }
    //     if(range_temp.y > range.y){
    //         range.y = range_temp.y;
    //     }

    // for(int v = 0; v < volumes.size() / 10; v++){
    //     vec2f range = volumes[v * 10].range;
    //     for(int i = 0; i < 10; i++){
    //         vec2f range_temp = volumes[v * 10 + i].range;
    //         if(range_temp.x < range.x){
    //             range.x = range_temp.x;
    //         }
    //         if(range_temp.y > range.y){
    //             range.y = range_temp.y;
    //         }
    //     }
    //     ranges.push_back(range);
        
    // }
    std::cout << "Total range: " << range << std::endl;

    ArcballCamera arcballCamera(worldBound, imgSize);
    TransferFunctionWidget transferFcnWidget;

    // Read view npy 
    std::unique_ptr<ParamReader> p_reader(new ParamReader(args.view_file, args.opacity_file, args.color_file));
    std::cout << p_reader->params.size() << std::endl;
    std::vector<VolParam> params = p_reader->params;
    // std::cout << "opacity size: " << params[0].opacity_tf.size() << std::endl;
    // std::cout << "color size: " << params[0].color_tf.size() << std::endl;

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
        renderer.setParam("aoSamples", 1);
        renderer.setParam("shadows", true);
        renderer.setParam("pixelSamples", 10);
        renderer.setParam("backgroundColor", vec4f(1.f)); // white, transparent
        //renderer.setParam("pixelFilter", "gaussian");
        renderer.commit();

        //! Transfer function
        auto colormap = transferFcnWidget.get_colormap();
// volumes.size()
        for(int v = 0; v < volumes.size(); v++){
            std::cout << "volume: " << volumes[v].timestep << std::endl;
            // params.size()
            for(int p0 = 0; p0 < cameras.size() ; p0++){
                // for (int p1 = 0; p1 < params.size(); p1++){
                    std::cout << " p0: " << p0 << std::endl;
                    //! Create and Setup Camera
                    // Camera c = gen_cameras_from_vtk(params[p0], volumes[v]);
                    ospray::cpp::Camera camera("perspective");
                    camera.setParam("aspect", imgSize.x / (float)imgSize.y);
                    camera.setParam("position", cameras[p0].pos);
                    camera.setParam("direction", cameras[p0].dir);
                    camera.setParam("up", cameras[p0].up);
                    // camera.setParam("fovy", c.fovy);
                    camera.commit(); // commit each object to indicate modifications are done

                    //std::vector<float> colors = params[0].color_tf;
                    std::vector<float> opacities = params[4].opacity_tf;
                    // std::vector<float> colors = params[0].color_tf;
                    

                    ospray::cpp::TransferFunction transfer_function = loadTransferFunctionWithColormap(colormap, opacities, range);
                    ospray::cpp::Volume osp_volume = createStructuredVolume(volumes[v]);
                    //! Volume Model
                    ospray::cpp::VolumetricModel volume_model(osp_volume);
                    volume_model.setParam("transferFunction", transfer_function);
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

                    //! Create and Setup Framebuffer
                    ospray::cpp::FrameBuffer framebuffer(imgSize.x, imgSize.y, OSP_FB_SRGBA, OSP_FB_COLOR | OSP_FB_ACCUM);
                    framebuffer.clear();

                    for(int i = 0; i < 10; i++){
                        framebuffer.renderFrame(renderer, camera, world);
                    }
                    uint32_t *fb = (uint32_t *)framebuffer.map(OSP_FB_COLOR);
                    std::string filename = "/home/sci/mengjiao/Desktop/data/images/channel_flow/volume_" + std::to_string(volumes[v].timestep) + "_view_" + std::to_string(p0) + "_tf_0.png";
                    //std::string filename = "/home/mengjiao/Desktop/data/images/channel_flow/volume_" + std::to_string(v) + "_view_" + std::to_string(p0) + "_tf_0.png";
                    stbi_write_png(filename.c_str(), imgSize.x, imgSize.y, 4, fb, imgSize.x * 4);
                    framebuffer.unmap(fb);


                // }
            }
        }

    }
    std::cout << "Done!" << std::endl;
    ospShutdown();
    return 0;
}
