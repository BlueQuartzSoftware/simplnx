#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/AbstractFilter.hpp"
#include "simplnx/Filter/FilterTraits.hpp"

namespace nx::core
{
/**
 * @class ComputeCoordinatesImageGeomFilter
 * @brief This filter will ....
 */
class SIMPLNXCORE_EXPORT ComputeCoordinatesImageGeomFilter : public AbstractFilter
{
public:
  ComputeCoordinatesImageGeomFilter() = default;
  ~ComputeCoordinatesImageGeomFilter() noexcept override = default;

  ComputeCoordinatesImageGeomFilter(const ComputeCoordinatesImageGeomFilter&) = delete;
  ComputeCoordinatesImageGeomFilter(ComputeCoordinatesImageGeomFilter&&) noexcept = delete;

  ComputeCoordinatesImageGeomFilter& operator=(const ComputeCoordinatesImageGeomFilter&) = delete;
  ComputeCoordinatesImageGeomFilter& operator=(ComputeCoordinatesImageGeomFilter&&) noexcept = delete;

  // Parameter Keys
  static constexpr StringLiteral k_OutputType_Key = "output_type_index";
  static constexpr StringLiteral k_SelectedImageGeomPath_Key = "selected_image_geom_path";
  static constexpr StringLiteral k_CoordsArrayPath_Key = "coords_array_path";
  static constexpr StringLiteral k_IndicesArrayPath_Key = "indices_array_path";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ComputeCoordinatesImageGeomFilter, "ff9c292f-eb3d-4c35-a80d-9c099aaa4714");
