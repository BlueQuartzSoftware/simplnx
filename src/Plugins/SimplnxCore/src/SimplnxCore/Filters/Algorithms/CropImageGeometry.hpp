#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

namespace nx::core
{

/**
 * @struct CropImageGeometryInputValues
 * @brief Holds all user-configured parameters for the CropImageGeometry algorithm.
 */
struct SIMPLNXCORE_EXPORT CropImageGeometryInputValues
{
  GeometrySelectionParameter::ValueType InputImageGeometryPath;                ///< Source ImageGeom to crop.
  DataGroupCreationParameter::ValueType OutputImageGeometryPath;               ///< Destination path for the cropped geometry.
  ArraySelectionParameter::ValueType FeatureIdsPath;                           ///< Per-cell Feature ID array (for renumbering).
  VectorUInt64Parameter::ValueType MinVoxel;                                   ///< User-specified minimum voxel bounds.
  VectorUInt64Parameter::ValueType MaxVoxel;                                   ///< User-specified maximum voxel bounds.
  BoolParameter::ValueType RenumberFeatures;                                   ///< If true, renumber Feature IDs to be contiguous.
  AttributeMatrixSelectionParameter::ValueType CellFeatureAttributeMatrixPath; ///< Feature-level AM (for renumbering).
  BoolParameter::ValueType RemoveOriginalGeometry;                             ///< If true, remove the source geometry after cropping.
  BoolParameter::ValueType CropXDim;                                           ///< Enable cropping in the X dimension.
  BoolParameter::ValueType CropYDim;                                           ///< Enable cropping in the Y dimension.
  BoolParameter::ValueType CropZDim;                                           ///< Enable cropping in the Z dimension.

  // Precomputed bounds from preflight
  uint64 XMin; ///< Effective minimum X voxel index (inclusive).
  uint64 XMax; ///< Effective maximum X voxel index (exclusive).
  uint64 YMin; ///< Effective minimum Y voxel index (inclusive).
  uint64 YMax; ///< Effective maximum Y voxel index (exclusive).
  uint64 ZMin; ///< Effective minimum Z voxel index (inclusive).
  uint64 ZMax; ///< Effective maximum Z voxel index (exclusive).
};

/**
 * @class CropImageGeometry
 * @brief Crops a region of interest from an ImageGeom by copying voxel data
 * from the source bounds into a new (smaller) ImageGeom.
 *
 * @section ooc_optimization Out-of-Core Optimization
 * The original implementation copied data element-by-element using getValue()/setValue()
 * in a triple-nested loop (Z, Y, X). For OOC data, each getValue() and setValue()
 * call triggered a chunk operation.
 *
 * The optimized implementation copies data one X-row at a time using bulk
 * copyIntoBuffer() and copyFromBuffer(). For each (Z, Y) pair, an entire row
 * of (XMax - XMin) tuples is read/written in a single operation. This reduces
 * the number of chunk operations from O(voxels * components) to O(Z * Y), a
 * factor-of-XDim improvement.
 */
class SIMPLNXCORE_EXPORT CropImageGeometry
{
public:
  CropImageGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CropImageGeometryInputValues* inputValues);
  ~CropImageGeometry() noexcept;

  CropImageGeometry(const CropImageGeometry&) = delete;
  CropImageGeometry(CropImageGeometry&&) noexcept = delete;
  CropImageGeometry& operator=(const CropImageGeometry&) = delete;
  CropImageGeometry& operator=(CropImageGeometry&&) noexcept = delete;

  /**
   * @brief Executes the crop operation, copying data row-by-row via bulk I/O.
   * @return Result<> indicating success or error.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;                              ///< Reference to the DataStructure.
  const CropImageGeometryInputValues* m_InputValues = nullptr; ///< User-configured parameters.
  const std::atomic_bool& m_ShouldCancel;                      ///< Cancellation flag.
  const IFilter::MessageHandler& m_MessageHandler;             ///< Message handler for progress.
};

} // namespace nx::core
