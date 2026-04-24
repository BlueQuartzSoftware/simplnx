#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/CropGeometryParameter.hpp"

#include <filesystem>

namespace nx::core
{

/**
 * @struct ReadNIfTIFileInputValues
 * @brief Plain-data bundle of inputs consumed by the `ReadNIfTIFile`
 *        algorithm.
 *
 * The filter's `executeImpl` populates this from its `Arguments` and
 * passes it through to the algorithm. Collecting the parameters in a
 * single struct keeps the algorithm constructor short and makes it
 * easy to invoke the algorithm directly from a test or another filter
 * without reconstructing the parameter plumbing.
 *
 * `ImageGeometryPath`, `CellAttributeMatrixName`, and
 * `ImageDataArrayName` must match whatever the filter's preflight
 * emitted — the algorithm relies on those DataPaths to locate the
 * pre-created `ImageGeom` and its `DataArray<T>`.
 */
struct SIMPLNXCORE_EXPORT ReadNIfTIFileInputValues
{
  /** @brief Path to the input `.nii` or `.nii.gz` file. */
  std::filesystem::path InputFilePath;

  /** @brief DataPath of the Image Geometry the filter's preflight
   *         created. The algorithm writes voxels into the
   *         `DataArray<T>` located at
   *         `ImageGeometryPath / CellAttributeMatrixName / ImageDataArrayName`. */
  DataPath ImageGeometryPath;

  /** @brief Name of the attribute matrix child of the Image Geometry
   *         that holds the voxel data array. */
  std::string CellAttributeMatrixName;

  /** @brief Name of the `DataArray<T>` the algorithm will fill with
   *         voxel values. */
  std::string ImageDataArrayName;

  /** @brief When `true`, the NIfTI header's `sform`/`qform` transform
   *         drives the ImageGeom origin and spacing. When `false`,
   *         the reader uses `pixdim` plus a zero origin even if a
   *         transform is recorded. */
  bool UseAffineIfPresent{true};

  /** @brief When `true` and the header declares a non-trivial scaling
   *         (`scl_slope != 0` and `(scl_slope != 1 or scl_inter != 0)`)
   *         on a single-component datatype, the voxels are promoted
   *         to `float32` and `y = slope * x + inter` is applied on
   *         read. Scaling is always skipped for RGB24/RGBA32 per the
   *         NIfTI-1 specification. */
  bool ApplyScalingTransform{true};

  /** @brief Optional voxel-index or physical-coordinate sub-volume
   *         to retain. When set to anything other than
   *         `CropValues::TypeEnum::NoCropping`, only the selected
   *         region is stored in the output DataArray — the rest of
   *         the file is read and discarded on the fly. */
  CropGeometryParameter::ValueType CroppingOptions;
};

/**
 * @class ReadNIfTIFile
 * @brief Streams voxel data out of a NIfTI-1 (`.nii` / `.nii.gz`) file
 *        into the DataArray that `ReadNIfTIFileFilter::preflightImpl`
 *        has already created.
 *
 * The algorithm does a single pass over the file: it re-reads the
 * 348-byte header (shared cheaply with preflight via zlib's
 * `gzopen`), computes the effective crop bounds, seeks to `vox_offset`,
 * and streams voxels one source scan-line at a time into a
 * destination-typed scratch buffer before bulk-copying the cropped
 * subrange into the DataStore via
 * `CopyFromArray::CopyData`. That pattern plays well with OOC stores
 * and avoids per-voxel virtual dispatch.
 *
 * Construction mirrors the project's standard algorithm pattern:
 * references to the DataStructure, the filter's message handler, the
 * shared cancel flag, and a non-owning pointer to the input bundle.
 * The algorithm holds no state of its own — invoke it by calling
 * `operator()`.
 *
 * ### Typical use (from a filter's `executeImpl`)
 * @code
 * ReadNIfTIFileInputValues inputValues;
 * // ... populate inputValues from filterArgs ...
 * return ReadNIfTIFile(dataStructure, messageHandler, shouldCancel, &inputValues)();
 * @endcode
 */
class SIMPLNXCORE_EXPORT ReadNIfTIFile
{
public:
  /**
   * @brief Constructs the algorithm. No work is done until `operator()`
   *        is called.
   *
   * @param dataStructure  The DataStructure the filter's preflight
   *                       populated with an empty ImageGeom and
   *                       DataArray. The algorithm locates and fills
   *                       that array.
   * @param mesgHandler    Sink for progress and informational
   *                       messages. Progress ticks roughly every
   *                       256k destination tuples.
   * @param shouldCancel   Atomic cancel flag polled at the top of
   *                       each source z-slice.
   * @param inputValues    Non-owning pointer to the parameter bundle.
   *                       Must outlive the call to `operator()`.
   */
  ReadNIfTIFile(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadNIfTIFileInputValues* inputValues);
  ~ReadNIfTIFile() noexcept;

  ReadNIfTIFile(const ReadNIfTIFile&) = delete;
  ReadNIfTIFile(ReadNIfTIFile&&) noexcept = delete;
  ReadNIfTIFile& operator=(const ReadNIfTIFile&) = delete;
  ReadNIfTIFile& operator=(ReadNIfTIFile&&) noexcept = delete;

  /**
   * @brief Runs the read. Opens the file, parses the header, resolves
   *        the crop bounds, and streams voxels into the pre-created
   *        DataArray.
   *
   * @return `Result<>` carrying an error from `ReadNiftiHeader`,
   *         `ComputeCropBounds`, the `gzseek` / `gzread` path, or a
   *         type-dispatch fall-through. An early cancel returns a
   *         valid empty `Result<>`.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ReadNIfTIFileInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
