// cpp
#include <iostream> 
#include <vector>
#include <random>
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

ospray::cpp::Geometry generate_random_spheres(int num_spheres, std::vector<vec4f> &colors, const box3f world_bound);

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

    std::vector<Camera> cameras = gen_cameras(args.n_samples, worldBound);
    std::cout << "camera pos:" << cameras.size() << std::endl;
    std::ofstream outfile;
    // save camera to file
    outfile.open("/home/mengjiao/Desktop/projects/ospray_viewer/random_sphere_camers.txt");
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
    /* FOR VIEWER
    ArcballCamera arcballCamera(worldBound, imgSize);
    std::shared_ptr<App> app;
    app = std::make_shared<App>(imgSize, arcballCamera);
    */

    // initialize OSPRay; OSPRay parses (and removes) its commandline parameters,
    // e.g. "--osp:debug"
    OSPError init_error = ospInit(&argc, argv);
    if (init_error != OSP_NO_ERROR)
        return init_error;
    
    //!! Create GLFW Window
    // GLFWwindow* window;
    // glfwSetErrorCallback(glfw_error_callback);
    // if (!glfwInit()){
    //     std::cout << "Cannot Initialize GLFW!! " << std::endl;
    //     exit(EXIT_FAILURE);
    // }else{
    //     std::cout << "Initialize GLFW!! " << std::endl;
    // }

    // glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    // glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

	// window = glfwCreateWindow(imgSize.x, imgSize.y, "OSPRay Viewer", NULL, NULL);

    // if (!window){
    //     std::cout << "Why not opening a window" << std::endl;
    //     glfwTerminate();
    //     exit(EXIT_FAILURE);
    // }else{
    //     std::cout << "Aha! Window opens successfully!!" << std::endl;
    // }
    // glfwMakeContextCurrent(window);
    // glfwSwapInterval(1);
    

    // if (gl3wInit()) {
    //     fprintf(stderr, "failed to initialize OpenGL\n");
    //     return -1;
    // }
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // // Setup Platform/Renderer bindings
    // ImGui_ImplGlfw_InitForOpenGL(window, true);
    // ImGui_ImplOpenGL3_Init();

    // Shader display_render(fullscreen_quad_vs, display_texture_fs);

    // GLuint render_texture;
	// glGenTextures(1, &render_texture);
	// glBindTexture(GL_TEXTURE_2D, render_texture);
	// glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, imgSize.x, imgSize.y);

	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// GLuint vao;
	// glCreateVertexArrays(1, &vao);
	// glBindVertexArray(vao);

	// glClearColor(0.0, 0.0, 0.0, 0.0);
	// glDisable(GL_DEPTH_TEST);

    {
        // //! Create and Setup Camera
        // ospray::cpp::Camera camera("perspective");
        // camera.setParam("aspect", imgSize.x / (float)imgSize.y);
        // camera.setParam("position", arcballCamera.eyePos());
        // camera.setParam("direction", arcballCamera.lookDir());
        // camera.setParam("up", arcballCamera.upDir());
        // camera.commit(); // commit each object to indicate modifications are done

        //! Generate random spheres
        int num_spheres = 90;
        std::vector<vec4f> colors;
        ospray::cpp::Geometry sphere = generate_random_spheres(num_spheres, colors, worldBound);

        // Geometry Model 
        ospray::cpp::GeometricModel geo_model(sphere);
        geo_model.setParam("color", ospray::cpp::CopiedData(colors));
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
            light.setParam("intensity", 0.4f);
            // light.setParam("color", vec3f(1.f));
            // light.setParam("visible", true);
            light.commit();
            lights.push_back(light);
        }
        {
            ospray::cpp::Light light("distant");
            light.setParam("intensity", 0.4f);
            light.setParam("direction", vec3f(0.f));
            light.commit();
            lights.push_back(light);
        }
        world.setParam("light", ospray::cpp::CopiedData(lights));
        world.commit();

        //! Create Renderer, Choose Scientific Visualization Renderer
        ospray::cpp::Renderer renderer("scivis");

        //! Complete Setup of Renderer
        renderer.setParam("shadows", true);
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
            {
                renderer.setParam("aoSamples", 1);
                renderer.commit();
                framebuffer.clear();
                for(int i = 0; i < 10; i++){
                    framebuffer.renderFrame(renderer, camera, world);
                }
                uint32_t *fb = (uint32_t *)framebuffer.map(OSP_FB_COLOR);
                std::string filename = "/home/mengjiao/Desktop/data/images/random_spheres/with_ao/view_with_ao_" + std::to_string(c) + ".png";
                stbi_write_png(filename.c_str(), imgSize.x, imgSize.y, 4, fb, imgSize.x * 4);
                framebuffer.unmap(fb);
            }
            {
                renderer.setParam("aoSamples", 0);
                renderer.commit();
                framebuffer.clear();
                for(int i = 0; i < 10; i++){
                    framebuffer.renderFrame(renderer, camera, world);
                }
                uint32_t *fb = (uint32_t *)framebuffer.map(OSP_FB_COLOR);
                std::string filename = "/home/mengjiao/Desktop/data/images/random_spheres/without_ao/view_without_ao_" + std::to_string(c) + ".png";
                stbi_write_png(filename.c_str(), imgSize.x, imgSize.y, 4, fb, imgSize.x * 4);
                framebuffer.unmap(fb);

            }

        }

        // glfwSetWindowUserPointer(window, app.get());
        // glfwSetCursorPosCallback(window, cursorPosCallback);

        /* FOR VIEWER
        // while (!glfwWindowShouldClose(window))
        // {
        //     if (app ->isCameraChanged) {
        //         camera.setParam("position", app->camera.eyePos());
        //         camera.setParam("direction", app->camera.lookDir());
        //         camera.setParam("up", app->camera.upDir());
        //         // std::cout << "camera pos " << app->camera.eyePos() << std::endl;
        //         // std::cout << "camera look dir " << app->camera.lookDir() << std::endl;
        //         // std::cout << "camera up dir " << app->camera.upDir() << std::endl;
        //         camera.commit();
        //         framebuffer.clear();
        //         app ->isCameraChanged = false;
        //     }
        //     // render one frame
        //     framebuffer.renderFrame(renderer, camera, world);
        //     // framebuffer.clear();

        //     // access framebuffer and write its content as PPM file
        //     uint32_t *fb = (uint32_t *)framebuffer.map(OSP_FB_COLOR);
        //     glViewport(0, 0, imgSize.x, imgSize.y);
		//     glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, imgSize.x, imgSize.y, GL_RGBA, GL_UNSIGNED_BYTE, fb);

        //     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        //     glUseProgram(display_render.program);
        //     glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        //     framebuffer.unmap(fb);
        //     glfwSwapBuffers(window);
        //     glfwPollEvents();
        // }
        */
    }
    // glfwDestroyWindow(window);
    // glfwTerminate();

    ospShutdown();

    return 0;

}

ospray::cpp::Geometry generate_random_spheres(int num_spheres, std::vector<vec4f> &colors, const box3f world_bound)
{
    int32_t randomSeed = 0;
    std::vector<vec3f> s_center(num_spheres);
    std::vector<float> s_radius(num_spheres);
    colors.resize(num_spheres);

    // create random number distributions for sphere center, radius, and color
    std::mt19937 gen(randomSeed);

    std::uniform_real_distribution<float> centerDistribution(world_bound.lower.x, world_bound.upper.x);
    std::uniform_real_distribution<float> radiusDistribution(2.f, 4.f);
    std::uniform_real_distribution<float> colorDistribution(0.5f, 1.f);

    // populate the spheres

    for (auto &center : s_center) {
        center.x = centerDistribution(gen);
        center.y = centerDistribution(gen);
        center.z = centerDistribution(gen);
    }

    for (auto &radius : s_radius)
        radius = radiusDistribution(gen);

    for (auto &c : colors) {
        c.x = colorDistribution(gen);
        c.y = colorDistribution(gen);
        c.z = colorDistribution(gen);
        c.w = colorDistribution(gen);
    }
    ospray::cpp::Geometry spheresGeometry("sphere");

    spheresGeometry.setParam("sphere.position", ospray::cpp::CopiedData(s_center));
    spheresGeometry.setParam("sphere.radius", ospray::cpp::CopiedData(s_radius));
    spheresGeometry.commit();

    return spheresGeometry;

}