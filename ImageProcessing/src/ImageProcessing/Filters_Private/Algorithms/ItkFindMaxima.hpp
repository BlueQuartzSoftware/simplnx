#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ItkFindMaximaInputValues inputValues;

  inputValues.SelectedCellArrayPath = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  inputValues.Tolerance = filterArgs.value<float32>(k_Tolerance_Key);
  inputValues.NewCellArrayName = filterArgs.value<DataPath>(k_NewCellArrayName_Key);

  return ItkFindMaxima(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct IMAGEPROCESSING_EXPORT ItkFindMaximaInputValues
{
  DataPath SelectedCellArrayPath;
  float32 Tolerance;
  DataPath NewCellArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class IMAGEPROCESSING_EXPORT ItkFindMaxima
{
public:
  ItkFindMaxima(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ItkFindMaximaInputValues* inputValues);
  ~ItkFindMaxima() noexcept;

  ItkFindMaxima(const ItkFindMaxima&) = delete;
  ItkFindMaxima(ItkFindMaxima&&) noexcept = delete;
  ItkFindMaxima& operator=(const ItkFindMaxima&) = delete;
  ItkFindMaxima& operator=(ItkFindMaxima&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ItkFindMaximaInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
