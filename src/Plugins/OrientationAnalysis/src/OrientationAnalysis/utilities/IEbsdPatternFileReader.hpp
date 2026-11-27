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
  std::string format_name;
  uint32 header_version = 0;
  DataType pixel_data_type = DataType::uint8;
  uint64 bytes_per_pixel = 0;
  uint64 pattern_width = 0;
  uint64 pattern_height = 0;
  uint64 data_offset = 0;
  uint64 pattern_stride = 0;
  uint64 number_of_patterns = 0;
  std::optional<uint64> number_of_rows;
  std::optional<uint64> number_of_columns;
  uint8 extra_patterns = 0;
  bool is_hexagonal = false;
  std::optional<float64> x_step;
  std::optional<float64> y_step;
  uint64 file_size = 0;
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
