#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class CreateLambertSphere
 * @brief This filter will ....
 */
class ORIENTATIONANALYSIS_EXPORT CreateLambertSphere : public IFilter
{
public:
  CreateLambertSphere() = default;
  ~CreateLambertSphere() noexcept override = default;

  CreateLambertSphere(const CreateLambertSphere&) = delete;
  CreateLambertSphere(CreateLambertSphere&&) noexcept = delete;

  CreateLambertSphere& operator=(const CreateLambertSphere&) = delete;
  CreateLambertSphere& operator=(CreateLambertSphere&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_Hemisphere_Key = "hemisphere";
  static inline constexpr StringLiteral k_CreateVertexGeometry_Key = "create_vertex_geometry";
  static inline constexpr StringLiteral k_VertexDataContainerName_Key = "vertex_data_container_name";
  static inline constexpr StringLiteral k_CreateEdgeGeometry_Key = "create_edge_geometry";
  static inline constexpr StringLiteral k_EdgeDataContainerName_Key = "edge_data_container_name";
  static inline constexpr StringLiteral k_CreateTriangleGeometry_Key = "create_triangle_geometry";
  static inline constexpr StringLiteral k_TriangleDataContainerName_Key = "triangle_data_container_name";
  static inline constexpr StringLiteral k_CreateQuadGeometry_Key = "create_quad_geometry";
  static inline constexpr StringLiteral k_QuadDataContainerName_Key = "quad_data_container_name";
  static inline constexpr StringLiteral k_ImageDataArrayPath_Key = "image_data_array_path";

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
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, CreateLambertSphere, "916e2881-4c1c-47bd-99b0-6fb183ecdcea");
