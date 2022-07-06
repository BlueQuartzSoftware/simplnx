#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ItkHoughCirclesInputValues inputValues;

  inputValues.SaveAsNewArray = filterArgs.value<bool>(k_SaveAsNewArray_Key);
  inputValues.SelectedCellArrayPath = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  inputValues.NewCellArrayName = filterArgs.value<DataPath>(k_NewCellArrayName_Key);
  inputValues.MinRadius = filterArgs.value<float32>(k_MinRadius_Key);
  inputValues.MaxRadius = filterArgs.value<float32>(k_MaxRadius_Key);
  inputValues.NumberCircles = filterArgs.value<int32>(k_NumberCircles_Key);

  return ItkHoughCircles(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct IMAGEPROCESSING_EXPORT ItkHoughCirclesInputValues
{
  bool SaveAsNewArray;
  DataPath SelectedCellArrayPath;
  DataPath NewCellArrayName;
  float32 MinRadius;
  float32 MaxRadius;
  int32 NumberCircles;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class IMAGEPROCESSING_EXPORT ItkHoughCircles
{
public:
  ItkHoughCircles(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ItkHoughCirclesInputValues* inputValues);
  ~ItkHoughCircles() noexcept;

  ItkHoughCircles(const ItkHoughCircles&) = delete;
  ItkHoughCircles(ItkHoughCircles&&) noexcept = delete;
  ItkHoughCircles& operator=(const ItkHoughCircles&) = delete;
  ItkHoughCircles& operator=(ItkHoughCircles&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ItkHoughCirclesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
