#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ITKMedianImageInputValues inputValues;

  inputValues.Radius = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Radius_Key);
  inputValues.SelectedCellArrayPath = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  inputValues.NewCellArrayName = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  return ITKMedianImage(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct ITKIMAGEPROCESSING_EXPORT ITKMedianImageInputValues
{
  VectorFloat32Parameter::ValueType Radius;
  DataPath SelectedCellArrayPath;
  StringParameter::ValueType NewCellArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ITKIMAGEPROCESSING_EXPORT ITKMedianImage
{
public:
  ITKMedianImage(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ITKMedianImageInputValues* inputValues);
  ~ITKMedianImage() noexcept;

  ITKMedianImage(const ITKMedianImage&) = delete;
  ITKMedianImage(ITKMedianImage&&) noexcept = delete;
  ITKMedianImage& operator=(const ITKMedianImage&) = delete;
  ITKMedianImage& operator=(ITKMedianImage&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ITKMedianImageInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
