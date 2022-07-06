#pragma once

#include "ITKImageProcessing/ITKImageProcessing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ImportVectorImageStackInputValues inputValues;

  inputValues.InputFileListInfo = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_InputFileListInfo_Key);
  inputValues.Origin = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  inputValues.Spacing = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  inputValues.ConvertToGrayscale = filterArgs.value<bool>(k_ConvertToGrayscale_Key);
  inputValues.DataContainerName = filterArgs.value<DataPath>(k_DataContainerName_Key);
  inputValues.CellAttributeMatrixName = filterArgs.value<DataPath>(k_CellAttributeMatrixName_Key);
  inputValues.VectorDataArrayName = filterArgs.value<DataPath>(k_VectorDataArrayName_Key);

  return ImportVectorImageStack(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct ITKIMAGEPROCESSING_EXPORT ImportVectorImageStackInputValues
{
  <<<NOT_IMPLEMENTED>>> InputFileListInfo;
  /*[x]*/ VectorFloat32Parameter::ValueType Origin;
  VectorFloat32Parameter::ValueType Spacing;
  bool ConvertToGrayscale;
  DataPath DataContainerName;
  DataPath CellAttributeMatrixName;
  DataPath VectorDataArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ITKIMAGEPROCESSING_EXPORT ImportVectorImageStack
{
public:
  ImportVectorImageStack(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ImportVectorImageStackInputValues* inputValues);
  ~ImportVectorImageStack() noexcept;

  ImportVectorImageStack(const ImportVectorImageStack&) = delete;
  ImportVectorImageStack(ImportVectorImageStack&&) noexcept = delete;
  ImportVectorImageStack& operator=(const ImportVectorImageStack&) = delete;
  ImportVectorImageStack& operator=(ImportVectorImageStack&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ImportVectorImageStackInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
