#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

namespace complex
{

struct COMPLEXCORE_EXPORT AvizoRectilinearCoordinateWriterInputValues
{
  FileSystemPathParameter::ValueType OutputFile;
  bool WriteBinaryFile;
  DataPath GeometryPath;
  DataPath FeatureIdsArrayPath;
  StringParameter::ValueType Units;
};

/**
 * @class AvizoRectilinearCoordinateWriter
 * @brief This filter writes out a native Avizo Rectilinear Coordinate data file.
 */

class COMPLEXCORE_EXPORT AvizoRectilinearCoordinateWriter
{
public:
  AvizoRectilinearCoordinateWriter(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                   AvizoRectilinearCoordinateWriterInputValues* inputValues);
  ~AvizoRectilinearCoordinateWriter() noexcept;

  AvizoRectilinearCoordinateWriter(const AvizoRectilinearCoordinateWriter&) = delete;
  AvizoRectilinearCoordinateWriter(AvizoRectilinearCoordinateWriter&&) noexcept = delete;
  AvizoRectilinearCoordinateWriter& operator=(const AvizoRectilinearCoordinateWriter&) = delete;
  AvizoRectilinearCoordinateWriter& operator=(AvizoRectilinearCoordinateWriter&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

protected:
  Result<> generateHeader(FILE* outputFile) const;
  Result<> writeData(FILE* outputFile) const;

private:
  DataStructure& m_DataStructure;
  const AvizoRectilinearCoordinateWriterInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
