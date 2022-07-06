#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ItkWatershedInputValues inputValues;

  inputValues.SelectedCellArrayPath = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  inputValues.FeatureIdsArrayName = filterArgs.value<DataPath>(k_FeatureIdsArrayName_Key);
  inputValues.Threshold = filterArgs.value<float32>(k_Threshold_Key);
  inputValues.Level = filterArgs.value<float32>(k_Level_Key);

  return ItkWatershed(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct IMAGEPROCESSING_EXPORT ItkWatershedInputValues
{
  DataPath SelectedCellArrayPath;
  DataPath FeatureIdsArrayName;
  float32 Threshold;
  float32 Level;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class IMAGEPROCESSING_EXPORT ItkWatershed
{
public:
  ItkWatershed(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ItkWatershedInputValues* inputValues);
  ~ItkWatershed() noexcept;

  ItkWatershed(const ItkWatershed&) = delete;
  ItkWatershed(ItkWatershed&&) noexcept = delete;
  ItkWatershed& operator=(const ItkWatershed&) = delete;
  ItkWatershed& operator=(ItkWatershed&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ItkWatershedInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
