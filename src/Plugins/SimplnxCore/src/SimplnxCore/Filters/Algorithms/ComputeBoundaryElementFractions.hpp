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
 * @struct ComputeBoundaryElementFractionsInputValues
 * @brief Identifies cell inputs and the feature-level fraction output.
 */
struct SIMPLNXCORE_EXPORT ComputeBoundaryElementFractionsInputValues
{
  DataObjectNameParameter::ValueType BoundaryCellFractionsArrayName;
  ArraySelectionParameter::ValueType BoundaryCellsArrayPath;
  AttributeMatrixSelectionParameter::ValueType FeatureDataAttributeMatrixPath;
  ArraySelectionParameter::ValueType FeatureIdsArrayPath;
};

/**
 * @class ComputeBoundaryElementFractions
 * @brief Computes each feature's fraction of cells that are boundary cells.
 *
 * Cell arrays are streamed through bounded bulk-I/O buffers so out-of-core
 * stores avoid per-cell chunk-cache access while feature-level counts remain
 * in memory. Memory scales with the feature count plus 65,536 cell tuples.
 *
 * Counts use Float32 and can lose integer precision for large features. Feature
 * zero retains its existing output value. A positive feature with no cells
 * produces NaN through zero division.
 */

class SIMPLNXCORE_EXPORT ComputeBoundaryElementFractions
{
public:
  /**
   * @brief Initializes boundary-fraction calculation.
   * @param dataStructure Provides cell arrays and feature output.
   * @param mesgHandler Supplies the filter message handler.
   * @param shouldCancel Signals cancellation between pages.
   * @param inputValues Identifies input and output paths.
   * @pre All arguments outlive this executor.
   */
  ComputeBoundaryElementFractions(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                  ComputeBoundaryElementFractionsInputValues* inputValues);
  ~ComputeBoundaryElementFractions() noexcept;

  ComputeBoundaryElementFractions(const ComputeBoundaryElementFractions&) = delete;
  ComputeBoundaryElementFractions(ComputeBoundaryElementFractions&&) noexcept = delete;
  ComputeBoundaryElementFractions& operator=(const ComputeBoundaryElementFractions&) = delete;
  ComputeBoundaryElementFractions& operator=(ComputeBoundaryElementFractions&&) noexcept = delete;

  /**
   * @brief Counts boundary cells and writes one fraction per positive feature.
   * @return Source or destination bulk-I/O errors.
   * @pre Cell arrays have equal tuple counts.
   * @pre Every Feature ID is nonnegative and less than the output tuple count.
   *
   * Cancellation returns success before any locally accumulated output is written.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeBoundaryElementFractionsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
