#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class RemoveFlaggedEdgesFilter
 * @brief This filter will ....
 */
class SIMPLNXCORE_EXPORT RemoveFlaggedEdgesFilter : public IFilter
{
public:
  RemoveFlaggedEdgesFilter() = default;
  ~RemoveFlaggedEdgesFilter() noexcept override = default;

  RemoveFlaggedEdgesFilter(const RemoveFlaggedEdgesFilter&) = delete;
  RemoveFlaggedEdgesFilter(RemoveFlaggedEdgesFilter&&) noexcept = delete;

  RemoveFlaggedEdgesFilter& operator=(const RemoveFlaggedEdgesFilter&) = delete;
  RemoveFlaggedEdgesFilter& operator=(RemoveFlaggedEdgesFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_InputEdgeGeometryPath_Key = "input_edge_geometry_path";
  static inline constexpr StringLiteral k_MaskArrayPath_Key = "mask_array_path";
  static inline constexpr StringLiteral k_OutputEdgeGeometryPath_Key = "output_edge_geometry_path";

  static inline constexpr StringLiteral k_EdgeDataHandling_Key = "edge_data_handling_index";
  static inline constexpr StringLiteral k_EdgeDataSelectedArrays_Key = "edge_data_selected_array_paths";
  static inline constexpr StringLiteral k_EdgeDataSelectedAttributeMatrix_Key = "edge_data_selected_attribute_matrix_path";

  static inline constexpr StringLiteral k_VertexDataHandling_Key = "vertex_data_handling_index";
  static inline constexpr StringLiteral k_VertexDataSelectedArrays_Key = "vertex_data_selected_array_paths";
  static inline constexpr StringLiteral k_VertexDataSelectedAttributeMatrix_Key = "vertex_data_selected_attribute_matrix_path";

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
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param ds The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                       const std::atomic_bool& shouldCancel) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, RemoveFlaggedEdgesFilter, "48155f61-2709-4731-be95-43745bb3f8d8");
