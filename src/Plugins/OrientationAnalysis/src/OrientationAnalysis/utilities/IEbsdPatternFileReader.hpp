#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <atomic>
#include <filesystem>
#include <optional>
#include <string>

namespace nx::core
{
struct ORIENTATIONANALYSIS_EXPORT EbsdPatternFileInfo
{
  std::string formatName;
  uint32 headerVersion = 0;
  DataType pixelDataType = DataType::uint8;
  uint64 bytesPerPixel = 0;
  uint64 patternWidth = 0;
  uint64 patternHeight = 0;
  uint64 dataOffset = 0;
  uint64 patternStride = 0;
  uint64 numberOfPatterns = 0;
  std::optional<uint64> numberOfRows;
  std::optional<uint64> numberOfColumns;
  uint8 extraPatterns = 0;
  bool isHexagonal = false;
  std::optional<float64> xStep;
  std::optional<float64> yStep;
  uint64 fileSize = 0;
};

class ORIENTATIONANALYSIS_EXPORT IEbsdPatternFileReader
{
public:
  virtual ~IEbsdPatternFileReader() noexcept = default;

  IEbsdPatternFileReader(const IEbsdPatternFileReader&) = delete;
  IEbsdPatternFileReader(IEbsdPatternFileReader&&) noexcept = delete;
  IEbsdPatternFileReader& operator=(const IEbsdPatternFileReader&) = delete;
  IEbsdPatternFileReader& operator=(IEbsdPatternFileReader&&) noexcept = delete;

  virtual Result<EbsdPatternFileInfo> readFileInfo() const = 0;
  virtual Result<> readPatternData(const EbsdPatternFileInfo& fileInfo, IDataArray& outputArray, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler) const = 0;

protected:
  IEbsdPatternFileReader() = default;
};
} // namespace nx::core
