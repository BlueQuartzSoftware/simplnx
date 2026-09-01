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
 * @namespace nx::core
 * @brief Contains simplnx core types and functions.
 */

/**
 * @struct ComputeFeatureSizesInputValues
 * @brief Stores filter values for feature-size execution.
 */
struct SIMPLNXCORE_EXPORT ComputeFeatureSizesInputValues
{
  DataObjectNameParameter::ValueType EquivalentDiametersName;
  AttributeMatrixSelectionParameter::ValueType FeatureAttributeMatrixPath;
  ArraySelectionParameter::ValueType FeatureIdsPath;
  GeometrySelectionParameter::ValueType InputImageGeometryPath;
  DataObjectNameParameter::ValueType NumElementsName;
  BoolParameter::ValueType SaveElementSizes; ///< True to retain generated geometry element sizes.
  DataObjectNameParameter::ValueType VolumesName;
};

/**
 * @class ComputeFeatureSizes
 * @brief Dispatches ImageGeom and RectGrid feature-size calculation.
 *
 * Each feature receives a voxel count, an area or volume, and an equivalent diameter.
 *
 * Dispatch uses FeatureIdsPath only. Direct execution uses parallel per-element access. Scanline
 * execution uses sequential bulk transfers. Output and RectGrid element-size storage do not select
 * the path.
 *
 * The direct path uses requireStoresInMemory() only to disable parallel scheduling for a nonresident
 * Feature ID store. It does not pin, lock, or make generic DataArray or DataStore access safe.
 *
 * @see ComputeFeatureSizesDirect.
 * @see ComputeFeatureSizesScanline.
 */
class SIMPLNXCORE_EXPORT ComputeFeatureSizes
{
public:
  /**
   * @brief Initializes the feature-size dispatcher.
   * @param dataStructure Contains geometry, Feature IDs, and outputs.
   * @param mesgHandler Supplies filter messages.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Selects outputs and required objects.
   * @pre inputValues is not null.
   * @pre All arguments outlive this executor.
   */
  ComputeFeatureSizes(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeFeatureSizesInputValues* inputValues);
  /**
   * @brief Destroys the feature-size dispatcher.
   */
  ~ComputeFeatureSizes() noexcept;

  ComputeFeatureSizes(const ComputeFeatureSizes&) = delete;
  ComputeFeatureSizes(ComputeFeatureSizes&&) noexcept = delete;
  ComputeFeatureSizes& operator=(const ComputeFeatureSizes&) = delete;
  ComputeFeatureSizes& operator=(ComputeFeatureSizes&&) noexcept = delete;

  /**
   * @brief Computes requested feature sizes.
   * @return Success, or an implementation error.
   *
   * Direct and scanline paths check cancellation at different phase boundaries. Both can return
   * success with partially written feature outputs.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeFeatureSizesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
