#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ChangeAngleRepresentationInputValues inputValues;
  inputValues.AnglesArrayPath = filterArgs.value<ArraySelectionParameter::ValueType>(angles_array_path);
  inputValues.ConversionTypeIndex = filterArgs.value<ChoicesParameter::ValueType>(conversion_type_index);
  return ChangeAngleRepresentation(dataStructure, messageHandler, shouldCancel, &inputValues)();

*/

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ChangeAngleRepresentationInputValues
{
  ArraySelectionParameter::ValueType AnglesArrayPath;
  ChoicesParameter::ValueType ConversionTypeIndex;
};

/**
 * @class ChangeAngleRepresentation
 * @brief This algorithm implements support code for the ChangeAngleRepresentationFilter
 */

class SIMPLNXCORE_EXPORT ChangeAngleRepresentation
{
public:
  ChangeAngleRepresentation(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ChangeAngleRepresentationInputValues* inputValues);
  ~ChangeAngleRepresentation() noexcept;

  ChangeAngleRepresentation(const ChangeAngleRepresentation&) = delete;
  ChangeAngleRepresentation(ChangeAngleRepresentation&&) noexcept = delete;
  ChangeAngleRepresentation& operator=(const ChangeAngleRepresentation&) = delete;
  ChangeAngleRepresentation& operator=(ChangeAngleRepresentation&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ChangeAngleRepresentationInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
