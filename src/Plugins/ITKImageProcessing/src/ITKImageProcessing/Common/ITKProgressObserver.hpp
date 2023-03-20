#pragma once

#include <string>

#include <fmt/core.h>

#include "complex/Filter/IFilter.hpp"

#include "itkCommand.h"
#include "itkMacro.h"
#include "itkProcessObject.h"

namespace itk
{
class ProgressObserver : public itk::Command
{
public:
  ITK_DISALLOW_COPY_AND_MOVE(ProgressObserver);

  /** Standard class type aliases. */
  using Self = ProgressObserver;
  using Pointer = SmartPointer<Self>;

  /** Method for creation through the object factory. */
  static Pointer New(const complex::IFilter::MessageHandler& messageHandler)
  {
    Pointer smartPtr = ::itk::ObjectFactory<ProgressObserver>::Create();
    if(smartPtr == nullptr)
    {
      smartPtr = new ProgressObserver(messageHandler);
    }
    smartPtr->UnRegister();
    return smartPtr;
  }

  /** Run-time type information (and related methods). */
  itkTypeMacro(ProgressObserver, itk::Command);

  void Execute(itk::Object* caller, const itk::EventObject& event) override
  {
    Execute((const itk::Object*)caller, event);
  }

  void Execute(const itk::Object* caller, const itk::EventObject& event) override
  {
    if(!itk::ProgressEvent().CheckEvent(&event))
    {
      return;
    }

    auto now = std::chrono::steady_clock::now();
    //// Only send updates every 1 second
    if(std::chrono::duration_cast<std::chrono::milliseconds>(now - m_StartTime).count() > 1000)
    {
      const auto* processObject = dynamic_cast<const itk::ProcessObject*>(caller);
      if(processObject == nullptr)
      {
        return;
      }

      std::string progressStr = fmt::format("{:.0}%", processObject->GetProgress() * 100);

      std::string ss;
      if(m_MessagePrefix.empty())
      {
        ss = progressStr;
      }
      else
      {
        ss = fmt::format("{} : {}", m_MessagePrefix, progressStr);
      }

      m_MessageHandler(complex::IFilter::Message::Type::Info, ss);
      m_StartTime = std::chrono::steady_clock::now();
    }
  }

  void setMessagePrefix(const std::string& prefix)
  {
    m_MessagePrefix = prefix;
  }

protected:
  ProgressObserver() = delete;
  ProgressObserver(const complex::IFilter::MessageHandler& m)
  : m_MessageHandler(m)
  , m_StartTime(std::chrono::steady_clock::now())
  {
  }
  ~ProgressObserver() override = default;

private:
  const complex::IFilter::MessageHandler& m_MessageHandler;
  std::string m_MessagePrefix;
  std::chrono::steady_clock::time_point m_StartTime;
};
} // namespace itk
