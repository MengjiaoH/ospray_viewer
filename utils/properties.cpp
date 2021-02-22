#include "properties.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include "TransferFunction/widgets/TransferFunctionWidget.h"

void TransferFunctionProp::Init()
{
  using tfn::tfn_widget::TransferFunctionWidget;
  // if (self != nullptr) {
  //   widget = std::make_shared<TransferFunctionWidget>
  //     ([&](const std::vector<float> &c,
  //          const std::vector<float> &a,
  //          const std::array<float, 2> &r) 
  //      {
  //        lock.lock();
  //        colors = std::vector<float>(c);
  //        alphas = std::vector<float>(a);
  //        valueRange = rkcommon::math::vec2f(r[0], r[1]);
  //        doUpdate = true;
  //        lock.unlock();
  //      });
  //   widget->setDefaultRange(valueRange_default[0],
  //                           valueRange_default[1]);
  // }
}
void TransferFunctionProp::Print()
{
  if ((!colors.empty()) && !(alphas.empty())) {
    const std::vector<float> &c = colors;
    const std::vector<float> &a = alphas;
    std::cout << std::endl
              << "static std::vector<float> colors = {" << std::endl;
    for (int i = 0; i < c.size() / 3; ++i) {
      std::cout << "    " << c[3 * i] << ", " << c[3 * i + 1] << ", "
                << c[3 * i + 2] << "," << std::endl;
    }
    std::cout << "};" << std::endl;
    std::cout << "static std::vector<float> opacities = {" << std::endl;
    for (int i = 0; i < a.size() / 2; ++i) {
      std::cout << "    " << a[2 * i + 1] << ", " << std::endl;
    }
    std::cout << "};" << std::endl << std::endl;
  }
}
void TransferFunctionProp::Draw(bool* p_open)
{
  // if (self != nullptr) {
    if (widget->drawUI(p_open)) { widget->render(128); };
  // }
}
bool TransferFunctionProp::Commit()
{
  bool update = false;
  if (doUpdate && lock.try_lock()) {
    doUpdate = false;
    ospray::cpp::CopiedData colorsData = ospray::cpp::CopiedData(colors.size() / 3, 
                                    OSP_VEC3F, 
                                    colors.data());
    colorsData.commit();
    std::vector<float> o(alphas.size() / 2);
    for (int i = 0; i < alphas.size() / 2; ++i) {
      o[i] = alphas[2 * i + 1]; 
    }
    ospray::cpp::CopiedData opacitiesData = ospray::cpp::CopiedData(o.size(), OSP_FLOAT, o.data());   
    opacitiesData.commit();
    self.setParam("colors", colorsData);
    self.setParam("opacities", opacitiesData);
    if (valueRange.update())
      self.setParam("valueRange", (rkcommon::math::vec2f&)valueRange.ref());
    self.commit();
    // ospRelease(colorsData);
    // ospRelease(opacitiesData);
    lock.unlock();
    update = true;
  }
  return update;
}