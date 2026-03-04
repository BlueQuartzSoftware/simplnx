#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/AbstractFilter.hpp"
#include "simplnx/Filter/FilterTraits.hpp"

namespace nx::core
{
/**
 * @class CreateAMScanPathsFilter
 * @brief This filter will generate additive manufacturing scan paths from an edge geometry
 */
class SIMPLNXCORE_EXPORT CreateAMScanPathsFilter : public AbstractFilter
{
public:
  CreateAMScanPathsFilter() = default;
  ~CreateAMScanPathsFilter() noexcept override = default;

  CreateAMScanPathsFilter(const CreateAMScanPathsFilter&) = delete;
  CreateAMScanPathsFilter(CreateAMScanPathsFilter&&) noexcept = delete;

  CreateAMScanPathsFilter& operator=(const CreateAMScanPathsFilter&) = delete;
  CreateAMScanPathsFilter& operator=(CreateAMScanPathsFilter&&) noexcept = delete;

  // Parameter Keys
  static constexpr StringLiteral k_StripeWidth_Key = "hatch_length";
  static constexpr StringLiteral k_HatchSpacing_Key = "hatch_spacing";
  static constexpr StringLiteral k_RotationAngle = "rotation_angle";
  static constexpr StringLiteral k_CADSliceDataContainerPath_Key = "cad_slice_data_container_path";
  static constexpr StringLiteral k_CADSliceIdsArrayPath_Key = "cad_slice_ids_array_path";
  static constexpr StringLiteral k_CADRegionIdsArrayPath_Key = "cad_region_ids_array_path";
  static constexpr StringLiteral k_HatchDataContainerPath_Key = "hatch_data_container_path";
  static constexpr StringLiteral k_VertexAttributeMatrixName_Key = "vertex_attribute_matrix_name";
  static constexpr StringLiteral k_HatchAttributeMatrixName_Key = "hatch_attribute_matrix_name";
  static constexpr StringLiteral k_RegionIdsArrayName_Key = "region_ids_array_name";

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
   * @brief Returns the human-readable name of the filter.
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
   * @brief Returns parameters version integer.
   * The Initial version should always be 1.
   * Should be incremented everytime the parameters change.
   * @return VersionType
   */
  VersionType parametersVersion() const override;

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
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @param shouldCancel Atomic boolean value that can be checked to cancel the filter
   * @param executionContext The ExecutionContext that can be used to determine the correct absolute path from a relative path
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                const ExecutionContext& executionContext) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param pipelineNode The node in the pipeline that is being executed
   * @param messageHandler The MessageHandler object
   * @param shouldCancel Atomic boolean value that can be checked to cancel the filter
   * @param executionContext The ExecutionContext that can be used to determine the correct absolute path from a relative path
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                       const ExecutionContext& executionContext) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, CreateAMScanPathsFilter, "b757be16-1418-4b69-b475-99e63d00a2e3");
/* LEGACY UUID FOR THIS FILTER 08de1ffb-6cb1-5896-8133-a30d2dd0f937 */
