#include "HDF5DatasetProbe.hpp"

#include <hdf5.h>

#include <array>
#include <optional>

namespace nx::core::UnitTest
{
namespace
{
std::optional<DatasetLayout> TranslateH5Layout(H5D_layout_t layout) noexcept
{
  switch(layout)
  {
  case H5D_COMPACT:
    return DatasetLayout::Compact;
  case H5D_CONTIGUOUS:
    return DatasetLayout::Contiguous;
  case H5D_CHUNKED:
    return DatasetLayout::Chunked;
  case H5D_VIRTUAL:
    return DatasetLayout::Virtual;
  default:
    return std::nullopt;
  }
}
} // namespace

std::optional<DatasetProbeInfo> ProbeHdf5Dataset(const std::filesystem::path& filePath, const std::string& datasetPath)
{
  const hid_t fileId = H5Fopen(filePath.string().c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
  if(fileId < 0)
  {
    return std::nullopt;
  }

  const hid_t datasetId = H5Dopen2(fileId, datasetPath.c_str(), H5P_DEFAULT);
  if(datasetId < 0)
  {
    H5Fclose(fileId);
    return std::nullopt;
  }

  const hid_t dcplId = H5Dget_create_plist(datasetId);
  if(dcplId < 0)
  {
    H5Dclose(datasetId);
    H5Fclose(fileId);
    return std::nullopt;
  }

  auto layoutOpt = TranslateH5Layout(H5Pget_layout(dcplId));
  if(!layoutOpt.has_value())
  {
    H5Pclose(dcplId);
    H5Dclose(datasetId);
    H5Fclose(fileId);
    return std::nullopt;
  }

  DatasetProbeInfo info;
  info.layout = *layoutOpt;

  const int nFilters = H5Pget_nfilters(dcplId);
  for(int i = 0; i < nFilters; ++i)
  {
    unsigned int flags = 0;
    // DEFLATE uses a single client-data value, but overestimating the buffer is
    // harmless and avoids a subtle API misuse if HDF5 ever extends the filter.
    std::size_t cdCapacity = 8;
    std::array<unsigned int, 8> cdValues{};
    std::array<char, 32> filterName{};
    unsigned int filterConfig = 0;
    const H5Z_filter_t filterId = H5Pget_filter2(dcplId, static_cast<unsigned int>(i), &flags, &cdCapacity, cdValues.data(), filterName.size(), filterName.data(), &filterConfig);
    if(filterId == H5Z_FILTER_DEFLATE)
    {
      info.hasDeflate = true;
      info.deflateLevel = static_cast<std::int32_t>(cdValues[0]);
      break;
    }
  }

  H5Pclose(dcplId);
  H5Dclose(datasetId);
  H5Fclose(fileId);
  return info;
}

} // namespace nx::core::UnitTest
