#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class LabelTriangleGeometryFilter
 * @brief
 */
class SIMPLNXCORE_EXPORT LabelTriangleGeometryFilter : public IFilter
{
public:
  LabelTriangleGeometryFilter() = default;
  ~LabelTriangleGeometryFilter() noexcept override = default;

  LabelTriangleGeometryFilter(const LabelTriangleGeometryFilter&) = delete;
  LabelTriangleGeometryFilter(LabelTriangleGeometryFilter&&) noexcept = delete;

  LabelTriangleGeometryFilter& operator=(const LabelTriangleGeometryFilter&) = delete;
  LabelTriangleGeometryFilter& operator=(LabelTriangleGeometryFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_TriangleGeomPath_Key = "input_triangle_geometry_path";
  static inline constexpr StringLiteral k_CreatedRegionIdsPath_Key = "created_region_ids_path";
  static inline constexpr StringLiteral k_TriangleAttributeMatrixName_Key = "triangle_attribute_matrix_name";
  static inline constexpr StringLiteral k_NumTrianglesName_Key = "num_triangles_name";

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
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, LabelTriangleGeometryFilter, "7a7a2c6f-3b03-444d-8b8c-5976b0e5c82e");
/* LEGACY UUID FOR THIS FILTER a250a228-3b6b-5b37-a6e4-8687484f04c4 */
