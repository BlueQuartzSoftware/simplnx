#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ComputeUmeyamaTransformInputValues inputValues;

  inputValues.UseScaling = filterArgs.value<bool>(k_UseScaling_Key);
  inputValues.SourcePointSet = filterArgs.value<DataPath>(k_SourcePointSet_Key);
  inputValues.DestPointSet = filterArgs.value<DataPath>(k_DestPointSet_Key);
  inputValues.TransformationAttributeMatrixName = filterArgs.value<DataPath>(k_TransformationAttributeMatrixName_Key);
  inputValues.TransformationMatrixName = filterArgs.value<DataPath>(k_TransformationMatrixName_Key);

  return ComputeUmeyamaTransform(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT ComputeUmeyamaTransformInputValues
{
  bool UseScaling;
  DataPath SourcePointSet;
  DataPath DestPointSet;
  DataPath TransformationAttributeMatrixName;
  DataPath TransformationMatrixName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT ComputeUmeyamaTransform
{
public:
  ComputeUmeyamaTransform(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeUmeyamaTransformInputValues* inputValues);
  ~ComputeUmeyamaTransform() noexcept;

  ComputeUmeyamaTransform(const ComputeUmeyamaTransform&) = delete;
  ComputeUmeyamaTransform(ComputeUmeyamaTransform&&) noexcept = delete;
  ComputeUmeyamaTransform& operator=(const ComputeUmeyamaTransform&) = delete;
  ComputeUmeyamaTransform& operator=(ComputeUmeyamaTransform&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeUmeyamaTransformInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
