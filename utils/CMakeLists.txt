
# add_library(transferFunction TransferFunction/widgets/TransferFunctionWidget.cpp)
# target_link_libraries(transferFunction imgui)
#find_package(glm REQUIRED)

add_library(util
	callbacks.cpp
	ArcballCamera.cpp
	imgui_impl_glfw.cpp
	imgui_impl_opengl3.cpp
	shader.cpp
	widget.cpp
	# properties.cpp
	transfer_function_widget.cpp
	parseArgs.cpp
	ParamReader.cpp)

set_target_properties(util PROPERTIES
	CXX_STANDARD 11
	CXX_STANDARD_REQUIRED ON)

target_include_directories(util PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
	$<BUILD_INTERFACE:${OPENGL_INCLUDE_DIR}>)

target_link_libraries(util PUBLIC
	glfw
	imgui
	gl3w
	#glm
	rkcommon::rkcommon
	${OPENGL_LIBRARIES}
	${OSPRAY_LIBRARIES})