#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

/**
* This is example code to put in the Execute Method of the filter.


*/

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ConditionalSetValueInputValues
{
  ArraySelectionParameter::ValueType ConditionalArrayPath;
  BoolParameter::ValueType InvertMask;
  StringParameter::ValueType RemoveValue;
  StringParameter::ValueType ReplaceValue;
  ArraySelectionParameter::ValueType SelectedArrayPath;
  BoolParameter::ValueType UseConditional;
};

/**
 * @class ConditionalSetValue
 * @brief This algorithm implements support code for the ConditionalSetValueFilter
 */

class SIMPLNXCORE_EXPORT ConditionalSetValue
{
public:
  ConditionalSetValue(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ConditionalSetValueInputValues* inputValues);
  ~ConditionalSetValue() noexcept;

  ConditionalSetValue(const ConditionalSetValue&) = delete;
  ConditionalSetValue(ConditionalSetValue&&) noexcept = delete;
  ConditionalSetValue& operator=(const ConditionalSetValue&) = delete;
  ConditionalSetValue& operator=(ConditionalSetValue&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ConditionalSetValueInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
