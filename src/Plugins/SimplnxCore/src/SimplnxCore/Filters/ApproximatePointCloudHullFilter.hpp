#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ApproximatePointCloudHullFilter
 * @brief
 */
class SIMPLNXCORE_EXPORT ApproximatePointCloudHullFilter : public IFilter
{
public:
  ApproximatePointCloudHullFilter() = default;
  ~ApproximatePointCloudHullFilter() noexcept override = default;

  ApproximatePointCloudHullFilter(const ApproximatePointCloudHullFilter&) = delete;
  ApproximatePointCloudHullFilter(ApproximatePointCloudHullFilter&&) noexcept = delete;

  ApproximatePointCloudHullFilter& operator=(const ApproximatePointCloudHullFilter&) = delete;
  ApproximatePointCloudHullFilter& operator=(ApproximatePointCloudHullFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_GridResolution_Key = "grid_resolution";
  static inline constexpr StringLiteral k_MinEmptyNeighbors_Key = "min_empty_neighbors";
  static inline constexpr StringLiteral k_VertexGeomPath_Key = "input_vertex_geometry_path";
  static inline constexpr StringLiteral k_HullVertexGeomPath_Key = "output_vertex_geometry_path";

  /**
   * @brief Reads SIMPL json and converts it simplnx Arguments.
   * @param json
   * @return Result<Arguments>
   */
  static Result<Arguments> FromSIMPLJson(const nlohmann::json& json);

  /**
   * @brief Returns the filter's name.
   * @return std::string
   */
  std::string name() const override;

  /**
   * @brief Returns the C++ classname of this filter.
   * @return std::string
   */
  std::string className() const override;

  /**
   * @brief Returns the filter's uuid.
   * @return Uuid
   */
  Uuid uuid() const override;

  /**
   * @brief Returns the user-friendly filter name.
   * @return std::string
   */
  std::string humanName() const override;

  /**
   * @brief Returns the default tags for this filter.
   * @return
   */
  std::vector<std::string> defaultTags() const override;

  /**
   * @brief Returns a copy of the filter's parameters.
   * @return Parameters
   */
  Parameters parameters() const override;

  /**
   * @brief Returns parameters version integer.
   * Initial version should always be 1.
   * Should be incremented everytime the parameters change.
   * @return VersionType
   */
  VersionType parametersVersion() const override;

  /**
   * @brief Returns a copy of the filter.
   * @return UniquePointer
   */
  UniquePointer clone() const override;

protected:
  /**
   * @brief
   * @param data
   * @param args
   * @param messageHandler
   * @return Result<OutputActions>
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param pipelineNode The PipelineNode object that called this filter
   * @param messageHandler The MessageHandler object
   * @param shouldCancel The atomic boolean that holds if the filter should be canceled
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                       const std::atomic_bool& shouldCancel) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ApproximatePointCloudHullFilter, "c19203b7-2217-4e52-bff4-7f611695421a");
