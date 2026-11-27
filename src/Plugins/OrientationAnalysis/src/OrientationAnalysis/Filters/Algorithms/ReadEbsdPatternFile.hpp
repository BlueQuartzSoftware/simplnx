#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <filesystem>

namespace nx::core
{
struct ORIENTATIONANALYSIS_EXPORT ReadEbsdPatternFileInputValues
{
  std::filesystem::path input_file;
  DataPath output_array_path;
  bool set_scan_dimensions = false;
  uint64 number_of_rows = 1;
  uint64 number_of_columns = 1;
};

class ORIENTATIONANALYSIS_EXPORT ReadEbsdPatternFile
{
public:
  ReadEbsdPatternFile(DataStructure& dataStructure, const ReadEbsdPatternFileInputValues& inputValues, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler);
  ~ReadEbsdPatternFile() noexcept = default;

  ReadEbsdPatternFile(const ReadEbsdPatternFile&) = delete;
  ReadEbsdPatternFile(ReadEbsdPatternFile&&) noexcept = delete;
  ReadEbsdPatternFile& operator=(const ReadEbsdPatternFile&) = delete;
  ReadEbsdPatternFile& operator=(ReadEbsdPatternFile&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ReadEbsdPatternFileInputValues& m_InputValues;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
