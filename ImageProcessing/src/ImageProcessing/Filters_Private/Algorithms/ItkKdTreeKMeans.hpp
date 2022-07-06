#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ItkKdTreeKMeansInputValues inputValues;

  inputValues.Classes = filterArgs.value<int32>(k_Classes_Key);
  inputValues.SelectedCellArrayPath = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  inputValues.NewCellArrayName = filterArgs.value<DataPath>(k_NewCellArrayName_Key);

  return ItkKdTreeKMeans(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct IMAGEPROCESSING_EXPORT ItkKdTreeKMeansInputValues
{
  int32 Classes;
  DataPath SelectedCellArrayPath;
  DataPath NewCellArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class IMAGEPROCESSING_EXPORT ItkKdTreeKMeans
{
public:
  ItkKdTreeKMeans(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ItkKdTreeKMeansInputValues* inputValues);
  ~ItkKdTreeKMeans() noexcept;

  ItkKdTreeKMeans(const ItkKdTreeKMeans&) = delete;
  ItkKdTreeKMeans(ItkKdTreeKMeans&&) noexcept = delete;
  ItkKdTreeKMeans& operator=(const ItkKdTreeKMeans&) = delete;
  ItkKdTreeKMeans& operator=(ItkKdTreeKMeans&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ItkKdTreeKMeansInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
