#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class AdaptiveAlignmentMutualInformation
 * @brief This filter will ....
 */
class DREAM3DREVIEW_EXPORT AdaptiveAlignmentMutualInformation : public IFilter
{
public:
  AdaptiveAlignmentMutualInformation() = default;
  ~AdaptiveAlignmentMutualInformation() noexcept override = default;

  AdaptiveAlignmentMutualInformation(const AdaptiveAlignmentMutualInformation&) = delete;
  AdaptiveAlignmentMutualInformation(AdaptiveAlignmentMutualInformation&&) noexcept = delete;

  AdaptiveAlignmentMutualInformation& operator=(const AdaptiveAlignmentMutualInformation&) = delete;
  AdaptiveAlignmentMutualInformation& operator=(AdaptiveAlignmentMutualInformation&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_GlobalCorrection_Key = "global_correction";
  static inline constexpr StringLiteral k_ImageDataArrayPath_Key = "image_data_array_path";
  static inline constexpr StringLiteral k_ShiftX_Key = "shift_x";
  static inline constexpr StringLiteral k_ShiftY_Key = "shift_y";
  static inline constexpr StringLiteral k_IgnoredDataArrayPaths_Key = "ignored_data_array_paths";
  static inline constexpr StringLiteral k_MisorientationTolerance_Key = "misorientation_tolerance";
  static inline constexpr StringLiteral k_UseGoodVoxels_Key = "use_good_voxels";
  static inline constexpr StringLiteral k_QuatsArrayPath_Key = "quats_array_path";
  static inline constexpr StringLiteral k_CellPhasesArrayPath_Key = "cell_phases_array_path";
  static inline constexpr StringLiteral k_GoodVoxelsArrayPath_Key = "good_voxels_array_path";
  static inline constexpr StringLiteral k_CrystalStructuresArrayPath_Key = "crystal_structures_array_path";

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
   * @brief Returns the default tags for this filter.
   * @return
   */
  std::vector<std::string> defaultTags() const override;

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
  PreflightResult preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param ds The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, AdaptiveAlignmentMutualInformation, "1a5dd69b-4af9-4575-8c26-b2237ddce13e");
