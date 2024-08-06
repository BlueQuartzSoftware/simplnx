#pragma once

#include "itkConfigure.h"


#include <itkCommand.h>
#include <itkProcessObject.h>

#include "simplnx/Filter/IFilter.hpp"

namespace itk
{

class Dream3DFilterInterruption : public Command
{
public:
  ITK_DISALLOW_COPY_AND_MOVE(Dream3DFilterInterruption);

  /** Standard class type aliases. */
  using Self = Dream3DFilterInterruption;
  using Pointer = SmartPointer<Self>;

  /** Method for creation through the object factory. */
  static Pointer New(const std::atomic_bool& shouldCancel)
  {
    Pointer smartPtr = ::itk::ObjectFactory<Dream3DFilterInterruption>::Create();
    if(smartPtr == nullptr)
    {
      smartPtr = new Dream3DFilterInterruption(shouldCancel);
    }
    smartPtr->UnRegister();
    return smartPtr;
  }

  /** Run-time type information (and related methods). */
#if defined(ITK_VERSION_MAJOR) && ITK_VERSION_MINOR == 2
  itkTypeMacro(Dream3DFilterInterruption, itk::Command);
#else
  itkOverrideGetNameOfClassMacro(Dream3DFilterInterruption);
#endif

  void Execute(Object* caller, const EventObject& event) override
  {
    if(m_ShouldCancel)
    {
      ProcessObject* po = dynamic_cast<ProcessObject*>(caller);
      if(po)
      {
        Execute(po);
      }
    }
  }

  void Execute(ProcessObject* object)
  {
    object->AbortGenerateDataOn();
  }

  void Execute(const Object* o_not_used, const EventObject& eo_not_used) override // has to be implemented
  {
  }

protected:
  Dream3DFilterInterruption() = delete;
  Dream3DFilterInterruption(const std::atomic_bool& shouldCancel)
  : m_ShouldCancel(shouldCancel)
  {
  }
  ~Dream3DFilterInterruption() override = default;

private:
  const std::atomic_bool& m_ShouldCancel;
};

} // namespace itk
