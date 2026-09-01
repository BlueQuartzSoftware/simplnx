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
 * @brief Collects crop settings, paths, and effective inclusive voxel bounds.
 */
struct SIMPLNXCORE_EXPORT CropImageGeometryInputValues
{
  GeometrySelectionParameter::ValueType InputImageGeometryPath;
  DataGroupCreationParameter::ValueType OutputImageGeometryPath;
  ArraySelectionParameter::ValueType FeatureIdsPath;
  VectorUInt64Parameter::ValueType MinVoxel;
  VectorUInt64Parameter::ValueType MaxVoxel;
  BoolParameter::ValueType RenumberFeatures;
  AttributeMatrixSelectionParameter::ValueType CellFeatureAttributeMatrixPath;
  BoolParameter::ValueType RemoveOriginalGeometry;
  BoolParameter::ValueType CropXDim;
  BoolParameter::ValueType CropYDim;
  BoolParameter::ValueType CropZDim;

  // Preflight computes these inclusive voxel indices.
  uint64 XMin;
  uint64 XMax;
  uint64 YMin;
  uint64 YMax;
  uint64 ZMin;
  uint64 ZMax;
};

/**
 * @class CropImageGeometry
 * @brief Copies an inclusive voxel region to a smaller ImageGeom.
 *
 * Each array task reads up to 32 complete source Z slices. It extracts cropped
 * rows in memory and writes one destination slab. This reduces HDF5 operations
 * at the cost of source-slice scratch for each concurrently processed array.
 */
class SIMPLNXCORE_EXPORT CropImageGeometry
{
public:
  /**
   * @brief Initializes image-geometry cropping.
   * @param dataStructure Contains source and destination geometry.
   * @param mesgHandler Receives per-array progress messages.
   * @param shouldCancel Signals cancellation between slab transfers.
   * @param inputValues Identifies paths, options, and effective bounds.
   * @pre All arguments and the inputValues object outlive this executor.
   */
  CropImageGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CropImageGeometryInputValues* inputValues);
  ~CropImageGeometry() noexcept;

  CropImageGeometry(const CropImageGeometry&) = delete;
  CropImageGeometry(CropImageGeometry&&) noexcept = delete;
  CropImageGeometry& operator=(const CropImageGeometry&) = delete;
  CropImageGeometry& operator=(CropImageGeometry&&) noexcept = delete;

  /**
   * @brief Copies cell arrays and optionally renumbers feature data.
   * @return Success, or a bounds, feature-validation, deep-copy, or renumbering error.
   *
   * Cell-array tasks can run concurrently across arrays. Their bulk-transfer
   * Result values are not inspected. A failed read can therefore lead to
   * invalid output without an error result.
   *
   * Cancellation returns success. Each destination array is filled before its
   * first slab copy, and completed slabs remain. Structural and feature changes
   * made before a later cancellation also remain.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const CropImageGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
