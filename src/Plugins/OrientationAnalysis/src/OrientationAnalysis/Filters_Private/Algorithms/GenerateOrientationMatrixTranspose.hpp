#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  GenerateOrientationMatrixTransposeInputValues inputValues;

  inputValues.OrientationMatrixDataArrayPath = filterArgs.value<DataPath>(k_OrientationMatrixDataArrayPath_Key);
  inputValues.OutputDataArrayPath = filterArgs.value<DataPath>(k_OutputDataArrayPath_Key);
  inputValues.DeleteOriginalData = filterArgs.value<bool>(k_DeleteOriginalData_Key);

  return GenerateOrientationMatrixTranspose(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct ORIENTATIONANALYSIS_EXPORT GenerateOrientationMatrixTransposeInputValues
{
  DataPath OrientationMatrixDataArrayPath;
  DataPath OutputDataArrayPath;
  bool DeleteOriginalData;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT GenerateOrientationMatrixTranspose
{
public:
  GenerateOrientationMatrixTranspose(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                     GenerateOrientationMatrixTransposeInputValues* inputValues);
  ~GenerateOrientationMatrixTranspose() noexcept;

  GenerateOrientationMatrixTranspose(const GenerateOrientationMatrixTranspose&) = delete;
  GenerateOrientationMatrixTranspose(GenerateOrientationMatrixTranspose&&) noexcept = delete;
  GenerateOrientationMatrixTranspose& operator=(const GenerateOrientationMatrixTranspose&) = delete;
  GenerateOrientationMatrixTranspose& operator=(GenerateOrientationMatrixTranspose&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const GenerateOrientationMatrixTransposeInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
