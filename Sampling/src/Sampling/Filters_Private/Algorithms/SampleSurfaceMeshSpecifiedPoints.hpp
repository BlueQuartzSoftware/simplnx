#pragma once

#include "Sampling/Sampling_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  SampleSurfaceMeshSpecifiedPointsInputValues inputValues;

  inputValues.SurfaceMeshFaceLabelsArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  inputValues.InputFilePath = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFilePath_Key);
  inputValues.OutputFilePath = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputFilePath_Key);

  return SampleSurfaceMeshSpecifiedPoints(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct SAMPLING_EXPORT SampleSurfaceMeshSpecifiedPointsInputValues
{
  DataPath SurfaceMeshFaceLabelsArrayPath;
  FileSystemPathParameter::ValueType InputFilePath;
  FileSystemPathParameter::ValueType OutputFilePath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SAMPLING_EXPORT SampleSurfaceMeshSpecifiedPoints
{
public:
  SampleSurfaceMeshSpecifiedPoints(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                   SampleSurfaceMeshSpecifiedPointsInputValues* inputValues);
  ~SampleSurfaceMeshSpecifiedPoints() noexcept;

  SampleSurfaceMeshSpecifiedPoints(const SampleSurfaceMeshSpecifiedPoints&) = delete;
  SampleSurfaceMeshSpecifiedPoints(SampleSurfaceMeshSpecifiedPoints&&) noexcept = delete;
  SampleSurfaceMeshSpecifiedPoints& operator=(const SampleSurfaceMeshSpecifiedPoints&) = delete;
  SampleSurfaceMeshSpecifiedPoints& operator=(SampleSurfaceMeshSpecifiedPoints&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const SampleSurfaceMeshSpecifiedPointsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
