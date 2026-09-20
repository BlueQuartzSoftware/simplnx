#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"

namespace nx::core
{
/**
 * @namespace nx::core
 * @brief Contains simplnx core types and functions.
 */

/**
 * @struct ComputeFeatureBoundsInputValues
 * @brief Stores filter values for feature-bound execution.
 *
 * Split output uses MinArrayPath and MaxArrayPath. Unified output uses UnifiedArrayPath. Edge
 * geometry settings apply only when CreateEdgeGeometry is true.
 */
struct SIMPLNXCORE_EXPORT ComputeFeatureBoundsInputValues
{
  ChoicesParameter::ValueType OutputType;
  DataPath GeometryPath;
  DataPath FeatureAMPath;
  DataPath FeatureIdsArrayPath;
  DataPath MinArrayPath;
  DataPath MaxArrayPath;
  DataPath UnifiedArrayPath;

  bool CreateEdgeGeometry;
  DataGroupCreationParameter::ValueType EdgeGeometryDataPath;
  DataObjectNameParameter::ValueType EdgeAttributeMatrixName;
  DataObjectNameParameter::ValueType FeatureIdsArrayName;
};

/**
 * @class ComputeFeatureBounds
 * @brief Dispatches feature-bound computation to a direct in-memory implementation
 * or a bounded-memory bulk-I/O implementation for out-of-core inputs.
 *
 * The dispatcher examines Feature IDs to select the path. Scanline execution reads Feature IDs in
 * fixed chunks and retains six float32 values per feature. Direct execution can use per-element
 * geometry access. Neither path establishes generic DataArray or DataStore thread safety.
 */
class SIMPLNXCORE_EXPORT ComputeFeatureBounds
{
public:
  /**
   * @brief Initializes the feature-bound dispatcher.
   * @param dataStructure Contains geometry, Feature IDs, and outputs.
   * @param mesgHandler Supplies filter messages.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Selects output layout and required objects.
   * @pre inputValues is not null.
   * @pre All arguments outlive this executor.
   */
  ComputeFeatureBounds(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeFeatureBoundsInputValues* inputValues);
  /**
   * @brief Destroys the feature-bound dispatcher.
   */
  ~ComputeFeatureBounds() noexcept;

  ComputeFeatureBounds(const ComputeFeatureBounds&) = delete;
  ComputeFeatureBounds(ComputeFeatureBounds&&) noexcept = delete;
  ComputeFeatureBounds& operator=(const ComputeFeatureBounds&) = delete;
  ComputeFeatureBounds& operator=(ComputeFeatureBounds&&) noexcept = delete;

  /**
   * @enum OutputDataType
   * @brief Identifies feature-bound output layout.
   */
  enum OutputDataType : uint8
  {
    Split = 0,  ///< Writes separate three-component minimum and maximum arrays.
    Unified = 1 ///< Writes one six-component array.
  };

  /**
   * @brief Computes requested feature bounds and optional edge geometry.
   * @return Success, or a Feature ID storage, geometry, or feature-sizing error.
   *
   * The direct path checks cancellation only before computation starts. Scanline execution checks
   * cancellation before input batches and output phases. The scanline path does not check
   * cancellation during bounds or edge-geometry writes.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeFeatureBoundsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
