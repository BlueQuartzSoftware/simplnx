#pragma once

#include "SimulationIO/SimulationIO_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ImportOnScaleTableFileInputValues inputValues;

  inputValues.InputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  inputValues.Origin = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  inputValues.Spacing = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  inputValues.RectGridGeometryDesc = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_RectGridGeometryDesc_Key);
  inputValues.VolumeDataContainerName = filterArgs.value<DataPath>(k_VolumeDataContainerName_Key);
  inputValues.CellAttributeMatrixName = filterArgs.value<DataPath>(k_CellAttributeMatrixName_Key);
  inputValues.FeatureIdsArrayName = filterArgs.value<DataPath>(k_FeatureIdsArrayName_Key);
  inputValues.PhaseAttributeMatrixName = filterArgs.value<DataPath>(k_PhaseAttributeMatrixName_Key);
  inputValues.MaterialNameArrayName = filterArgs.value<DataPath>(k_MaterialNameArrayName_Key);

  return ImportOnScaleTableFile(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct SIMULATIONIO_EXPORT ImportOnScaleTableFileInputValues
{
  FileSystemPathParameter::ValueType InputFile;
  VectorFloat32Parameter::ValueType Origin;
  VectorFloat32Parameter::ValueType Spacing;
  <<<NOT_IMPLEMENTED>>> RectGridGeometryDesc;
  DataPath VolumeDataContainerName;
  DataPath CellAttributeMatrixName;
  DataPath FeatureIdsArrayName;
  DataPath PhaseAttributeMatrixName;
  DataPath MaterialNameArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SIMULATIONIO_EXPORT ImportOnScaleTableFile
{
public:
  ImportOnScaleTableFile(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ImportOnScaleTableFileInputValues* inputValues);
  ~ImportOnScaleTableFile() noexcept;

  ImportOnScaleTableFile(const ImportOnScaleTableFile&) = delete;
  ImportOnScaleTableFile(ImportOnScaleTableFile&&) noexcept = delete;
  ImportOnScaleTableFile& operator=(const ImportOnScaleTableFile&) = delete;
  ImportOnScaleTableFile& operator=(ImportOnScaleTableFile&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ImportOnScaleTableFileInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
