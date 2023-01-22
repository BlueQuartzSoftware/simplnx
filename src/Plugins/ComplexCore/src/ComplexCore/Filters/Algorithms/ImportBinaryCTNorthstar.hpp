#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"


/**
* This is example code to put in the Execute Method of the filter.
  ImportBinaryCTNorthstarInputValues inputValues;

  inputValues.InputHeaderFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputHeaderFile_Key);
  inputValues.DataContainerName = filterArgs.value<StringParameter::ValueType>(k_DataContainerName_Key);
  inputValues.CellAttributeMatrixName = filterArgs.value<DataPath>(k_CellAttributeMatrixName_Key);
  inputValues.DensityArrayName = filterArgs.value<DataPath>(k_DensityArrayName_Key);
  inputValues.LengthUnit = filterArgs.value<ChoicesParameter::ValueType>(k_LengthUnit_Key);
  inputValues.VolumeDescription = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_VolumeDescription_Key);
  inputValues.DataFileInfo = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_DataFileInfo_Key);
  inputValues.ImportSubvolume = filterArgs.value<bool>(k_ImportSubvolume_Key);
  inputValues.StartVoxelCoord = filterArgs.value<VectorInt32Parameter::ValueType>(k_StartVoxelCoord_Key);
  inputValues.EndVoxelCoord = filterArgs.value<VectorInt32Parameter::ValueType>(k_EndVoxelCoord_Key);
  inputValues.ImportedVolumeDescription = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_ImportedVolumeDescription_Key);

  return ImportBinaryCTNorthstar(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct COMPLEXCORE_EXPORT ImportBinaryCTNorthstarInputValues
{
  FileSystemPathParameter::ValueType InputHeaderFile;
  StringParameter::ValueType DataContainerName;
  DataPath CellAttributeMatrixName;
  DataPath DensityArrayName;
  ChoicesParameter::ValueType LengthUnit;
  <<<NOT_IMPLEMENTED>>> VolumeDescription;
  <<<NOT_IMPLEMENTED>>> DataFileInfo;
  bool ImportSubvolume;
  VectorInt32Parameter::ValueType StartVoxelCoord;
  VectorInt32Parameter::ValueType EndVoxelCoord;
  <<<NOT_IMPLEMENTED>>> ImportedVolumeDescription;

};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class COMPLEXCORE_EXPORT ImportBinaryCTNorthstar
{
public:
  ImportBinaryCTNorthstar(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ImportBinaryCTNorthstarInputValues* inputValues);
  ~ImportBinaryCTNorthstar() noexcept;

  ImportBinaryCTNorthstar(const ImportBinaryCTNorthstar&) = delete;
  ImportBinaryCTNorthstar(ImportBinaryCTNorthstar&&) noexcept = delete;
  ImportBinaryCTNorthstar& operator=(const ImportBinaryCTNorthstar&) = delete;
  ImportBinaryCTNorthstar& operator=(ImportBinaryCTNorthstar&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ImportBinaryCTNorthstarInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
