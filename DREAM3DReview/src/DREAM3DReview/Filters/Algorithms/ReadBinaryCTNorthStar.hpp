#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ReadBinaryCTNorthStarInputValues inputValues;

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

  return ReadBinaryCTNorthStar(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT ReadBinaryCTNorthStarInputValues
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

class DREAM3DREVIEW_EXPORT ReadBinaryCTNorthStar
{
public:
  ReadBinaryCTNorthStar(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadBinaryCTNorthStarInputValues* inputValues);
  ~ReadBinaryCTNorthStar() noexcept;

  ReadBinaryCTNorthStar(const ReadBinaryCTNorthStar&) = delete;
  ReadBinaryCTNorthStar(ReadBinaryCTNorthStar&&) noexcept = delete;
  ReadBinaryCTNorthStar& operator=(const ReadBinaryCTNorthStar&) = delete;
  ReadBinaryCTNorthStar& operator=(ReadBinaryCTNorthStar&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ReadBinaryCTNorthStarInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
