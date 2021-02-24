// cpp
#include <iostream> 
#include <vector>
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
    // Load raw data 
    
    Volume volume = load_raw_volume(args.filename, args.dims, args.dtype);
	
    // image size
    vec2i imgSize;
    imgSize.x = 1024; // width
    imgSize.y = 768; // height

	box3f worldBound = box3f(-volume.dims / 2 * volume.spacing, volume.dims / 2 * volume.spacing);
    vec2f range = volume.range; 

    ArcballCamera arcballCamera(worldBound, imgSize);
    std::shared_ptr<App> app;
    app = std::make_shared<App>(imgSize, arcballCamera);
    TransferFunctionWidget transferFcnWidget;

    // initialize OSPRay; OSPRay parses (and removes) its commandline parameters,
    // e.g. "--osp:debug"
    OSPError init_error = ospInit(&argc, argv);
    if (init_error != OSP_NO_ERROR)
        return init_error;
    
    //!! Create GLFW Window
    GLFWwindow* window;
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()){
        std::cout << "Cannot Initialize GLFW!! " << std::endl;
        exit(EXIT_FAILURE);
    }else{
        std::cout << "Initialize GLFW!! " << std::endl;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);

	window = glfwCreateWindow(imgSize.x, imgSize.y, "OSPRay Viewer", NULL, NULL);

    if (!window){
        std::cout << "Why not opening a window" << std::endl;
        glfwTerminate();
        exit(EXIT_FAILURE);
    }else{
        std::cout << "Aha! Window opens successfully!!" << std::endl;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (gl3wInit()) {
        fprintf(stderr, "failed to initialize OpenGL\n");
        return -1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();

    Shader display_render(fullscreen_quad_vs, display_texture_fs);

    GLuint render_texture;
	glGenTextures(1, &render_texture);
	glBindTexture(GL_TEXTURE_2D, render_texture);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, imgSize.x, imgSize.y);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	GLuint vao;
	glCreateVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glClearColor(0.0, 0.0, 0.0, 0.0);
	glDisable(GL_DEPTH_TEST);

    // use scoped lifetimes of wrappers to release everything before ospShutdown()
    {
        //! Create and Setup Camera
        ospray::cpp::Camera camera("perspective");
        camera.setParam("aspect", imgSize.x / (float)imgSize.y);
        camera.setParam("position", arcballCamera.eyePos());
        camera.setParam("direction", arcballCamera.lookDir());
        camera.setParam("up", arcballCamera.upDir());
        camera.commit(); // commit each object to indicate modifications are done

        //! Transfer function
        auto colormap = transferFcnWidget.get_colormap();
		ospray::cpp::TransferFunction transfer_function = makeTransferFunction(colormap, range);
        // ospray::cpp::Volume osp_volume = createSphericalVolume(volume);
        // }else{
		//! Volume
        // if(args.vtype == "spherical"){
            
        ospray::cpp::Volume osp_volume = createStructuredVolume(volume);
        // }
		
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
        
        // {
        //     ospray::cpp::Light light("distant");
        //     light.setParam("intensity", 0.4f);
        //     light.setParam("direction", vec3f(1.f));
        //     light.commit();
        //     lights.push_back(light)
        // }

        world.setParam("light", ospray::cpp::CopiedData(lights));
        world.commit();

        //! Create Renderer, Choose Scientific Visualization Renderer
        ospray::cpp::Renderer renderer("scivis");

        //! Complete Setup of Renderer
        renderer.setParam("aoSamples", 1);
        renderer.setParam("shadows", true);
        renderer.setParam("backgroundColor", 1.f); // white, transparent
        //renderer.setParam("pixelFilter", "gaussian");
        renderer.commit();

        //! Create and Setup Framebuffer
        ospray::cpp::FrameBuffer framebuffer(imgSize.x, imgSize.y, OSP_FB_SRGBA, OSP_FB_COLOR | OSP_FB_ACCUM);
        framebuffer.clear();

        glfwSetWindowUserPointer(window, app.get());
        glfwSetCursorPosCallback(window, cursorPosCallback);

        while (!glfwWindowShouldClose(window))
        {
            app -> isTransferFcnChanged = transferFcnWidget.changed();
            if (app ->isCameraChanged) {
                camera.setParam("position", app->camera.eyePos());
                camera.setParam("direction", app->camera.lookDir());
                camera.setParam("up", app->camera.upDir());
                // std::cout << "camera pos " << app->camera.eyePos() << std::endl;
                // std::cout << "camera look dir " << app->camera.lookDir() << std::endl;
                // std::cout << "camera up dir " << app->camera.upDir() << std::endl;
                camera.commit();
                framebuffer.clear();
                app ->isCameraChanged = false;
            }
            if (app -> isTransferFcnChanged) {
                // std::cout << "transfer function changed!" << std::endl;
			    auto colormap = transferFcnWidget.get_colormap();
                transfer_function = makeTransferFunction(colormap, range);
                transfer_function.commit();
                volume_model.setParam("transferFunction", transfer_function);
                volume_model.commit();
                framebuffer.clear();
                app ->isTransferFcnChanged = false;
		    }
            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            if (ImGui::Begin("Transfer Function")) {
                transferFcnWidget.draw_ui();
            }
            ImGui::End();
            ImGui::Render();

            // render one frame
            framebuffer.renderFrame(renderer, camera, world);
            framebuffer.clear();

            // access framebuffer and write its content as PPM file
            uint32_t *fb = (uint32_t *)framebuffer.map(OSP_FB_COLOR);
            glViewport(0, 0, imgSize.x, imgSize.y);
		    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, imgSize.x, imgSize.y, GL_RGBA, GL_UNSIGNED_BYTE, fb);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glUseProgram(display_render.program);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            framebuffer.unmap(fb);

            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    ospShutdown();

    return 0;
}
