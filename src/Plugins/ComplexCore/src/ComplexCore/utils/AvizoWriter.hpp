#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

namespace complex
{

struct COMPLEXCORE_EXPORT AvizoWriterInputValues
{
  FileSystemPathParameter::ValueType OutputFile;
  bool WriteBinaryFile;
  DataPath GeometryPath;
  DataPath FeatureIdsArrayPath;
  StringParameter::ValueType Units;
};

/**
 * @class AvizoWriter
 * @brief This filter writes out a native Avizo Uniform Coordinate data file.
 */

class COMPLEXCORE_EXPORT AvizoWriter
{
public:
  AvizoWriter(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, AvizoWriterInputValues* inputValues);
  virtual ~AvizoWriter() noexcept;

  AvizoWriter(const AvizoWriter&) = delete;
  AvizoWriter(AvizoWriter&&) noexcept = delete;
  AvizoWriter& operator=(const AvizoWriter&) = delete;
  AvizoWriter& operator=(AvizoWriter&&) noexcept = delete;

  Result<> execute();

  const std::atomic_bool& getCancel();

protected:
  virtual Result<> generateHeader(FILE* outputFile) const = 0;
  virtual Result<> writeData(FILE* outputFile) const = 0;

  DataStructure& m_DataStructure;
  const AvizoWriterInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
