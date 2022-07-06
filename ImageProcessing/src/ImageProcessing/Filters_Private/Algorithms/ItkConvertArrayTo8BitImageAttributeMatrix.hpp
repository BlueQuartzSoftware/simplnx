#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ItkConvertArrayTo8BitImageAttributeMatrixInputValues inputValues;

  inputValues.AttributeMatrixName = filterArgs.value<DataPath>(k_AttributeMatrixName_Key);

  return ItkConvertArrayTo8BitImageAttributeMatrix(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct IMAGEPROCESSING_EXPORT ItkConvertArrayTo8BitImageAttributeMatrixInputValues
{
  DataPath AttributeMatrixName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class IMAGEPROCESSING_EXPORT ItkConvertArrayTo8BitImageAttributeMatrix
{
public:
  ItkConvertArrayTo8BitImageAttributeMatrix(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                            ItkConvertArrayTo8BitImageAttributeMatrixInputValues* inputValues);
  ~ItkConvertArrayTo8BitImageAttributeMatrix() noexcept;

  ItkConvertArrayTo8BitImageAttributeMatrix(const ItkConvertArrayTo8BitImageAttributeMatrix&) = delete;
  ItkConvertArrayTo8BitImageAttributeMatrix(ItkConvertArrayTo8BitImageAttributeMatrix&&) noexcept = delete;
  ItkConvertArrayTo8BitImageAttributeMatrix& operator=(const ItkConvertArrayTo8BitImageAttributeMatrix&) = delete;
  ItkConvertArrayTo8BitImageAttributeMatrix& operator=(ItkConvertArrayTo8BitImageAttributeMatrix&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ItkConvertArrayTo8BitImageAttributeMatrixInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
