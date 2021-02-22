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
// #include "dataLoader.h"
#include "ospray_volume.h"

#include <vtkSmartPointer.h>
#include <vtkDoubleArray.h>
#include <vtkUnstructuredGridReader.h>
#include <vtkUnstructuredGrid.h>
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkPointSet.h>
#include <vtkIdList.h>

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


struct UnstructedVolume
{
    std::vector<float> vertex_data;
    std::vector<vec3f> vertex_position;
    std::vector<uint32_t> indices;
    std::vector<uint32_t> cell_index;
    std::vector<uint8_t> cell_types;
    vec2f range;

};

UnstructedVolume load_VTK_Unstructed_Volume(std::string filename)
{
    UnstructedVolume volume;
    
    auto reader = vtkSmartPointer<vtkUnstructuredGridReader>::New();
	reader->SetFileName(filename.c_str());
	reader->Update();
	vtkSmartPointer<vtkUnstructuredGrid> ugrid = reader->GetOutput();
	int num_cells = ugrid->GetNumberOfCells();
	std::cout << "Number of cells: " <<	num_cells << std::endl;

	int num_points = ugrid->GetNumberOfPoints();
	std::cout << "Number of points: " <<	num_points << std::endl;

    auto numberOfPointArrays = ugrid->GetPointData()->GetNumberOfArrays();
    std::cout << "Number of PointData arrays: " << numberOfPointArrays << std::endl;
	
	// float *pts = (float*)malloc(sizeof(float)*3*num_points);
	// This array will store all the vertex locations in the format x0 y0 z0 x1 y1 z1 x2 y2 z2 .. xn yn zn
	
	for(int i = 0; i < num_points; i++)
	{
		double p[3];
		ugrid->GetPoint(i, p);
        float p_x = (float)p[0];
        float p_y = (float)p[1];
        float p_z = (float)p[2];
		volume.vertex_position.push_back(vec3f{p_x, p_y, p_z});
        // [i*3+0] = p[0];
		// volume.vertex_position.push_back(p[1]);
        // [i*3+1] = p[1];
		// volume.vertex_position.push_back(p[2]);
        // [i*3+2] = p[2];
	}
	
	// std::cout << "First point : " << volume.vertex_position[0] << " " << volume.vertex_position[1] << " " << volume.vertex_position[2] << std::endl; 
	// std::cout << "Last point : " << volume.vertex_position[num_points*3-3] << " " << volume.vertex_position[num_points*3-2] << " " << volume.vertex_position[num_points*3-1] << std::endl; 

    // vertex data 
    vtkSmartPointer<vtkDoubleArray> array = vtkDoubleArray::SafeDownCast(ugrid->GetPointData()->GetArray("p"));
    if(array){
        for(int i = 0; i < num_points; i++){
            double value;
            value = array->GetValue(i);
            float v = (float)value;
            volume.vertex_data.push_back(v);
            // std::cout << i << ": " << value << std::endl;
        }
    }else{
        std::cout << "ERROR: no array" << std::endl;
    }

    volume.range.x = *std::min_element(volume.vertex_data.begin(), volume.vertex_data.end());
    volume.range.y = *std::max_element(volume.vertex_data.begin(), volume.vertex_data.end());
    std::cout << "volume range: " << volume.range << std::endl;


    vtkSmartPointer<vtkIdList> pointIds = vtkSmartPointer<vtkIdList>::New();

	// Assuming each cell has 8 vertices
	// int *point_indices = (int*)malloc(sizeof(int)*num_cells*8);
	// int *cell_start_index = (int*)malloc(sizeof(int)*num_cells);
	// int counter = 0;

	for(int i = 0; i < num_cells; i++)
	{
		// cell_start_index[i] = counter;
        // volume.cell_index.push_back(counter);
        

		ugrid->GetCellPoints(i, pointIds);
		for(int n = 0; n < pointIds->GetNumberOfIds(); n++)
		{
            if (n == 0){
                volume.cell_index.push_back(pointIds->GetId(n));
                volume.cell_types.push_back(OSP_HEXAHEDRON);
            }
			// counter++;
			// point_indices[i*8+n] = pointIds->GetId(n);		
            volume.indices.push_back(pointIds->GetId(n));
		}
	}		
	// Test print
	// std::cout << point_indices[0] << " " << point_indices[8] << " " << point_indices[(num_cells*8)-8] << std::endl;
	// std::cout << cell_start_index[0] << " " << cell_start_index[1] << " " << cell_start_index[num_cells-1] << std::endl;

    return volume;

}

ospray::cpp::Volume createUnstructuredVolume(const UnstructedVolume volume)
{
  ospray::cpp::Volume osp_volume("unstructured");

  osp_volume.setParam("vertex.position", ospray::cpp::CopiedData(volume.vertex_position));
  osp_volume.setParam("vertex.data", ospray::cpp::CopiedData(volume.vertex_data));
//   osp_volume.setParam("indexPrefixed", true);
  osp_volume.setParam("index", ospray::cpp::CopiedData(volume.indices));
  osp_volume.setParam("cell.index", ospray::cpp::CopiedData(volume.cell_index));
  osp_volume.setParam("cell.type", ospray::cpp::CopiedData(volume.cell_types));
  osp_volume.commit();
  return osp_volume;
}



int main(int argc, const char **argv)
{
    //! Read Arguments
    Args args;
    parseArgs(argc, argv, args);

    // load vtk unstructed data 
    
    UnstructedVolume volume = load_VTK_Unstructed_Volume(args.filename);
    //Debug 
    std::cout << "number of vertex: " << volume.vertex_position.size() << std::endl;
    std::cout << "number of vertex data: " << volume.vertex_data.size() << std::endl;
    std::cout << "number of index: " << volume.indices.size() << std::endl;
    std::cout << "number of cell index: " << volume.cell_index.size() << std::endl;

    for(int i = 0; i < 16; i++){
        std::cout << volume.vertex_position[i] << std::endl;
        std::cout << volume.vertex_data[i] << std::endl;
        std::cout << volume.indices[i] << std::endl;
        std::cout << volume.cell_index[i] << std::endl;
    }
    
    const float hSize = .4f;
    const float hX = -.5f, hY = -.5f, hZ = 0.f;
    std::vector<vec3f> vertices = {// hexahedron
//     {-3.14159, -3.14159, 0},
//     {-3.04342, -3.14159, 0}, 
//     {-2.94524, -3.14159, 0 },
//     {-2.84707, -3.14159, 0},
//     {-2.74889, -3.14159, 0},
//     {-2.65072, -3.14159, 0}, 
// -2.55254 -3.14159 0 -2.45437 -3.14159 0
      {-hSize + hX, -hSize + hY, hSize + hZ}, // bottom quad
      {hSize + hX, -hSize + hY, hSize + hZ},
      {hSize + hX, -hSize + hY, -hSize + hZ},
      {-hSize + hX, -hSize + hY, -hSize + hZ},
      {-hSize + hX, hSize + hY, hSize + hZ}, // top quad
      {hSize + hX, hSize + hY, hSize + hZ},
      {hSize + hX, hSize + hY, -hSize + hZ},
      {-hSize + hX, hSize + hY, -hSize + hZ}
    };
    std::vector<float> vertexValues = {// hexahedron
      0.f, 0.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f};

    std::vector<uint32_t> indicesSharedVert = {// hexahedron
      0, 1, 2, 3, 4, 5, 6, 7};
    std::vector<uint32_t> &indices = indicesSharedVert;

    // define cell offsets in indices array
    std::vector<uint32_t> cells = {0};
    // define cell types
    std::vector<uint8_t> cellTypes = {OSP_HEXAHEDRON};


    // image size
    vec2i imgSize;
    imgSize.x = 1024; // width
    imgSize.y = 768; // height

    box3f worldBound;
    worldBound.lower = vec3f{-3.14159, -3.14159, 0};
    worldBound.upper = vec3f{3.14159, 3.14159, 6.28319};
    
    vec2f range = vec2f{0, 1};
    // volume.range; 
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
        camera.setParam("position", vec3f{0, 0, -10});
        // arcballCamera.eyePos()
        camera.setParam("direction", arcballCamera.lookDir());
        camera.setParam("up", arcballCamera.upDir());
        camera.commit(); // commit each object to indicate modifications are done

        //! Transfer function
        auto colormap = transferFcnWidget.get_colormap();
        // range = vec2f{0, 1};
		ospray::cpp::TransferFunction transfer_function = makeTransferFunction(colormap, range);
        //! Volume
		// ospray::cpp::Volume osp_volume = createUnstructuredVolume(volume);
        ospray::cpp::Volume osp_volume("unstructured");

        // set data objects for volume object
        osp_volume.setParam("vertex.position", ospray::cpp::CopiedData(vertices));

        osp_volume.setParam("vertex.data", ospray::cpp::CopiedData(vertexValues));

        osp_volume.setParam("index", ospray::cpp::CopiedData(indicesSharedVert));
        osp_volume.setParam("cell.index", ospray::cpp::CopiedData(cells));
        osp_volume.setParam("cell.type", ospray::cpp::CopiedData(cellTypes));

        osp_volume.commit();

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
        ospray::cpp::Light light("ambient");
        light.commit();

        world.setParam("light", ospray::cpp::CopiedData(light));
        world.commit();

        //! Create Renderer, Choose Scientific Visualization Renderer
        ospray::cpp::Renderer renderer("scivis");

        //! Complete Setup of Renderer
        renderer.setParam("aoSamples", 0);
        renderer.setParam("shadows", true);
        renderer.setParam("backgroundColor", 1.f); // white, transparent
        renderer.setParam("pixelFilter", "gaussian");
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