#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class FindVertexToTriangleDistancesFilter
 * @brief This filter will ....
 */
class SIMPLNXCORE_EXPORT FindVertexToTriangleDistancesFilter : public IFilter
{
public:
  FindVertexToTriangleDistancesFilter() = default;
  ~FindVertexToTriangleDistancesFilter() noexcept override = default;

  FindVertexToTriangleDistancesFilter(const FindVertexToTriangleDistancesFilter&) = delete;
  FindVertexToTriangleDistancesFilter(FindVertexToTriangleDistancesFilter&&) noexcept = delete;

  FindVertexToTriangleDistancesFilter& operator=(const FindVertexToTriangleDistancesFilter&) = delete;
  FindVertexToTriangleDistancesFilter& operator=(FindVertexToTriangleDistancesFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_SelectedVertexGeometryPath_Key = "selected_vertex_geometry_path";
  static inline constexpr StringLiteral k_SelectedTriangleGeometryPath_Key = "selected_triangle_geometry_path";
  static inline constexpr StringLiteral k_TriangleNormalsArrayPath_Key = "triangle_normals_array_path";
  static inline constexpr StringLiteral k_DistancesArrayName_Key = "distances_array_name";
  static inline constexpr StringLiteral k_ClosestTriangleIdArrayName_Key = "closest_triangle_id_array_name";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, FindVertexToTriangleDistancesFilter, "af60518e-5fdb-45f2-9028-cd7787016830");
/* LEGACY UUID FOR THIS FILTER fcdde553-36b4-5731-bc88-fc499806cb4e */
