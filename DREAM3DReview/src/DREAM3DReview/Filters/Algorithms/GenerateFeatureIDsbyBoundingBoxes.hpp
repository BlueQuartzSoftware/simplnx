#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  GenerateFeatureIDsbyBoundingBoxesInputValues inputValues;

  inputValues.FeatureIDsArrayPath = filterArgs.value<DataPath>(k_FeatureIDsArrayPath_Key);
  inputValues.FeatureAttributeMatrixArrayPath = filterArgs.value<DataPath>(k_FeatureAttributeMatrixArrayPath_Key);
  inputValues.BoxCenterArrayPath = filterArgs.value<DataPath>(k_BoxCenterArrayPath_Key);
  inputValues.BoxDimensionsArrayPath = filterArgs.value<DataPath>(k_BoxDimensionsArrayPath_Key);
  inputValues.BoxFeatureIDsArrayPath = filterArgs.value<DataPath>(k_BoxFeatureIDsArrayPath_Key);

  return GenerateFeatureIDsbyBoundingBoxes(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT GenerateFeatureIDsbyBoundingBoxesInputValues
{
  DataPath FeatureIDsArrayPath;
  DataPath FeatureAttributeMatrixArrayPath;
  DataPath BoxCenterArrayPath;
  DataPath BoxDimensionsArrayPath;
  DataPath BoxFeatureIDsArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT GenerateFeatureIDsbyBoundingBoxes
{
public:
  GenerateFeatureIDsbyBoundingBoxes(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                    GenerateFeatureIDsbyBoundingBoxesInputValues* inputValues);
  ~GenerateFeatureIDsbyBoundingBoxes() noexcept;

  GenerateFeatureIDsbyBoundingBoxes(const GenerateFeatureIDsbyBoundingBoxes&) = delete;
  GenerateFeatureIDsbyBoundingBoxes(GenerateFeatureIDsbyBoundingBoxes&&) noexcept = delete;
  GenerateFeatureIDsbyBoundingBoxes& operator=(const GenerateFeatureIDsbyBoundingBoxes&) = delete;
  GenerateFeatureIDsbyBoundingBoxes& operator=(GenerateFeatureIDsbyBoundingBoxes&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const GenerateFeatureIDsbyBoundingBoxesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
