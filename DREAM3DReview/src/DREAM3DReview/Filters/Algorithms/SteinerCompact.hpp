#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  SteinerCompactInputValues inputValues;

  inputValues.Plane = filterArgs.value<ChoicesParameter::ValueType>(k_Plane_Key);
  inputValues.Sites = filterArgs.value<ChoicesParameter::ValueType>(k_Sites_Key);
  inputValues.VtkOutput = filterArgs.value<bool>(k_VtkOutput_Key);
  inputValues.VtkFileName = filterArgs.value<FileSystemPathParameter::ValueType>(k_VtkFileName_Key);
  inputValues.TxtOutput = filterArgs.value<bool>(k_TxtOutput_Key);
  inputValues.TxtFileName = filterArgs.value<FileSystemPathParameter::ValueType>(k_TxtFileName_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.CellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);

  return SteinerCompact(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT SteinerCompactInputValues
{
  ChoicesParameter::ValueType Plane;
  ChoicesParameter::ValueType Sites;
  bool VtkOutput;
  FileSystemPathParameter::ValueType VtkFileName;
  bool TxtOutput;
  FileSystemPathParameter::ValueType TxtFileName;
  DataPath FeatureIdsArrayPath;
  DataPath CellPhasesArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT SteinerCompact
{
public:
  SteinerCompact(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, SteinerCompactInputValues* inputValues);
  ~SteinerCompact() noexcept;

  SteinerCompact(const SteinerCompact&) = delete;
  SteinerCompact(SteinerCompact&&) noexcept = delete;
  SteinerCompact& operator=(const SteinerCompact&) = delete;
  SteinerCompact& operator=(SteinerCompact&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const SteinerCompactInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
