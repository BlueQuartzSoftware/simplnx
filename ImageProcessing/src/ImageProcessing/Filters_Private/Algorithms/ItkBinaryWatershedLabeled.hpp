#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ItkBinaryWatershedLabeledInputValues inputValues;

  inputValues.SelectedCellArrayPath = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  inputValues.PeakTolerance = filterArgs.value<float32>(k_PeakTolerance_Key);
  inputValues.NewCellArrayName = filterArgs.value<DataPath>(k_NewCellArrayName_Key);

  return ItkBinaryWatershedLabeled(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct IMAGEPROCESSING_EXPORT ItkBinaryWatershedLabeledInputValues
{
  DataPath SelectedCellArrayPath;
  float32 PeakTolerance;
  DataPath NewCellArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class IMAGEPROCESSING_EXPORT ItkBinaryWatershedLabeled
{
public:
  ItkBinaryWatershedLabeled(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ItkBinaryWatershedLabeledInputValues* inputValues);
  ~ItkBinaryWatershedLabeled() noexcept;

  ItkBinaryWatershedLabeled(const ItkBinaryWatershedLabeled&) = delete;
  ItkBinaryWatershedLabeled(ItkBinaryWatershedLabeled&&) noexcept = delete;
  ItkBinaryWatershedLabeled& operator=(const ItkBinaryWatershedLabeled&) = delete;
  ItkBinaryWatershedLabeled& operator=(ItkBinaryWatershedLabeled&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ItkBinaryWatershedLabeledInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
