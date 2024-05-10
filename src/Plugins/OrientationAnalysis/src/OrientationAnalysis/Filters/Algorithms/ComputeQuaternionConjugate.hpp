#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ComputeQuaternionConjugateInputValues inputValues;

  inputValues.QuaternionDataArrayPath = filterArgs.value<DataPath>(k_QuaternionDataArrayPath_Key);
  inputValues.OutputDataArrayPath = filterArgs.value<DataPath>(k_OutputDataArrayPath_Key);
  inputValues.DeleteOriginalData = filterArgs.value<bool>(k_DeleteOriginalData_Key);

  return ComputeQuaternionConjugate(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT ComputeQuaternionConjugateInputValues
{
  DataPath QuaternionDataArrayPath;
  DataPath OutputDataArrayPath;
  bool DeleteOriginalData;
};

/**
 * @class
 */
class ORIENTATIONANALYSIS_EXPORT ComputeQuaternionConjugate
{
public:
  ComputeQuaternionConjugate(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeQuaternionConjugateInputValues* inputValues);
  ~ComputeQuaternionConjugate() noexcept;

  ComputeQuaternionConjugate(const ComputeQuaternionConjugate&) = delete;
  ComputeQuaternionConjugate(ComputeQuaternionConjugate&&) noexcept = delete;
  ComputeQuaternionConjugate& operator=(const ComputeQuaternionConjugate&) = delete;
  ComputeQuaternionConjugate& operator=(ComputeQuaternionConjugate&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeQuaternionConjugateInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
