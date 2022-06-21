#pragma once

#include "ImportExport/ImportExport_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  VisualizeGBCDGMTInputValues inputValues;

  inputValues.PhaseOfInterest = filterArgs.value<int32>(k_PhaseOfInterest_Key);
  inputValues.MisorientationRotation = filterArgs.value<VectorFloat32Parameter::ValueType>(k_MisorientationRotation_Key);
  inputValues.OutputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key);
  inputValues.GBCDArrayPath = filterArgs.value<DataPath>(k_GBCDArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);

  return VisualizeGBCDGMT(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct IMPORTEXPORT_EXPORT VisualizeGBCDGMTInputValues
{
  int32 PhaseOfInterest;
  VectorFloat32Parameter::ValueType MisorientationRotation;
  FileSystemPathParameter::ValueType OutputFile;
  DataPath GBCDArrayPath;
  DataPath CrystalStructuresArrayPath;

};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class IMPORTEXPORT_EXPORT VisualizeGBCDGMT
{
public:
  VisualizeGBCDGMT(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, VisualizeGBCDGMTInputValues* inputValues);
  ~VisualizeGBCDGMT() noexcept;

  VisualizeGBCDGMT(const VisualizeGBCDGMT&) = delete;
  VisualizeGBCDGMT(VisualizeGBCDGMT&&) noexcept = delete;
  VisualizeGBCDGMT& operator=(const VisualizeGBCDGMT&) = delete;
  VisualizeGBCDGMT& operator=(VisualizeGBCDGMT&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const VisualizeGBCDGMTInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
