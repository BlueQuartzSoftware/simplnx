#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"
#include "SimplnxCore/utils/nifti1.h"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"

#include <array>
#include <filesystem>
#include <optional>
#include <string>

/**
 * @file NiftiUtilities.hpp
 * @brief Header-parsing helpers for the NIfTI-1 single-file format (`.nii`
 *        and gzipped `.nii.gz`), shared by `ReadNIfTIFileFilter` and
 *        future NIfTI-aware code.
 *
 * The NIfTI-1 specification is defined by the bundled `nifti1.h` header
 * (Robert W. Cox, NIMH). This layer reads the 348-byte header, validates
 * it, detects endianness, and canonicalizes the fields simplnx actually
 * cares about (dimensions, spacing, origin, data type, scaling) into a
 * single `NiftiMetadata` struct. Voxel data is **not** read here — that
 * is the algorithm's responsibility, which uses `voxOffset` and
 * `byteSwapRequired` from the metadata to do its own stream reads.
 *
 * The helpers here are deliberately independent of the simplnx filter
 * framework, so they can be reused from unit tests, a future writer,
 * or a command-line tool.
 */

namespace nx::core::nifti
{
/**
 * @brief Size of the NIfTI-1 header in bytes. Always 348 for a valid
 *        NIfTI-1 file regardless of byte order.
 */
inline constexpr unsigned int k_HeaderSize = 348;

/**
 * @brief Minimum legal value of the `vox_offset` field in a single-file
 *        NIfTI-1 (`n+1`). The spec reserves bytes 348..351 for the
 *        extension block; voxel data therefore cannot start before
 *        byte 352.
 */
inline constexpr usize k_MinVoxOffset = 352;

/**
 * @brief Size (in bytes) of zlib's internal input/output buffer, set via
 *        `gzbuffer()` immediately after every `gzopen()`.
 *
 * zlib defaults to an 8 KB internal buffer, which means it refills from
 * the underlying file in 8 KB chunks of compressed data — one `read()`
 * syscall each. On a local disk that is negligible, but on network
 * attached storage (NAS) every refill is a separate network round-trip,
 * so a multi-GB `.nii.gz` triggers hundreds of thousands of small,
 * latency-bound reads. Raising the buffer to 4 MiB cuts the number of
 * round-trips by ~512x while costing only a few × this size in working
 * memory (negligible next to the voxel volume). Larger values yield
 * diminishing returns once the buffer exceeds the link's
 * bandwidth-delay product and the client's own read-ahead.
 */
inline constexpr unsigned int k_GzReadBufferSize = 4194304;

/**
 * @brief Magic bytes that identify the single-file `.nii` / `.nii.gz`
 *        format. Trailing null is included per the spec.
 */
inline constexpr std::array<char, 4> k_SingleFileMagic = {'n', '+', '1', '\0'};

/**
 * @brief Magic bytes that identify the separate-file `.hdr` / `.img`
 *        pair format. Present in the spec but **not** supported by
 *        this reader (see `ReadNiftiHeader` error `-34703`).
 */
inline constexpr std::array<char, 4> k_PairFileMagic = {'n', 'i', '1', '\0'};

/**
 * @struct NiftiTypeInfo
 * @brief Canonicalized mapping between a NIfTI datatype code (as stored
 *        in the header's `datatype` field) and the corresponding
 *        simplnx `DataType` plus per-voxel component count.
 *
 * The NIfTI specification uses a single `datatype` code to cover both
 * scalar types (e.g. `NIFTI_TYPE_UINT8 = 2`) and packed multi-component
 * types (e.g. `NIFTI_TYPE_RGB24 = 128`, which is three packed `uint8`
 * components). simplnx arrays separate type from cardinality, so the
 * RGB/RGBA codes expand to `{ uint8, 3 }` / `{ uint8, 4 }` here.
 */
struct SIMPLNXCORE_EXPORT NiftiTypeInfo
{
  /** @brief simplnx element type (`uint8`, `float32`, ...). */
  DataType dataType{DataType::uint8};

  /** @brief Components per voxel. 1 for scalar types; 3 for RGB24; 4 for RGBA32. */
  usize componentCount{1};

  /** @brief Total bits per voxel as declared by the NIfTI spec
   *         (8 for `uint8`, 24 for `RGB24`, 32 for `RGBA32`, etc.).
   *         Useful for consistency-checking against the header's
   *         `bitpix` field. */
  int16 bitpix{0};
};

/**
 * @brief Maps a NIfTI datatype code (`NIFTI_TYPE_*` as defined by
 *        `nifti1.h`) to a `NiftiTypeInfo`.
 *
 * Returns `std::nullopt` for codes that simplnx does not currently
 * handle: `NIFTI_TYPE_COMPLEX64`, `NIFTI_TYPE_COMPLEX128`,
 * `NIFTI_TYPE_COMPLEX256`, `NIFTI_TYPE_FLOAT128`, `NIFTI_TYPE_BINARY`,
 * and any unrecognized value. The filter's preflight converts that
 * `nullopt` into user-visible error `-34707`.
 *
 * @param niftiDatatype The raw `datatype` field value from the header.
 * @return Canonicalized type info, or `std::nullopt` if unsupported.
 */
std::optional<NiftiTypeInfo> SIMPLNXCORE_EXPORT NiftiDatatypeToSimplnx(int16 niftiDatatype);

/**
 * @struct NiftiMetadata
 * @brief Canonicalized view of a NIfTI-1 header, ready to drive
 *        `CreateImageGeometryAction` / `CreateArrayAction` and the
 *        streaming voxel reader.
 *
 * Fields are grouped by topic:
 *
 * 1. **Raw header**. Kept so the filter can surface useful diagnostics
 *    in `preflightUpdatedValues` (datatype code, dim vector, pixdim,
 *    etc.) even for fields the reader itself does not consume.
 * 2. **Canonicalized geometry**. `dimensions`, `spacing`, and `origin`
 *    are what the filter should pass to `CreateImageGeometryAction`.
 *    The values are derived from the best-available source in the
 *    header: `sform` if `sformCode > 0`, else `qform` if
 *    `qformCode > 0`, else the `pixdim` / zero-origin fallback.
 * 3. **Datatype**. `dataType` + `componentCount` tell the algorithm
 *    which simplnx `DataArray<T>` to create and how many components
 *    per tuple. `niftiDatatype` and `bitpix` are retained for
 *    diagnostics.
 * 4. **Data placement + byte order**. `voxOffset` is the byte offset
 *    into the file where voxel data starts (after the header plus
 *    any extension block). `byteSwapRequired` is true when the file
 *    was written in a byte order different from the host, and the
 *    algorithm must swap each multi-byte voxel as it is read.
 * 5. **Optional data scaling**. `sclSlope` / `sclInter` carry the
 *    header values verbatim. `hasNontrivialScaling` is a convenience
 *    flag the filter uses to decide whether to promote the output
 *    array to `float32` and apply `y = slope * x + inter`. It is
 *    always `false` for RGB24 / RGBA32 per the NIfTI spec.
 * 6. **Orientation**. `qformCode` / `sformCode` record which transform
 *    was present in the file; `sformMatrix` stores the 3x4 affine
 *    when it exists. `affineHasRotation` is `true` when the selected
 *    transform has a non-trivial rotation component — callers should
 *    emit a warning because simplnx `ImageGeom` is axis-aligned.
 * 7. **Other**. `intentCode`, `intentName`, and `descrip` are the
 *    free-form metadata strings from the file. `filePath` is the
 *    absolute or relative path the header was read from, preserved
 *    so downstream error messages can reference it.
 */
struct SIMPLNXCORE_EXPORT NiftiMetadata
{
  /** @name Raw header (diagnostics only) */
  /** @{ */
  std::string magic;               ///< Original `magic` field value ("n+1" for single-file).
  std::array<int16, 8> dim{};      ///< Full 8-element dim vector as stored in the header.
  std::array<float32, 8> pixdim{}; ///< Full 8-element pixdim vector.
  /** @} */

  /** @name Canonicalized geometry (drives the ImageGeom + DataArray) */
  /** @{ */
  std::array<usize, 3> dimensions{0, 0, 0};         ///< Voxel count per axis (x, y, z).
  std::array<float32, 3> spacing{1.0f, 1.0f, 1.0f}; ///< Voxel spacing per axis.
  std::array<float32, 3> origin{0.0f, 0.0f, 0.0f};  ///< World-space origin of voxel (0, 0, 0).
  /** @} */

  /** @name Datatype */
  /** @{ */
  int16 niftiDatatype{0};             ///< Raw `datatype` code from the header (`NIFTI_TYPE_*`).
  int16 bitpix{0};                    ///< Bits per voxel, cross-validated against the datatype.
  DataType dataType{DataType::uint8}; ///< simplnx element type for the DataArray.
  usize componentCount{1};            ///< Components per tuple (1 for scalars, 3 for RGB24, 4 for RGBA32).
  /** @} */

  /** @name Data placement + byte order */
  /** @{ */
  usize voxOffset{k_MinVoxOffset}; ///< Byte offset where voxel data starts.
  bool byteSwapRequired{false};    ///< True if the file is in the opposite byte order from the host.
  /** @} */

  /** @name Optional data scaling (`y = slope * x + inter`) */
  /** @{ */
  float32 sclSlope{0.0f};           ///< `scl_slope` from the header; 0 means "no scaling" per spec.
  float32 sclInter{0.0f};           ///< `scl_inter` from the header.
  bool hasNontrivialScaling{false}; ///< True when (slope != 0) and (slope != 1 or inter != 0); always false for RGB24/RGBA32.
  /** @} */

  /** @name Orientation */
  /** @{ */
  int16 qformCode{0};                                  ///< NIFTI_XFORM_* code for the qform; 0 = not set.
  int16 sformCode{0};                                  ///< NIFTI_XFORM_* code for the sform; 0 = not set.
  std::array<std::array<float32, 4>, 3> sformMatrix{}; ///< 3x4 affine `[srow_x; srow_y; srow_z]` when sformCode > 0.
  bool affineHasRotation{false};                       ///< True if the selected transform has non-trivial rotation.
  /** @} */

  /** @name Other */
  /** @{ */
  int16 intentCode{0};    ///< NIFTI_INTENT_* code, if any.
  std::string intentName; ///< Free-form intent name from the header.
  std::string descrip;    ///< 80-character description field from the header.
  std::string filePath;   ///< Path the header was read from; kept for diagnostics.
  /** @} */
};

/**
 * @brief Opens a NIfTI-1 file (either uncompressed `.nii` or gzipped
 *        `.nii.gz`) and parses its 348-byte header into a `NiftiMetadata`.
 *
 * This function is the single entry point for header parsing. It is
 * safe to call repeatedly — it opens and closes the file each time and
 * holds no state. The filter caches the returned metadata across
 * preflight and execute calls to avoid redundant I/O.
 *
 * ### What it does
 * 1. Opens the file through zlib's `gzopen`, which transparently
 *    handles both compressed and uncompressed input.
 * 2. Reads exactly 348 bytes into a `nifti_1_header`.
 * 3. Detects endianness by inspecting `sizeof_hdr`. If swapping makes
 *    it equal 348, every multi-byte field is byte-swapped in place
 *    and `byteSwapRequired` is set so the voxel reader knows to do
 *    the same.
 * 4. Validates: magic bytes must be `"n+1"`, rank must be 3, each
 *    spatial dimension must be > 0, the datatype code must be one
 *    `NiftiDatatypeToSimplnx` recognizes, and `bitpix` must agree
 *    with the datatype if declared.
 * 5. Canonicalizes geometry using the best-available orientation:
 *    `sform` (preferred) → `qform` → pixdim + zero origin. Spacing
 *    for the sform path comes from the column magnitudes of the
 *    affine, which correctly handles files where the affine carries
 *    anisotropic scaling.
 * 6. Records whether the selected affine has a non-trivial rotation
 *    so the caller can warn the user that simplnx's axis-aligned
 *    `ImageGeom` cannot represent it.
 *
 * ### Errors
 * Returned as `Result<NiftiMetadata>` invalid values. Error codes are
 * in the `-347xx` range:
 *
 * | Code    | Meaning                                                        |
 * |---------|----------------------------------------------------------------|
 * | `-34700` | Could not open file                                           |
 * | `-34701` | Short read of the 348-byte header (file truncated)            |
 * | `-34702` | `sizeof_hdr` is neither 348 nor its byte-swap                  |
 * | `-34703` | `.hdr`/`.img` pair format (`magic == "ni1"`), unsupported      |
 * | `-34704` | Unrecognized magic bytes (not a NIfTI-1 file)                  |
 * | `-34705` | `dim[0]` is not 3 (non-3D volumes are not supported yet)       |
 * | `-34706` | One or more of `dim[1..3]` is ≤ 0                              |
 * | `-34707` | Unsupported datatype (complex, float128, etc.)                 |
 * | `-34708` | `bitpix` inconsistent with the declared datatype                |
 *
 * @param filePath            Path to the `.nii` or `.nii.gz` file to read.
 * @param useAffineIfPresent  When `true` (the default in the filter),
 *                            `sform` / `qform` are used to set the
 *                            origin and spacing when they are present
 *                            in the file. When `false`, the filter
 *                            falls back to `pixdim` + zero origin
 *                            even if a transform is recorded.
 * @return A populated `NiftiMetadata` on success; an invalid
 *         `Result<NiftiMetadata>` carrying one of the codes above on
 *         failure.
 */
Result<NiftiMetadata> SIMPLNXCORE_EXPORT ReadNiftiHeader(const std::filesystem::path& filePath, bool useAffineIfPresent);

} // namespace nx::core::nifti
