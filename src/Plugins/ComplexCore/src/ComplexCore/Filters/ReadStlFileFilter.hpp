#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @brief ReadStlFileFilter This filter will read a Binary STL file into a Triangle
 * Geometry including the Normal Vector for each face. The actual algorithm is
 * contained in the ComplexCore/Filters/Algorithms/ReadStlFile
 *
 */
class COMPLEXCORE_EXPORT ReadStlFileFilter : public IFilter
{
public:
  ReadStlFileFilter() = default;
  ~ReadStlFileFilter() noexcept override = default;

  ReadStlFileFilter(const ReadStlFileFilter&) = delete;
  ReadStlFileFilter(ReadStlFileFilter&&) noexcept = delete;

  ReadStlFileFilter& operator=(const ReadStlFileFilter&) = delete;
  ReadStlFileFilter& operator=(ReadStlFileFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_ScaleOutput = "scale_output";
  static inline constexpr StringLiteral k_ScaleFactor = "scale_factor";
  static inline constexpr StringLiteral k_StlFilePath_Key = "stl_file_path";

  static inline constexpr StringLiteral k_TriangleGeometryName_Key = "triangle_geometry_name";

  static inline constexpr StringLiteral k_VertexAttributeMatrix_Key = "vertex_attribute_matrix";
  static inline constexpr StringLiteral k_FaceAttributeMatrix_Key = "face_attribute_matrix";
  static inline constexpr StringLiteral k_FaceNormalsName_Key = "face_normals_data_path";

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

COMPLEX_DEF_FILTER_TRAITS(complex, ReadStlFileFilter, "2f64bd45-9d28-4254-9e07-6aa7c6d3d015");
