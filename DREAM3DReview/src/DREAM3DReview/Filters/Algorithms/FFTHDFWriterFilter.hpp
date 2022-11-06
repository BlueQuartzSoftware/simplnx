#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FFTHDFWriterFilterInputValues inputValues;

  inputValues.OutputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFile_Key);
  inputValues.WriteEigenstrains = filterArgs.value<bool>(k_WriteEigenstrains_Key);
  inputValues.EigenstrainsOutputFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_EigenstrainsOutputFile_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.CellEulerAnglesArrayPath = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);
  inputValues.CellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.CellEigenstrainsArrayPath = filterArgs.value<DataPath>(k_CellEigenstrainsArrayPath_Key);

  return FFTHDFWriterFilter(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT FFTHDFWriterFilterInputValues
{
  FileSystemPathParameter::ValueType OutputFile;
  bool WriteEigenstrains;
  FileSystemPathParameter::ValueType EigenstrainsOutputFile;
  DataPath FeatureIdsArrayPath;
  DataPath CellEulerAnglesArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath CellEigenstrainsArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT FFTHDFWriterFilter
{
public:
  FFTHDFWriterFilter(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FFTHDFWriterFilterInputValues* inputValues);
  ~FFTHDFWriterFilter() noexcept;

  FFTHDFWriterFilter(const FFTHDFWriterFilter&) = delete;
  FFTHDFWriterFilter(FFTHDFWriterFilter&&) noexcept = delete;
  FFTHDFWriterFilter& operator=(const FFTHDFWriterFilter&) = delete;
  FFTHDFWriterFilter& operator=(FFTHDFWriterFilter&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FFTHDFWriterFilterInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
