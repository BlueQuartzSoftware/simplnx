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
 * @file MetaImageUtilities.hpp
 * @brief Header parsing + streaming voxel reader for the MHA / MetaImage format
 *        (attached `.mha` and detached `.mhd` + `.raw`/`.zraw`), shared by
 *        ReadMhaFileFilter.
 *
 * A MetaImage file is a plain-text ASCII header of `Key = Value` lines. The
 * header is TERMINATED by the `ElementDataFile` tag; for an attached file
 * (`ElementDataFile = LOCAL`) the binary data segment begins immediately after
 * that line's newline, for a detached file it is a sibling `.raw`/`.zraw`. Only
 * the DATA segment may be compressed (`CompressedData = True`), and MetaImage
 * uses raw ZLIB format (RFC 1950, `0x78..` header) -- NOT gzip. The same
 * inflateInit2(&zs, 15 + 32) auto-detect init used for NRRD gzip transparently
 * decodes MetaImage zlib as well, so MetaImageDataReader mirrors NrrdDataReader.
 *
 * These helpers are independent of the filter framework (and of Eigen/ITK) so
 * they can be reused from unit tests or a future writer. The 4x4 transform
 * matrix is assembled by the filter from the raw `transformMatrix` values here.
 */

namespace nx::core::mhd
{
/** @brief zlib compressed-input buffer size (4 MiB); mirrors nrrd::k_GzReadBufferSize. */
inline constexpr unsigned int k_ZlibReadBufferSize = 4194304;

/** @brief Data-segment encoding. Only raw + zlib-compressed are supported. */
enum class Encoding
{
  Raw,
  Compressed
};

/**
 * @struct MetaImageMetadata
 * @brief Canonicalized view of a MetaImage header, ready to drive
 *        CreateImageGeometryAction / CreateArrayAction, MetaImageDataReader, and
 *        (in the filter) the 4x4 transform assembly.
 */
struct IMAGEPROCESSING_EXPORT MetaImageMetadata
{
  std::string filePath; ///< Path the header was read from (diagnostics).
  usize nDims{0};       ///< NDims header value (2 or 3).

  // Canonicalized geometry (drives the ImageGeom + DataArray).
  std::array<usize, 3> dimensions{1, 1, 1};         ///< X, Y, Z voxel counts (Z=1 for 2D).
  std::array<float32, 3> spacing{1.0f, 1.0f, 1.0f}; ///< Per-axis spacing (ElementSpacing / ElementSize).
  std::array<float32, 3> origin{0.0f, 0.0f, 0.0f};  ///< World-space origin (Offset / Position / Origin).

  // Datatype.
  DataType dataType{DataType::uint8}; ///< simplnx element type (from ElementType).
  usize componentCount{1};            ///< ElementNumberOfChannels (interleaved components).
  usize elementSize{1};               ///< Bytes per component (for byteswap).

  // Data placement + decode.
  Encoding encoding{Encoding::Raw};   ///< raw or zlib-compressed.
  bool byteSwapRequired{false};       ///< File endian differs from host (multi-byte only).
  usize dataStartOffset{0};           ///< Byte offset of data within dataFilePath.
  std::filesystem::path dataFilePath; ///< Header path (attached) or sibling (detached).
  bool detached{false};               ///< True for .mhd + external data file.

  // Transform (orthogonal to the axis-aligned base geometry; consumed by the
  // filter's transform feature). Row-major, exactly nDims*nDims values as they
  // appear in the header; empty => identity.
  std::vector<float64> transformMatrix;
  std::array<float32, 3> centerOfRotation{0.0f, 0.0f, 0.0f};

  // Diagnostics.
  std::string typeString; ///< Original ElementType value (e.g. "MET_UCHAR").
};

/**
 * @brief Maps a MetaImage `ElementType` string (already trimmed + upper-cased)
 *        to a simplnx DataType and the per-component byte size, using MetaIO's
 *        canonical sizes (note MET_LONG / MET_ULONG are 4 bytes).
 * @return {DataType, elementSizeBytes} or std::nullopt for unsupported types.
 */
std::optional<std::pair<DataType, usize>> IMAGEPROCESSING_EXPORT MetTypeToSimplnx(const std::string& metType);

/**
 * @brief Parses the ASCII header of a MetaImage file into MetaImageMetadata.
 *
 * Supports the common single-image case: ObjectType=Image, NDims (2/3), DimSize,
 * ElementType, ElementNumberOfChannels, ElementSpacing/ElementSize,
 * Offset/Position/Origin/ImagePosition, BinaryData,
 * BinaryDataByteOrderMSB/ElementByteOrderMSB, CompressedData,
 * TransformMatrix/Rotation/Orientation, CenterOfRotation, and ElementDataFile (LOCAL
 * attached, or a single detached filename). Multi-alias fields are resolved by MetaIO's
 * fixed precedence (e.g. ImagePosition > Origin > Offset > Position), independent of the
 * order they appear in the file. A nonzero HeaderSize is rejected (-35814) rather than
 * silently ignored, since this reader does not honor the pre-data seek MetaIO performs.
 * Errors use the -358xx block.
 */
Result<MetaImageMetadata> IMAGEPROCESSING_EXPORT ReadMetaImageHeader(const std::filesystem::path& filePath);

/**
 * @class MetaImageDataReader
 * @brief Sequential byte reader over a MetaImage data segment. Presents a
 *        uniform `readBytes` regardless of encoding: raw => std::ifstream;
 *        compressed => zlib inflate (windowBits 15+32 => auto zlib/gzip
 *        detection) fed from a 4 MiB compressed-input buffer. Portable.
 *
 * SHARED-LOGIC: the inflate loop is a verbatim copy of nrrd::NrrdDataReader
 * (only error codes + names differ). TODO(refactor): extract a shared
 * ZlibInflateStreamReader once a third format consumer appears.
 */
class IMAGEPROCESSING_EXPORT MetaImageDataReader
{
public:
  static Result<std::unique_ptr<MetaImageDataReader>> Create(const MetaImageMetadata& md);
  ~MetaImageDataReader();

  MetaImageDataReader(const MetaImageDataReader&) = delete;
  MetaImageDataReader(MetaImageDataReader&&) noexcept = delete;
  MetaImageDataReader& operator=(const MetaImageDataReader&) = delete;
  MetaImageDataReader& operator=(MetaImageDataReader&&) noexcept = delete;

  /** @brief Reads exactly @p nBytes into @p dst. Error -35831 on short read,
   *         -35832 on inflate error. */
  Result<> readBytes(void* dst, usize nBytes);

private:
  MetaImageDataReader() = default;

  std::ifstream m_Stream;
  Encoding m_Encoding{Encoding::Raw};
  std::string m_FilePath;
  z_stream m_Zs{};
  bool m_ZInit{false};
  bool m_SrcEof{false};
  std::vector<unsigned char> m_CompBuf;
};

} // namespace nx::core::mhd
