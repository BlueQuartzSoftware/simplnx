#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

namespace nx::core
{
/**
 * @namespace nx::core::detail
 * @brief Contains the shared comparison choices for neighbor replacement.
 */
namespace detail
{
inline constexpr StringLiteral k_LessThan = "< [Less Than]";                             ///< Selects replacement below the threshold.
inline constexpr StringLiteral k_GreaterThan = "> [Greater Than]";                       ///< Selects replacement above the threshold.
inline const ChoicesParameter::Choices k_OperationChoices = {k_LessThan, k_GreaterThan}; ///< Defines the filter choice order.
} // namespace detail

/**
 * @struct ReplaceElementAttributesWithNeighborValuesInputValues
 * @brief Stores the comparison, loop, array, and geometry selections.
 */
struct SIMPLNXCORE_EXPORT ReplaceElementAttributesWithNeighborValuesInputValues
{
  float32 MinConfidence;
  ChoicesParameter::ValueType SelectedComparison;
  bool Loop;
  DataPath InputArrayPath;
  DataPath SelectedImageGeometryPath;
};

/**
 * @class ReplaceElementAttributesWithNeighborValues
 * @brief Replaces failed cell tuples from a passing face neighbor.
 *
 * Less-than mode replaces a value below the threshold from the greatest
 * neighbor at or above the threshold. Greater-than mode replaces a value above
 * the threshold from the least neighbor at or below the threshold. The
 * float32 threshold converts to the selected array's value type.
 *
 * Each pass keeps three comparison-array Z slices and one int64 source mark per
 * XY cell. A selected source supplies the complete tuple for every IDataArray
 * in the ImageGeom cell AttributeMatrix. Arrays process sequentially, so peak
 * transfer memory also includes as many as four slices of one sibling array.
 *
 * The comparison window preserves values from the start of a pass. The
 * algorithm commits each destination Z slice immediately to limit mark memory.
 * A sibling tuple copied from a prior Z slice can therefore contain a value
 * that this pass already copied. This makes tuple transfer dependent on Z order.
 *
 * Loop mode repeats while any value fails, even when no passing neighbor exists.
 * It has no independent pass limit and can fail to terminate. Cancellation is
 * checked only between complete passes. Bulk read and transfer results are
 * discarded, and the operation does not roll back prior slice writes.
 */
class SIMPLNXCORE_EXPORT ReplaceElementAttributesWithNeighborValues
{
public:
  /**
   * @brief Initializes the neighbor-replacement algorithm.
   * @param dataStructure Contains the ImageGeom and its cell arrays.
   * @param mesgHandler Receives pass progress messages.
   * @param shouldCancel Signals cancellation between passes.
   * @param inputValues Selects comparison and loop behavior.
   * @pre inputValues is not null.
   * @pre All arguments outlive this executor.
   */
  ReplaceElementAttributesWithNeighborValues(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                             ReplaceElementAttributesWithNeighborValuesInputValues* inputValues);

  /**
   * @brief Destroys the neighbor-replacement algorithm.
   */
  ~ReplaceElementAttributesWithNeighborValues() noexcept;

  ReplaceElementAttributesWithNeighborValues(const ReplaceElementAttributesWithNeighborValues&) = delete;
  ReplaceElementAttributesWithNeighborValues(ReplaceElementAttributesWithNeighborValues&&) noexcept = delete;
  ReplaceElementAttributesWithNeighborValues& operator=(const ReplaceElementAttributesWithNeighborValues&) = delete;
  ReplaceElementAttributesWithNeighborValues& operator=(ReplaceElementAttributesWithNeighborValues&&) noexcept = delete;

  /**
   * @brief Replaces failed cell tuples for one or more passes.
   * @return Success after completion or cancellation.
   * @pre InputArrayPath identifies a scalar cell array.
   * @pre The input and sibling arrays match the ImageGeom cell dimensions.
   *
   * The method cannot report bulk-I/O failures. A between-pass cancellation
   * checkpoint returns success and can leave prior slices modified.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel() const;

private:
  DataStructure& m_DataStructure;
  const ReplaceElementAttributesWithNeighborValuesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
