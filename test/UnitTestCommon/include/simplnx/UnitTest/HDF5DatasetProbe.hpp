#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

namespace nx::core::UnitTest
{
/**
 * @brief Storage layout of an HDF5 dataset, mirroring the relevant subset of H5D_layout_t.
 *
 * Used by ProbeHdf5Dataset so callers do not need to include <hdf5.h> just to
 * inspect dataset layout in tests.
 */
enum class DatasetLayout : std::int8_t
{
  Compact = 0,
  Contiguous = 1,
  Chunked = 2,
  Virtual = 3
};

/**
 * @brief Result of probing an HDF5 dataset's storage layout and compression filters.
 */
struct DatasetProbeInfo
{
  DatasetLayout layout = DatasetLayout::Contiguous;
  bool hasDeflate = false;
  std::int32_t deflateLevel = -1;
};

/**
 * @brief Opens an HDF5 file read-only, inspects the named dataset's creation
 *        property list, and returns its storage layout and gzip/deflate filter
 *        configuration.
 *
 * Returns std::nullopt on any failure (file open, dataset open, property-list
 * query, or unrecognized layout).
 *
 * @param filePath    Path to the .h5 / .dream3d file to open.
 * @param datasetPath Absolute HDF5 path to the target dataset (e.g. "/g/array").
 * @return DatasetProbeInfo populated with layout and filter info, or std::nullopt on failure.
 */
std::optional<DatasetProbeInfo> ProbeHdf5Dataset(const std::filesystem::path& filePath, const std::string& datasetPath);

} // namespace nx::core::UnitTest
