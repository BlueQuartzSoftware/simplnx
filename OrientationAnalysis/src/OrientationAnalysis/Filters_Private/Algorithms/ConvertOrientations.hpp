#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ConvertOrientationsInputValues inputValues;

  inputValues.InputType = filterArgs.value<ChoicesParameter::ValueType>(k_InputType_Key);
  inputValues.OutputType = filterArgs.value<ChoicesParameter::ValueType>(k_OutputType_Key);
  inputValues.InputOrientationArrayPath = filterArgs.value<DataPath>(k_InputOrientationArrayPath_Key);
  inputValues.OutputOrientationArrayName = filterArgs.value<DataPath>(k_OutputOrientationArrayName_Key);

  return ConvertOrientations(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct ORIENTATIONANALYSIS_EXPORT ConvertOrientationsInputValues
{
  ChoicesParameter::ValueType InputType;
  ChoicesParameter::ValueType OutputType;
  DataPath InputOrientationArrayPath;
  DataPath OutputOrientationArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT ConvertOrientations
{
public:
  ConvertOrientations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ConvertOrientationsInputValues* inputValues);
  ~ConvertOrientations() noexcept;

  ConvertOrientations(const ConvertOrientations&) = delete;
  ConvertOrientations(ConvertOrientations&&) noexcept = delete;
  ConvertOrientations& operator=(const ConvertOrientations&) = delete;
  ConvertOrientations& operator=(ConvertOrientations&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ConvertOrientationsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
