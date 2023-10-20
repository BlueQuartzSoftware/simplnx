#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class ErodeDilateMaskFilter
 * @brief This filter will erode or dilate data based on user supplied mask array.
 * If the mask is _dilated_, the **Filter** grows the *true* regions by one **Cell** in an iterative sequence for a user
defined number of iterations.
 */
class COMPLEXCORE_EXPORT ErodeDilateMaskFilter : public IFilter
{
public:
  ErodeDilateMaskFilter() = default;
  ~ErodeDilateMaskFilter() noexcept override = default;

  ErodeDilateMaskFilter(const ErodeDilateMaskFilter&) = delete;
  ErodeDilateMaskFilter(ErodeDilateMaskFilter&&) noexcept = delete;

  ErodeDilateMaskFilter& operator=(const ErodeDilateMaskFilter&) = delete;
  ErodeDilateMaskFilter& operator=(ErodeDilateMaskFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_Operation_Key = "operation";
  static inline constexpr StringLiteral k_NumIterations_Key = "num_iterations";
  static inline constexpr StringLiteral k_XDirOn_Key = "x_dir_on";
  static inline constexpr StringLiteral k_YDirOn_Key = "y_dir_on";
  static inline constexpr StringLiteral k_ZDirOn_Key = "z_dir_on";
  static inline constexpr StringLiteral k_MaskArrayPath_Key = "mask_array_path";
  static inline constexpr StringLiteral k_SelectedImageGeometry_Key = "selected_image_geometry";

  /**
   * @brief Reads SIMPL json and converts it complex Arguments.
   * @param json
   * @return Result<Arguments>
   */
  static Result<Arguments> FromSIMPLJson(const nlohmann::json& json);

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

COMPLEX_DEF_FILTER_TRAITS(complex, ErodeDilateMaskFilter, "cab66cd1-f64c-42b4-8f94-18f0835a967f");
/* LEGACY UUID FOR THIS FILTER 4fff1aa6-4f62-56c4-8ee9-8e28ec2fcbba */
