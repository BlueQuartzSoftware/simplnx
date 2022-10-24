#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  CopyObjectInputValues inputValues;

  inputValues.ObjectToCopy = filterArgs.value<ChoicesParameter::ValueType>(k_ObjectToCopy_Key);
  inputValues.DataContainerToCopy = filterArgs.value<DataPath>(k_DataContainerToCopy_Key);
  inputValues.AttributeMatrixToCopy = filterArgs.value<DataPath>(k_AttributeMatrixToCopy_Key);
  inputValues.AttributeArrayToCopy = filterArgs.value<DataPath>(k_AttributeArrayToCopy_Key);
  inputValues.CopiedObjectName = filterArgs.value<StringParameter::ValueType>(k_CopiedObjectName_Key);

  return CopyObject(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT CopyObjectInputValues
{
  ChoicesParameter::ValueType ObjectToCopy;
  DataPath DataContainerToCopy;
  DataPath AttributeMatrixToCopy;
  DataPath AttributeArrayToCopy;
  StringParameter::ValueType CopiedObjectName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT CopyObject
{
public:
  CopyObject(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CopyObjectInputValues* inputValues);
  ~CopyObject() noexcept;

  CopyObject(const CopyObject&) = delete;
  CopyObject(CopyObject&&) noexcept = delete;
  CopyObject& operator=(const CopyObject&) = delete;
  CopyObject& operator=(CopyObject&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const CopyObjectInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
