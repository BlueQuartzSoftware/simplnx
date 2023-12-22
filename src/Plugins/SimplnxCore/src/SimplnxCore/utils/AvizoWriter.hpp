#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT AvizoWriterInputValues
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

class SIMPLNXCORE_EXPORT AvizoWriter
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

} // namespace nx::core
