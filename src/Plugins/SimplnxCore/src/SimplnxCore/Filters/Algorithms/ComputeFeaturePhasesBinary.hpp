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
 * @struct ComputeFeaturePhasesBinaryInputValues
 * @brief Stores filter values for binary feature-phase assignment.
 */
struct SIMPLNXCORE_EXPORT ComputeFeaturePhasesBinaryInputValues
{
  AttributeMatrixSelectionParameter::ValueType CellFeatureAttributeMatrixPath;
  ArraySelectionParameter::ValueType FeatureIdsArrayPath;
  DataObjectNameParameter::ValueType FeaturePhasesArrayName;
  ArraySelectionParameter::ValueType MaskArrayPath;
};

/**
 * @class ComputeFeaturePhasesBinary
 * @brief Assigns binary Feature phases from a Cell or Feature mask.
 *
 * Cell Feature IDs and mask values use 1,000,000-tuple bulk buffers. Serial cell order preserves
 * last-cell-wins mask semantics. The feature cache grows only to the largest referenced Feature ID
 * and leaves unreferenced output Features at zero. A mask in the selected Feature Attribute Matrix
 * is converted directly in bounded chunks without reading Cell Feature IDs.
 */
class SIMPLNXCORE_EXPORT ComputeFeaturePhasesBinary
{
public:
  /**
   * @brief Initializes the binary feature-phase algorithm.
   * @param dataStructure Contains cell arrays and feature-phase output.
   * @param mesgHandler Supplies filter messages.
   * @param shouldCancel Signals cancellation between cell chunks.
   * @param inputValues Identifies cell arrays and output name.
   * @pre inputValues is not null.
   * @pre All arguments outlive this executor.
   */
  ComputeFeaturePhasesBinary(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeFeaturePhasesBinaryInputValues* inputValues);
  /**
   * @brief Destroys the binary feature-phase algorithm.
   */
  ~ComputeFeaturePhasesBinary() noexcept;

  ComputeFeaturePhasesBinary(const ComputeFeaturePhasesBinary&) = delete;
  ComputeFeaturePhasesBinary(ComputeFeaturePhasesBinary&&) noexcept = delete;
  ComputeFeaturePhasesBinary& operator=(const ComputeFeaturePhasesBinary&) = delete;
  ComputeFeaturePhasesBinary& operator=(ComputeFeaturePhasesBinary&&) noexcept = delete;

  /**
   * @brief Assigns binary phases from mask values.
   * @return Success, or a mask-type, Feature ID, or bulk-I/O error.
   *
   * Cancellation is checked between chunks. Cell-mask mode returns before its final output write;
   * Feature-mask mode can leave earlier output chunks written. Bulk I/O is not interrupted.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeFeaturePhasesBinaryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
