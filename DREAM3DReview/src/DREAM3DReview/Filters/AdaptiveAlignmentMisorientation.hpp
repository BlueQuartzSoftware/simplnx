#pragma once

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @class AdaptiveAlignmentMisorientation
 * @brief This filter will ....
 */
class COMPLEX_EXPORT AdaptiveAlignmentMisorientation : public IFilter
{
public:
  AdaptiveAlignmentMisorientation() = default;
  ~AdaptiveAlignmentMisorientation() noexcept override = default;

  AdaptiveAlignmentMisorientation(const AdaptiveAlignmentMisorientation&) = delete;
  AdaptiveAlignmentMisorientation(AdaptiveAlignmentMisorientation&&) noexcept = delete;

  AdaptiveAlignmentMisorientation& operator=(const AdaptiveAlignmentMisorientation&) = delete;
  AdaptiveAlignmentMisorientation& operator=(AdaptiveAlignmentMisorientation&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_GlobalCorrection_Key = "GlobalCorrection";
  static inline constexpr StringLiteral k_ImageDataArrayPath_Key = "ImageDataArrayPath";
  static inline constexpr StringLiteral k_ShiftX_Key = "ShiftX";
  static inline constexpr StringLiteral k_ShiftY_Key = "ShiftY";
  static inline constexpr StringLiteral k_IgnoredDataArrayPaths_Key = "IgnoredDataArrayPaths";
  static inline constexpr StringLiteral k_MisorientationTolerance_Key = "MisorientationTolerance";
  static inline constexpr StringLiteral k_UseGoodVoxels_Key = "UseGoodVoxels";
  static inline constexpr StringLiteral k_QuatsArrayPath_Key = "QuatsArrayPath";
  static inline constexpr StringLiteral k_CellPhasesArrayPath_Key = "CellPhasesArrayPath";
  static inline constexpr StringLiteral k_GoodVoxelsArrayPath_Key = "GoodVoxelsArrayPath";
  static inline constexpr StringLiteral k_CrystalStructuresArrayPath_Key = "CrystalStructuresArrayPath";

  /**
   * @brief Returns the name of the filter.
   * @return
   */
  std::string name() const override;

  /**
   * @brief Returns the C++ classname of this filter.
   * @return
   */
  std::string className() const override;

  /**
   * @brief Returns the uuid of the filter.
   * @return
   */
  Uuid uuid() const override;

  /**
   * @brief Returns the human readable name of the filter.
   * @return
   */
  std::string humanName() const override;

  /**
   * @brief Returns the parameters of the filter (i.e. its inputs)
   * @return
   */
  Parameters parameters() const override;

  /**
   * @brief Returns a copy of the filter.
   * @return
   */
  UniquePointer clone() const override;

protected:
  /**
   * @brief Takes in a DataStructure and checks that the filter can be run on it with the given arguments.
   * Returns any warnings/errors. Also returns the changes that would be applied to the DataStructure.
   * Some parts of the actions may not be completely filled out if all the required information is not available at preflight time.
   * @param ds The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<OutputActions> preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param ds The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, AdaptiveAlignmentMisorientation, "d35f6a05-30b8-544c-96d4-8bea25363dc3");
