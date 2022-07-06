#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ItkGrayToRGBInputValues inputValues;

  inputValues.RedArrayPath = filterArgs.value<DataPath>(k_RedArrayPath_Key);
  inputValues.GreenArrayPath = filterArgs.value<DataPath>(k_GreenArrayPath_Key);
  inputValues.BlueArrayPath = filterArgs.value<DataPath>(k_BlueArrayPath_Key);
  inputValues.NewCellArrayName = filterArgs.value<DataPath>(k_NewCellArrayName_Key);

  return ItkGrayToRGB(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct IMAGEPROCESSING_EXPORT ItkGrayToRGBInputValues
{
  DataPath RedArrayPath;
  DataPath GreenArrayPath;
  DataPath BlueArrayPath;
  DataPath NewCellArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class IMAGEPROCESSING_EXPORT ItkGrayToRGB
{
public:
  ItkGrayToRGB(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ItkGrayToRGBInputValues* inputValues);
  ~ItkGrayToRGB() noexcept;

  ItkGrayToRGB(const ItkGrayToRGB&) = delete;
  ItkGrayToRGB(ItkGrayToRGB&&) noexcept = delete;
  ItkGrayToRGB& operator=(const ItkGrayToRGB&) = delete;
  ItkGrayToRGB& operator=(ItkGrayToRGB&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ItkGrayToRGBInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
