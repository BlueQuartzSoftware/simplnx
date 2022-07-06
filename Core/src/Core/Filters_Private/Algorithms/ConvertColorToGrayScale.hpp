#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ConvertColorToGrayScaleInputValues inputValues;

  inputValues.ConversionAlgorithm = filterArgs.value<ChoicesParameter::ValueType>(k_ConversionAlgorithm_Key);
  inputValues.ColorWeights = filterArgs.value<VectorFloat32Parameter::ValueType>(k_ColorWeights_Key);
  inputValues.ColorChannel = filterArgs.value<int32>(k_ColorChannel_Key);
  inputValues.InputDataArrayVector = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_InputDataArrayVector_Key);
  inputValues.CreateNewAttributeMatrix = filterArgs.value<bool>(k_CreateNewAttributeMatrix_Key);
  inputValues.OutputAttributeMatrixName = filterArgs.value<StringParameter::ValueType>(k_OutputAttributeMatrixName_Key);
  inputValues.OutputArrayPrefix = filterArgs.value<StringParameter::ValueType>(k_OutputArrayPrefix_Key);

  return ConvertColorToGrayScale(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT ConvertColorToGrayScaleInputValues
{
  ChoicesParameter::ValueType ConversionAlgorithm;
  VectorFloat32Parameter::ValueType ColorWeights;
  int32 ColorChannel;
  MultiArraySelectionParameter::ValueType InputDataArrayVector;
  bool CreateNewAttributeMatrix;
  StringParameter::ValueType OutputAttributeMatrixName;
  StringParameter::ValueType OutputArrayPrefix;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT ConvertColorToGrayScale
{
public:
  ConvertColorToGrayScale(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ConvertColorToGrayScaleInputValues* inputValues);
  ~ConvertColorToGrayScale() noexcept;

  ConvertColorToGrayScale(const ConvertColorToGrayScale&) = delete;
  ConvertColorToGrayScale(ConvertColorToGrayScale&&) noexcept = delete;
  ConvertColorToGrayScale& operator=(const ConvertColorToGrayScale&) = delete;
  ConvertColorToGrayScale& operator=(ConvertColorToGrayScale&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ConvertColorToGrayScaleInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
