#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ApproximatePointCloudHull
 * @brief
 */
class SIMPLNXCORE_EXPORT ApproximatePointCloudHull : public IFilter
{
public:
  ApproximatePointCloudHull() = default;
  ~ApproximatePointCloudHull() noexcept override = default;

  ApproximatePointCloudHull(const ApproximatePointCloudHull&) = delete;
  ApproximatePointCloudHull(ApproximatePointCloudHull&&) noexcept = delete;

  ApproximatePointCloudHull& operator=(const ApproximatePointCloudHull&) = delete;
  ApproximatePointCloudHull& operator=(ApproximatePointCloudHull&&) noexcept = delete;

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
  PreflightResult preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief
   * @param data
   * @param args
   * @param pipelineNode
   * @param messageHandler
   * @return Result<>
   */
  Result<> executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ApproximatePointCloudHull, "c19203b7-2217-4e52-bff4-7f611695421a");
