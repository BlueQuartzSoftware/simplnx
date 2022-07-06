#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FindNormInputValues inputValues;

  inputValues.PSpace = filterArgs.value<float32>(k_PSpace_Key);
  inputValues.SelectedArrayPath = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  inputValues.NormArrayPath = filterArgs.value<DataPath>(k_NormArrayPath_Key);

  return FindNorm(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT FindNormInputValues
{
  float32 PSpace;
  DataPath SelectedArrayPath;
  DataPath NormArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT FindNorm
{
public:
  FindNorm(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindNormInputValues* inputValues);
  ~FindNorm() noexcept;

  FindNorm(const FindNorm&) = delete;
  FindNorm(FindNorm&&) noexcept = delete;
  FindNorm& operator=(const FindNorm&) = delete;
  FindNorm& operator=(FindNorm&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindNormInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
