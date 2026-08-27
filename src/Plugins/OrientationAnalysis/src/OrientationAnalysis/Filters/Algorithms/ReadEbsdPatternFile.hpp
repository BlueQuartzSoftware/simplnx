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
  std::filesystem::path inputFile;
  DataPath outputArrayPath;
  bool setScanDimensions = false;
  uint64 numberOfRows = 1;
  uint64 numberOfColumns = 1;
};

class ORIENTATIONANALYSIS_EXPORT ReadEbsdPatternFile
{
public:
  ReadEbsdPatternFile(DataStructure& dataStructure, const ReadEbsdPatternFileInputValues& inputValues, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler);
  ~ReadEbsdPatternFile() noexcept;

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
