#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  RenameAttributeMatrixInputValues inputValues;

  inputValues.SelectedAttributeMatrixPath = filterArgs.value<DataPath>(k_SelectedAttributeMatrixPath_Key);
  inputValues.NewAttributeMatrix = filterArgs.value<StringParameter::ValueType>(k_NewAttributeMatrix_Key);

  return RenameAttributeMatrix(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT RenameAttributeMatrixInputValues
{
  DataPath SelectedAttributeMatrixPath;
  StringParameter::ValueType NewAttributeMatrix;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT RenameAttributeMatrix
{
public:
  RenameAttributeMatrix(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RenameAttributeMatrixInputValues* inputValues);
  ~RenameAttributeMatrix() noexcept;

  RenameAttributeMatrix(const RenameAttributeMatrix&) = delete;
  RenameAttributeMatrix(RenameAttributeMatrix&&) noexcept = delete;
  RenameAttributeMatrix& operator=(const RenameAttributeMatrix&) = delete;
  RenameAttributeMatrix& operator=(RenameAttributeMatrix&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const RenameAttributeMatrixInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
