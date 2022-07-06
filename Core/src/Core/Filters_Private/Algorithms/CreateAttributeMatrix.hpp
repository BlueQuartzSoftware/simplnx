#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  CreateAttributeMatrixInputValues inputValues;

  inputValues.AttributeMatrixType = filterArgs.value<ChoicesParameter::ValueType>(k_AttributeMatrixType_Key);
  inputValues.TupleDimensions = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_TupleDimensions_Key);
  inputValues.CreatedAttributeMatrix = filterArgs.value<DataPath>(k_CreatedAttributeMatrix_Key);

  return CreateAttributeMatrix(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT CreateAttributeMatrixInputValues
{
  ChoicesParameter::ValueType AttributeMatrixType;
  <<<NOT_IMPLEMENTED>>> TupleDimensions;
  /*[x]*/ DataPath CreatedAttributeMatrix;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT CreateAttributeMatrix
{
public:
  CreateAttributeMatrix(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CreateAttributeMatrixInputValues* inputValues);
  ~CreateAttributeMatrix() noexcept;

  CreateAttributeMatrix(const CreateAttributeMatrix&) = delete;
  CreateAttributeMatrix(CreateAttributeMatrix&&) noexcept = delete;
  CreateAttributeMatrix& operator=(const CreateAttributeMatrix&) = delete;
  CreateAttributeMatrix& operator=(CreateAttributeMatrix&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const CreateAttributeMatrixInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
