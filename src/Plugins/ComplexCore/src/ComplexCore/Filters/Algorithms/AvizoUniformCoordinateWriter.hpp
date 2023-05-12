#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

namespace complex
{

struct COMPLEXCORE_EXPORT AvizoUniformCoordinateWriterInputValues
{
  FileSystemPathParameter::ValueType OutputFile;
  bool WriteBinaryFile;
  DataPath GeometryPath;
  DataPath FeatureIdsArrayPath;
  StringParameter::ValueType Units;
};

/**
 * @class AvizoUniformCoordinateWriter
 * @brief This filter writes out a native Avizo Uniform Coordinate data file.
 */

class COMPLEXCORE_EXPORT AvizoUniformCoordinateWriter
{
public:
  AvizoUniformCoordinateWriter(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, AvizoUniformCoordinateWriterInputValues* inputValues);
  ~AvizoUniformCoordinateWriter() noexcept;

  AvizoUniformCoordinateWriter(const AvizoUniformCoordinateWriter&) = delete;
  AvizoUniformCoordinateWriter(AvizoUniformCoordinateWriter&&) noexcept = delete;
  AvizoUniformCoordinateWriter& operator=(const AvizoUniformCoordinateWriter&) = delete;
  AvizoUniformCoordinateWriter& operator=(AvizoUniformCoordinateWriter&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

protected:
  Result<> generateHeader(FILE* outputFile) const;
  Result<> writeData(FILE* outputFile) const;

private:
  DataStructure& m_DataStructure;
  const AvizoUniformCoordinateWriterInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
