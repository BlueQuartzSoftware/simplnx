#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  GenerateMaskFromSimpleShapesInputValues inputValues;

  inputValues.MaskShape = filterArgs.value<ChoicesParameter::ValueType>(k_MaskShape_Key);
  inputValues.MaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputValues.CentersArrayPath = filterArgs.value<DataPath>(k_CentersArrayPath_Key);
  inputValues.AxesLengthArrayPath = filterArgs.value<DataPath>(k_AxesLengthArrayPath_Key);
  inputValues.BoxDimensionsArrayPath = filterArgs.value<DataPath>(k_BoxDimensionsArrayPath_Key);
  inputValues.CylinderRadiusArrayPath = filterArgs.value<DataPath>(k_CylinderRadiusArrayPath_Key);
  inputValues.CylinderHeightArrayPath = filterArgs.value<DataPath>(k_CylinderHeightArrayPath_Key);

  return GenerateMaskFromSimpleShapes(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT GenerateMaskFromSimpleShapesInputValues
{
  ChoicesParameter::ValueType MaskShape;
  DataPath MaskArrayPath;
  DataPath CentersArrayPath;
  DataPath AxesLengthArrayPath;
  DataPath BoxDimensionsArrayPath;
  DataPath CylinderRadiusArrayPath;
  DataPath CylinderHeightArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT GenerateMaskFromSimpleShapes
{
public:
  GenerateMaskFromSimpleShapes(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, GenerateMaskFromSimpleShapesInputValues* inputValues);
  ~GenerateMaskFromSimpleShapes() noexcept;

  GenerateMaskFromSimpleShapes(const GenerateMaskFromSimpleShapes&) = delete;
  GenerateMaskFromSimpleShapes(GenerateMaskFromSimpleShapes&&) noexcept = delete;
  GenerateMaskFromSimpleShapes& operator=(const GenerateMaskFromSimpleShapes&) = delete;
  GenerateMaskFromSimpleShapes& operator=(GenerateMaskFromSimpleShapes&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const GenerateMaskFromSimpleShapesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
