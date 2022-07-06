#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  WaveFrontObjectFileWriterInputValues inputValues;

  inputValues.OutputWaveFrontFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputWaveFrontFile_Key);
  inputValues.TriangleGeometry = filterArgs.value<DataPath>(k_TriangleGeometry_Key);

  return WaveFrontObjectFileWriter(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT WaveFrontObjectFileWriterInputValues
{
  FileSystemPathParameter::ValueType OutputWaveFrontFile;
  DataPath TriangleGeometry;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT WaveFrontObjectFileWriter
{
public:
  WaveFrontObjectFileWriter(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WaveFrontObjectFileWriterInputValues* inputValues);
  ~WaveFrontObjectFileWriter() noexcept;

  WaveFrontObjectFileWriter(const WaveFrontObjectFileWriter&) = delete;
  WaveFrontObjectFileWriter(WaveFrontObjectFileWriter&&) noexcept = delete;
  WaveFrontObjectFileWriter& operator=(const WaveFrontObjectFileWriter&) = delete;
  WaveFrontObjectFileWriter& operator=(WaveFrontObjectFileWriter&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const WaveFrontObjectFileWriterInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
