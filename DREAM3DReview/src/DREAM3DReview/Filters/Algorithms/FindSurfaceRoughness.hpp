#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FindSurfaceRoughnessInputValues inputValues;

  inputValues.BoundaryCellsArrayPath = filterArgs.value<DataPath>(k_BoundaryCellsArrayPath_Key);
  inputValues.AttributeMatrixName = filterArgs.value<StringParameter::ValueType>(k_AttributeMatrixName_Key);
  inputValues.RoughnessParamsArrayName = filterArgs.value<StringParameter::ValueType>(k_RoughnessParamsArrayName_Key);

  return FindSurfaceRoughness(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT FindSurfaceRoughnessInputValues
{
  DataPath BoundaryCellsArrayPath;
  StringParameter::ValueType AttributeMatrixName;
  StringParameter::ValueType RoughnessParamsArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT FindSurfaceRoughness
{
public:
  FindSurfaceRoughness(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindSurfaceRoughnessInputValues* inputValues);
  ~FindSurfaceRoughness() noexcept;

  FindSurfaceRoughness(const FindSurfaceRoughness&) = delete;
  FindSurfaceRoughness(FindSurfaceRoughness&&) noexcept = delete;
  FindSurfaceRoughness& operator=(const FindSurfaceRoughness&) = delete;
  FindSurfaceRoughness& operator=(FindSurfaceRoughness&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindSurfaceRoughnessInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
