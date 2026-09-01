#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"

namespace nx::core
{

/**
 * @namespace nx::core
 * @brief Contains simplnx core types and functions.
 */

/**
 * @struct ComputeFeaturePhasesInputValues
 * @brief Stores filter values for feature-phase aggregation.
 */
struct SIMPLNXCORE_EXPORT ComputeFeaturePhasesInputValues
{
  AttributeMatrixSelectionParameter::ValueType CellFeaturesAttributeMatrixPath;
  ArraySelectionParameter::ValueType CellPhasesArrayPath;
  ArraySelectionParameter::ValueType FeatureIdsPath;
  DataObjectNameParameter::ValueType FeaturePhasesArrayName;
};

/**
 * @class ComputeFeaturePhases
 * @brief Assigns each feature the last phase observed in its cells.
 *
 * Matching Feature ID and cell-phase chunks use 65,536 values. Feature-sized
 * caches avoid cell-sized temporary memory and one output bulk write publishes
 * all phases. Input and output bulk-I/O errors propagate to the caller.
 */
class SIMPLNXCORE_EXPORT ComputeFeaturePhases
{
public:
  /**
   * @brief Initializes the feature-phase algorithm.
   * @param dataStructure Contains cell arrays and feature-phase output.
   * @param mesgHandler Supplies filter messages.
   * @param shouldCancel Signals cancellation between cell chunks.
   * @param inputValues Identifies required arrays and output name.
   * @pre inputValues is not null.
   * @pre All arguments outlive this executor.
   */
  ComputeFeaturePhases(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeFeaturePhasesInputValues* inputValues);
  /**
   * @brief Destroys the feature-phase algorithm.
   */
  ~ComputeFeaturePhases() noexcept;

  ComputeFeaturePhases(const ComputeFeaturePhases&) = delete;
  ComputeFeaturePhases(ComputeFeaturePhases&&) noexcept = delete;
  ComputeFeaturePhases& operator=(const ComputeFeaturePhases&) = delete;
  ComputeFeaturePhases& operator=(ComputeFeaturePhases&&) noexcept = delete;

  /**
   * @brief Aggregates cell phases into feature phases.
   * @return Success or warning, or a Feature ID validation or bulk-I/O error.
   *
   * When a chunk checkpoint observes cancellation, the method returns success
   * before the output bulk write. Conflicting phases produce a warning and the
   * last observed phase becomes the output value.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeFeaturePhasesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
