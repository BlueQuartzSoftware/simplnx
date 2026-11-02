#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"

#include <zlib.h>

#include <array>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

/**
 * @file NrrdUtilities.hpp
 * @brief Header parsing + streaming voxel reader for the NRRD image format
 *        (attached `.nrrd` and detached `.nhdr`), used by ReadImageFilter's NRRD backend.
 *
 * A NRRD file is a plain-text ASCII header (magic `NRRD000x`, then `key: value`
 * lines) terminated by a single blank line, followed by the data segment. Only
 * the DATA segment may be gzip-compressed (`encoding: gzip`) — the header is
 * always plain text. This differs from NIfTI (whole-file gzip), so we parse the
 * header with std::ifstream, record the data byte offset, and stream the data
 * segment (raw or gzip) via NrrdDataReader.
 *
 * These helpers are independent of the filter framework so they can be reused
 * from unit tests or a future writer.
 */

namespace nx::core::nrrd
{
/** @brief zlib compressed-input buffer size (4 MiB); mirrors nifti::k_GzReadBufferSize. */
inline constexpr unsigned int k_GzReadBufferSize = 4194304;

/** @brief Data-segment encoding. Only raw + gzip are supported. */
enum class Encoding
{
  Raw,
  Gzip
};

/**
 * @struct NrrdMetadata
 * @brief Canonicalized view of a NRRD header, ready to drive
 *        CreateImageGeometryAction / CreateArrayAction and NrrdDataReader.
 */
struct IMAGEPROCESSING_EXPORT NrrdMetadata
{
  std::string filePath; ///< Path the header was read from (diagnostics).
  int version{0};       ///< NRRD version digit (4 for NRRD0004).

  // Canonicalized geometry (drives the ImageGeom + DataArray).
  std::array<usize, 3> dimensions{1, 1, 1};         ///< X, Y, Z voxel counts (Z=1 for 2D).
  std::array<float32, 3> spacing{1.0f, 1.0f, 1.0f}; ///< Per-axis spacing.
  std::array<float32, 3> origin{0.0f, 0.0f, 0.0f};  ///< World-space origin.
  usize spatialDim{3};                              ///< 2 or 3.

  // Datatype.
  DataType dataType{DataType::uint8}; ///< simplnx element type.
  usize componentCount{1};            ///< Components per tuple (RGB/vector => >1).
  usize elementSize{1};               ///< Bytes per component (for byteswap).

  // Data placement + decode.
  Encoding encoding{Encoding::Raw};   ///< raw or gzip.
  bool byteSwapRequired{false};       ///< File endian differs from host (multi-byte only).
  usize dataStartOffset{0};           ///< Byte offset of data within dataFilePath.
  std::filesystem::path dataFilePath; ///< Header path (attached) or sibling (detached).
  bool detached{false};               ///< True for .nhdr + external data file.

  // Orientation.
  bool affineHasRotation{false}; ///< True if space directions are non-axis-aligned.

  // Diagnostics.
  std::string typeString;      ///< Original `type` field value.
  std::vector<usize> rawSizes; ///< Original `sizes` field, in file axis order.
};

/**
 * @brief Maps a NRRD `type` string (already lower-cased) to a simplnx DataType
 *        and the per-component byte size. Handles the NRRD synonym set.
 * @return {DataType, elementSizeBytes} or std::nullopt for block/unknown.
 */
std::optional<std::pair<DataType, usize>> IMAGEPROCESSING_EXPORT NrrdTypeToSimplnx(const std::string& nrrdType);

/**
 * @brief Parses the ASCII header of a NRRD file into NrrdMetadata.
 *
 * Supports the common single-volume case: type/dimension/sizes/encoding
 * (raw|gzip)/endian/kinds/space directions/space origin/spacings/space
 * dimension/data file. A leading `none`/vector/color axis is treated as the
 * interleaved component axis (must be axis 0). Non-axis-aligned space
 * directions set `affineHasRotation` (rotation dropped by the caller with a
 * warning); spacing = per-column L2 norm. Errors use the -357xx block.
 */
Result<NrrdMetadata> IMAGEPROCESSING_EXPORT ReadNrrdHeader(const std::filesystem::path& filePath);

/**
 * @class NrrdDataReader
 * @brief Sequential byte reader over a NRRD data segment. Presents a uniform
 *        `readBytes` regardless of encoding: raw => std::ifstream; gzip =>
 *        zlib inflate (windowBits 15+32 => auto gzip/zlib detection) fed from a
 *        4 MiB compressed-input buffer. Portable (no file-descriptor tricks).
 */
class IMAGEPROCESSING_EXPORT NrrdDataReader
{
public:
  static Result<std::unique_ptr<NrrdDataReader>> Create(const NrrdMetadata& md);
  ~NrrdDataReader();

  NrrdDataReader(const NrrdDataReader&) = delete;
  NrrdDataReader(NrrdDataReader&&) noexcept = delete;
  NrrdDataReader& operator=(const NrrdDataReader&) = delete;
  NrrdDataReader& operator=(NrrdDataReader&&) noexcept = delete;

  /** @brief Reads exactly @p nBytes into @p dst. Error -35731 on short read,
   *         -35732 on inflate error. */
  Result<> readBytes(void* dst, usize nBytes);

private:
  NrrdDataReader() = default;

  std::ifstream m_Stream;
  Encoding m_Encoding{Encoding::Raw};
  std::string m_FilePath;
  z_stream m_Zs{};
  bool m_ZInit{false};
  bool m_SrcEof{false};
  std::vector<unsigned char> m_CompBuf;
};

} // namespace nx::core::nrrd
