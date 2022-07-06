#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  PipelineAnnotationInputValues inputValues;

  inputValues.Summary = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_Summary_Key);

  return PipelineAnnotation(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT PipelineAnnotationInputValues
{
  <<<NOT_IMPLEMENTED>>> Summary;
  /*[x]*/
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT PipelineAnnotation
{
public:
  PipelineAnnotation(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, PipelineAnnotationInputValues* inputValues);
  ~PipelineAnnotation() noexcept;

  PipelineAnnotation(const PipelineAnnotation&) = delete;
  PipelineAnnotation(PipelineAnnotation&&) noexcept = delete;
  PipelineAnnotation& operator=(const PipelineAnnotation&) = delete;
  PipelineAnnotation& operator=(PipelineAnnotation&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const PipelineAnnotationInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
