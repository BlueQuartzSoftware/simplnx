#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  GenerateQuaternionConjugateInputValues inputValues;

  inputValues.QuaternionDataArrayPath = filterArgs.value<DataPath>(k_QuaternionDataArrayPath_Key);
  inputValues.OutputDataArrayPath = filterArgs.value<DataPath>(k_OutputDataArrayPath_Key);
  inputValues.DeleteOriginalData = filterArgs.value<bool>(k_DeleteOriginalData_Key);

  return GenerateQuaternionConjugate(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct ORIENTATIONANALYSIS_EXPORT GenerateQuaternionConjugateInputValues
{
  DataPath QuaternionDataArrayPath;
  DataPath OutputDataArrayPath;
  bool DeleteOriginalData;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT GenerateQuaternionConjugate
{
public:
  GenerateQuaternionConjugate(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, GenerateQuaternionConjugateInputValues* inputValues);
  ~GenerateQuaternionConjugate() noexcept;

  GenerateQuaternionConjugate(const GenerateQuaternionConjugate&) = delete;
  GenerateQuaternionConjugate(GenerateQuaternionConjugate&&) noexcept = delete;
  GenerateQuaternionConjugate& operator=(const GenerateQuaternionConjugate&) = delete;
  GenerateQuaternionConjugate& operator=(GenerateQuaternionConjugate&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const GenerateQuaternionConjugateInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
