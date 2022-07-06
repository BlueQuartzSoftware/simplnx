#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ItkConvertArrayTo8BitImageInputValues inputValues;

  inputValues.SelectedArrayPath = filterArgs.value<DataPath>(k_SelectedArrayPath_Key);
  inputValues.NewArrayArrayName = filterArgs.value<DataPath>(k_NewArrayArrayName_Key);

  return ItkConvertArrayTo8BitImage(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct IMAGEPROCESSING_EXPORT ItkConvertArrayTo8BitImageInputValues
{
  DataPath SelectedArrayPath;
  DataPath NewArrayArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class IMAGEPROCESSING_EXPORT ItkConvertArrayTo8BitImage
{
public:
  ItkConvertArrayTo8BitImage(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ItkConvertArrayTo8BitImageInputValues* inputValues);
  ~ItkConvertArrayTo8BitImage() noexcept;

  ItkConvertArrayTo8BitImage(const ItkConvertArrayTo8BitImage&) = delete;
  ItkConvertArrayTo8BitImage(ItkConvertArrayTo8BitImage&&) noexcept = delete;
  ItkConvertArrayTo8BitImage& operator=(const ItkConvertArrayTo8BitImage&) = delete;
  ItkConvertArrayTo8BitImage& operator=(ItkConvertArrayTo8BitImage&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ItkConvertArrayTo8BitImageInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
