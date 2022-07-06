#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ItkKMeansInputValues inputValues;

  inputValues.Classes = filterArgs.value<int32>(k_Classes_Key);
  inputValues.Slice = filterArgs.value<bool>(k_Slice_Key);
  inputValues.SaveAsNewArray = filterArgs.value<bool>(k_SaveAsNewArray_Key);
  inputValues.SelectedCellArrayPath = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  inputValues.NewCellArrayName = filterArgs.value<DataPath>(k_NewCellArrayName_Key);

  return ItkKMeans(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct IMAGEPROCESSING_EXPORT ItkKMeansInputValues
{
  int32 Classes;
  bool Slice;
  bool SaveAsNewArray;
  DataPath SelectedCellArrayPath;
  DataPath NewCellArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class IMAGEPROCESSING_EXPORT ItkKMeans
{
public:
  ItkKMeans(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ItkKMeansInputValues* inputValues);
  ~ItkKMeans() noexcept;

  ItkKMeans(const ItkKMeans&) = delete;
  ItkKMeans(ItkKMeans&&) noexcept = delete;
  ItkKMeans& operator=(const ItkKMeans&) = delete;
  ItkKMeans& operator=(ItkKMeans&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ItkKMeansInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
