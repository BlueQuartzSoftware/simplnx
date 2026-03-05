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
namespace detail
{
inline constexpr StringLiteral k_LessThan = "< [Less Than]";
inline constexpr StringLiteral k_GreaterThan = "> [Greater Than]";
inline const ChoicesParameter::Choices k_OperationChoices = {k_LessThan, k_GreaterThan};
} // namespace detail

/**
 * @struct ReplaceElementAttributesWithNeighborValuesInputValues
 * @brief Holds all user-supplied parameters for the ReplaceElementAttributesWithNeighborValues algorithm.
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
 * @brief Replaces voxel data with the best face-neighbor value based on a threshold comparison.
 */
class SIMPLNXCORE_EXPORT ReplaceElementAttributesWithNeighborValues
{
public:
  /**
   * @brief Constructs the algorithm with all required references and parameters.
   * @param dataStructure The DataStructure containing all input/output arrays
   * @param mesgHandler Handler for sending progress messages to the UI
   * @param shouldCancel Atomic flag checked between iterations to support cancellation
   * @param inputValues User-supplied parameters controlling the algorithm behavior
   */
  ReplaceElementAttributesWithNeighborValues(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                             ReplaceElementAttributesWithNeighborValuesInputValues* inputValues);

  /**
   * @brief Default destructor.
   */
  ~ReplaceElementAttributesWithNeighborValues() noexcept;

  ReplaceElementAttributesWithNeighborValues(const ReplaceElementAttributesWithNeighborValues&) = delete;
  ReplaceElementAttributesWithNeighborValues(ReplaceElementAttributesWithNeighborValues&&) noexcept = delete;
  ReplaceElementAttributesWithNeighborValues& operator=(const ReplaceElementAttributesWithNeighborValues&) = delete;
  ReplaceElementAttributesWithNeighborValues& operator=(ReplaceElementAttributesWithNeighborValues&&) noexcept = delete;

  /**
   * @brief Executes the replace element attributes with neighbor values algorithm.
   * @return Result<> indicating success or any errors encountered during execution
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
