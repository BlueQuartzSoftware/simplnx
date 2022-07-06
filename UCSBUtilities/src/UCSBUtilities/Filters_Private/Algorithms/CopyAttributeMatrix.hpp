#pragma once

#include "UCSBUtilities/UCSBUtilities_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  CopyAttributeMatrixInputValues inputValues;

  inputValues.SelectedAttributeMatrixPath = filterArgs.value<DataPath>(k_SelectedAttributeMatrixPath_Key);
  inputValues.NewAttributeMatrix = filterArgs.value<DataPath>(k_NewAttributeMatrix_Key);

  return CopyAttributeMatrix(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct UCSBUTILITIES_EXPORT CopyAttributeMatrixInputValues
{
  DataPath SelectedAttributeMatrixPath;
  DataPath NewAttributeMatrix;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class UCSBUTILITIES_EXPORT CopyAttributeMatrix
{
public:
  CopyAttributeMatrix(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CopyAttributeMatrixInputValues* inputValues);
  ~CopyAttributeMatrix() noexcept;

  CopyAttributeMatrix(const CopyAttributeMatrix&) = delete;
  CopyAttributeMatrix(CopyAttributeMatrix&&) noexcept = delete;
  CopyAttributeMatrix& operator=(const CopyAttributeMatrix&) = delete;
  CopyAttributeMatrix& operator=(CopyAttributeMatrix&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const CopyAttributeMatrixInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
