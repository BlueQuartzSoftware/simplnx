#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ChangeAngleRepresentationInputValues inputValues;

  inputValues.ConversionType = filterArgs.value<ChoicesParameter::ValueType>(k_ConversionType_Key);
  inputValues.CellEulerAnglesArrayPath = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);

  return ChangeAngleRepresentation(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct ORIENTATIONANALYSIS_EXPORT ChangeAngleRepresentationInputValues
{
  ChoicesParameter::ValueType ConversionType;
  DataPath CellEulerAnglesArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT ChangeAngleRepresentation
{
public:
  ChangeAngleRepresentation(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ChangeAngleRepresentationInputValues* inputValues);
  ~ChangeAngleRepresentation() noexcept;

  ChangeAngleRepresentation(const ChangeAngleRepresentation&) = delete;
  ChangeAngleRepresentation(ChangeAngleRepresentation&&) noexcept = delete;
  ChangeAngleRepresentation& operator=(const ChangeAngleRepresentation&) = delete;
  ChangeAngleRepresentation& operator=(ChangeAngleRepresentation&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ChangeAngleRepresentationInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
