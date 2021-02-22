#pragma once

/* ospray */
#include "ospray/ospray_cpp.h"
#include "rkcommon/math/vec.h"
#include "rkcommon/utility/TransactionalValue.h"
/* imgui */
#include "TransferFunction/widgets/TransferFunctionWidget.h"
/* stl */
#include <vector>


#define TValue rkcommon::utility::TransactionalValue
#define Setter(field, name, type, value)                 \
private:                                                 \
  TValue<type> field{value};                             \
  type imgui_##field = field.ref();                      \
public:                                                  \
  void Set##name (const type& v) {                       \
    this->field = v;                                     \
    this->imgui_##field = v;                             \
  }


struct Prop { 
    Prop() {}
    virtual ~Prop() {}
    virtual void Draw() = 0; 
    virtual bool Commit() = 0; 
};

class TransferFunctionProp : public Prop
{
  private:
    ospray::cpp::TransferFunction self{nullptr};
    bool doUpdate{false}; // no initial update
    std::shared_ptr<tfn::tfn_widget::TransferFunctionWidget> widget;
    std::mutex lock;
    std::vector<float> colors;
    std::vector<float> alphas;
    float valueRange_default[2] = {0.f, 1.f};
    TValue<rkcommon::math::vec2f> valueRange;
  public:
    TransferFunctionProp() = default;
    ospray::cpp::TransferFunction& operator*() { return self; }
    void Create(ospray::cpp::TransferFunction o, 
                const float a = 0.f, 
                const float b = 1.f)
    {
      self             = o;
      valueRange_default[0] = a;
      valueRange_default[1] = b;
    }
    void Init();
    void Draw(bool* p_open = NULL);
    void Draw() { Draw(NULL); };
    bool Commit();
    void Print();
};

#undef Setter
#undef TValue