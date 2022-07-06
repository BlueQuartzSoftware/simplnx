#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ImportQMMeltpoolH5FileInputValues inputValues;

  inputValues.InputFiles = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_InputFiles_Key);
  inputValues.PossibleIndices = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_PossibleIndices_Key);
  inputValues.SliceRange = filterArgs.value<VectorInt32Parameter::ValueType>(k_SliceRange_Key);
  inputValues.DataContainerPath = filterArgs.value<DataPath>(k_DataContainerPath_Key);
  inputValues.VertexAttributeMatrixName = filterArgs.value<StringParameter::ValueType>(k_VertexAttributeMatrixName_Key);
  inputValues.Power = filterArgs.value<float32>(k_Power_Key);

  return ImportQMMeltpoolH5File(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT ImportQMMeltpoolH5FileInputValues
{
  <<<NOT_IMPLEMENTED>>> InputFiles;
  /*[x]*/<<<NOT_IMPLEMENTED>>> PossibleIndices;
  VectorInt32Parameter::ValueType SliceRange;
  DataPath DataContainerPath;
  StringParameter::ValueType VertexAttributeMatrixName;
  float32 Power;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT ImportQMMeltpoolH5File
{
public:
  ImportQMMeltpoolH5File(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ImportQMMeltpoolH5FileInputValues* inputValues);
  ~ImportQMMeltpoolH5File() noexcept;

  ImportQMMeltpoolH5File(const ImportQMMeltpoolH5File&) = delete;
  ImportQMMeltpoolH5File(ImportQMMeltpoolH5File&&) noexcept = delete;
  ImportQMMeltpoolH5File& operator=(const ImportQMMeltpoolH5File&) = delete;
  ImportQMMeltpoolH5File& operator=(ImportQMMeltpoolH5File&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ImportQMMeltpoolH5FileInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
