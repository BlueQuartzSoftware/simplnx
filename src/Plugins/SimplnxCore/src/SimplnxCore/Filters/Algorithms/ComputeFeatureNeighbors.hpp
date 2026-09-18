#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

namespace nx::core
{

/**
 * @namespace nx::core
 * @brief Contains simplnx core types and functions.
 */

/**
 * @struct ComputeFeatureNeighborsInputValues
 * @brief Stores filter values for feature-neighbor execution.
 *
 * BoundaryCellsPath applies when StoreBoundaryCells is true. SurfaceFeaturesPath applies when
 * StoreSurfaceFeatures is true.
 */
struct SIMPLNXCORE_EXPORT ComputeFeatureNeighborsInputValues
{
  DataPath BoundaryCellsPath;
  AttributeMatrixSelectionParameter::ValueType CellFeatureArrayPath;
  ArraySelectionParameter::ValueType FeatureIdsPath;
  GeometrySelectionParameter::ValueType InputImageGeometryPath;
  DataPath NeighborListPath;
  DataPath NumberOfNeighborsPath;
  DataPath SharedSurfaceAreaListPath;
  BoolParameter::ValueType StoreBoundaryCells;
  BoolParameter::ValueType StoreSurfaceFeatures;
  DataPath SurfaceFeaturesPath;
};

/**
 * @class ComputeFeatureNeighbors
 * @brief Dispatches ImageGeom feature-neighbor calculations.
 *
 * Dispatch uses FeatureIdsPath only. It selects direct execution for the normal in-memory path and
 * scanline execution for out-of-core Feature IDs. Output storage does not select the path.
 *
 * The direct path uses per-element store access. The scanline path uses three Feature ID slices and
 * bulk transfers. Both paths retain feature-neighbor maps in memory. This design does not establish
 * generic DataArray or DataStore thread safety.
 *
 * @see ComputeFeatureNeighborsDirect.
 * @see ComputeFeatureNeighborsScanline.
 */
class SIMPLNXCORE_EXPORT ComputeFeatureNeighbors
{
public:
  /**
   * @brief Initializes the feature-neighbor dispatcher.
   * @param dataStructure Contains the ImageGeom, Feature IDs, and outputs.
   * @param mesgHandler Supplies filter messages.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Selects outputs and identifies required objects.
   * @pre inputValues is not null.
   * @pre All arguments outlive this executor.
   */
  ComputeFeatureNeighbors(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeFeatureNeighborsInputValues* inputValues);
  /**
   * @brief Destroys the feature-neighbor dispatcher.
   */
  ~ComputeFeatureNeighbors() noexcept;

  ComputeFeatureNeighbors(const ComputeFeatureNeighbors&) = delete;
  ComputeFeatureNeighbors(ComputeFeatureNeighbors&&) noexcept = delete;
  ComputeFeatureNeighbors& operator=(const ComputeFeatureNeighbors&) = delete;
  ComputeFeatureNeighbors& operator=(ComputeFeatureNeighbors&&) noexcept = delete;

  /**
   * @brief Computes requested feature-neighbor output.
   * @return Success, or an implementation error.
   *
   * The direct path checks cancellation only during the 3D interior sweep. Scanline execution checks
   * cancellation between Z slices. Each path can return success with partial output.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeFeatureNeighborsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
