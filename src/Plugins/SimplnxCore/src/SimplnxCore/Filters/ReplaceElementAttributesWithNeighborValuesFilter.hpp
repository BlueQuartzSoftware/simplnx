#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ReplaceElementAttributesWithNeighborValuesFilter
 * @brief This filter will replace values based on neighbor values. More information can
 * be found in the help file located in docs/ReplaceElementAttributesWithNeighborValuesFilter.md
 */
class SIMPLNXCORE_EXPORT ReplaceElementAttributesWithNeighborValuesFilter : public IFilter
{
public:
  ReplaceElementAttributesWithNeighborValuesFilter() = default;
  ~ReplaceElementAttributesWithNeighborValuesFilter() noexcept override = default;

  ReplaceElementAttributesWithNeighborValuesFilter(const ReplaceElementAttributesWithNeighborValuesFilter&) = delete;
  ReplaceElementAttributesWithNeighborValuesFilter(ReplaceElementAttributesWithNeighborValuesFilter&&) noexcept = delete;

  ReplaceElementAttributesWithNeighborValuesFilter& operator=(const ReplaceElementAttributesWithNeighborValuesFilter&) = delete;
  ReplaceElementAttributesWithNeighborValuesFilter& operator=(ReplaceElementAttributesWithNeighborValuesFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_MinConfidence_Key = "min_confidence";
  static inline constexpr StringLiteral k_SelectedComparison_Key = "selected_comparison";
  static inline constexpr StringLiteral k_Loop_Key = "loop";
  static inline constexpr StringLiteral k_ComparisonDataPath = "comparison_data_path";
  static inline constexpr StringLiteral k_SelectedImageGeometryPath_Key = "selected_image_geometry_path";

  /**
   * @brief Reads SIMPL json and converts it simplnx Arguments.
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
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ReplaceElementAttributesWithNeighborValuesFilter, "65128c53-d3be-4a69-a559-32a48d603884");
/* LEGACY UUID FOR THIS FILTER 17410178-4e5f-58b9-900e-8194c69200ab */
