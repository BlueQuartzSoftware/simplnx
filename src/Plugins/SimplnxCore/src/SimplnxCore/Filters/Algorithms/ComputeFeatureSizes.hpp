#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

namespace nx::core
{

/**
 * @struct ComputeFeatureSizesInputValues
 * @brief Holds all user-configured parameters for the ComputeFeatureSizes algorithm.
 */
struct SIMPLNXCORE_EXPORT ComputeFeatureSizesInputValues
{
  DataObjectNameParameter::ValueType EquivalentDiametersName;              ///< Output: equivalent spherical/circular diameter array name.
  AttributeMatrixSelectionParameter::ValueType FeatureAttributeMatrixPath; ///< Feature-level Attribute Matrix.
  ArraySelectionParameter::ValueType FeatureIdsPath;                       ///< Per-cell Feature ID array.
  GeometrySelectionParameter::ValueType InputImageGeometryPath;            ///< Input ImageGeom or RectGridGeom.
  DataObjectNameParameter::ValueType NumElementsName;                      ///< Output: per-feature voxel count array name.
  BoolParameter::ValueType SaveElementSizes;                               ///< If true, persist per-element sizes in the Geometry.
  DataObjectNameParameter::ValueType VolumesName;                          ///< Output: per-feature volume/area array name.
};

/**
 * @class ComputeFeatureSizes
 * @brief Computes the volume (or area in 2D), equivalent diameter, and voxel count
 * for each feature in an Image Geometry or Rectilinear Grid Geometry.
 *
 * @section ooc_optimization Out-of-Core Optimization
 * The original implementation iterated over all voxels using per-element getValue()
 * calls on the FeatureIds and element-sizes DataStores. For OOC data this caused
 * chunk thrashing on every voxel access.
 *
 * The optimized implementation reads FeatureIds (and element sizes for RectGrid)
 * in fixed-size chunks (64K tuples) via copyIntoBuffer(), processing each chunk
 * from a local buffer. Accumulation uses plain std::vectors, and Kahan summation
 * for RectGrid volumes is performed on the local buffer data rather than through
 * virtual DataStore dispatch.
 */
class SIMPLNXCORE_EXPORT ComputeFeatureSizes
{
public:
  ComputeFeatureSizes(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeFeatureSizesInputValues* inputValues);
  ~ComputeFeatureSizes() noexcept;

  ComputeFeatureSizes(const ComputeFeatureSizes&) = delete;
  ComputeFeatureSizes(ComputeFeatureSizes&&) noexcept = delete;
  ComputeFeatureSizes& operator=(const ComputeFeatureSizes&) = delete;
  ComputeFeatureSizes& operator=(ComputeFeatureSizes&&) noexcept = delete;

  /**
   * @brief Executes the feature size computation using chunked bulk I/O.
   * @return Result<> indicating success or error.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;                                ///< Reference to the DataStructure.
  const ComputeFeatureSizesInputValues* m_InputValues = nullptr; ///< User-configured parameters.
  const std::atomic_bool& m_ShouldCancel;                        ///< Cancellation flag.
  const IFilter::MessageHandler& m_MessageHandler;               ///< Message handler for progress.
};

} // namespace nx::core
