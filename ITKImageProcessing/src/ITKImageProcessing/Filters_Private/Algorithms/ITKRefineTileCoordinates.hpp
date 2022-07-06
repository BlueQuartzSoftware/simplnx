#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ITKRefineTileCoordinatesInputValues inputValues;

  inputValues.MontageSize = filterArgs.value<VectorInt32Parameter::ValueType>(k_MontageSize_Key);
  inputValues.ImportMode = filterArgs.value<ChoicesParameter::ValueType>(k_ImportMode_Key);
  inputValues.TileOverlap = filterArgs.value<float32>(k_TileOverlap_Key);
  inputValues.ApplyRefinedOrigin = filterArgs.value<bool>(k_ApplyRefinedOrigin_Key);
  inputValues.DataContainers = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_DataContainers_Key);
  inputValues.CommonAttributeMatrixName = filterArgs.value<StringParameter::ValueType>(k_CommonAttributeMatrixName_Key);
  inputValues.CommonDataArrayName = filterArgs.value<StringParameter::ValueType>(k_CommonDataArrayName_Key);

  return ITKRefineTileCoordinates(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct ITKIMAGEPROCESSING_EXPORT ITKRefineTileCoordinatesInputValues
{
  VectorInt32Parameter::ValueType MontageSize;
  ChoicesParameter::ValueType ImportMode;
  float32 TileOverlap;
  bool ApplyRefinedOrigin;
  <<<NOT_IMPLEMENTED>>> DataContainers;
  /*[x]*/ StringParameter::ValueType CommonAttributeMatrixName;
  StringParameter::ValueType CommonDataArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ITKIMAGEPROCESSING_EXPORT ITKRefineTileCoordinates
{
public:
  ITKRefineTileCoordinates(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ITKRefineTileCoordinatesInputValues* inputValues);
  ~ITKRefineTileCoordinates() noexcept;

  ITKRefineTileCoordinates(const ITKRefineTileCoordinates&) = delete;
  ITKRefineTileCoordinates(ITKRefineTileCoordinates&&) noexcept = delete;
  ITKRefineTileCoordinates& operator=(const ITKRefineTileCoordinates&) = delete;
  ITKRefineTileCoordinates& operator=(ITKRefineTileCoordinates&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ITKRefineTileCoordinatesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
