#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
class SIMPLNXCORE_EXPORT ExtractInternalSurfacesFromTriangleGeometryFilter : public IFilter
{
public:
  ExtractInternalSurfacesFromTriangleGeometryFilter() = default;
  ~ExtractInternalSurfacesFromTriangleGeometryFilter() noexcept override = default;

  ExtractInternalSurfacesFromTriangleGeometryFilter(const ExtractInternalSurfacesFromTriangleGeometryFilter&) = delete;
  ExtractInternalSurfacesFromTriangleGeometryFilter(ExtractInternalSurfacesFromTriangleGeometryFilter&&) noexcept = delete;

  ExtractInternalSurfacesFromTriangleGeometryFilter& operator=(const ExtractInternalSurfacesFromTriangleGeometryFilter&) = delete;
  ExtractInternalSurfacesFromTriangleGeometryFilter& operator=(ExtractInternalSurfacesFromTriangleGeometryFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_SelectedTriangleGeometryPath_Key = "input_triangle_geometry_path";
  static inline constexpr StringLiteral k_CreatedTriangleGeometryPath_Key = "output_triangle_geometry_path";
  static inline constexpr StringLiteral k_NodeTypesPath_Key = "node_types_path";
  static inline constexpr StringLiteral k_CopyVertexPaths_Key = "copy_vertex_array_paths";
  static inline constexpr StringLiteral k_CopyTrianglePaths_Key = "copy_triangle_array_paths";
  static inline constexpr StringLiteral k_VertexAttributeMatrixName_Key = "vertex_attribute_matrix_name";
  static inline constexpr StringLiteral k_TriangleAttributeMatrixName_Key = "triangle_attribute_matrix_name";
  static inline constexpr StringLiteral k_NodeTypeRange_Key = "node_type_range";

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
   * @brief Returns the filter's UUID.
   * @return Uuid
   */
  Uuid uuid() const override;

  /**
   * @brief Returns the filter name as a human-readable string.
   * @return std::string
   */
  std::string humanName() const override;

  /**
   * @brief Returns the default tags for this filter.
   * @return
   */
  std::vector<std::string> defaultTags() const override;

  /**
   * @brief Returns a collection of parameters required to execute the filter.
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
   * @brief Creates and returns a copy of the filter.
   * @return UniquePointer
   */
  UniquePointer clone() const override;

protected:
  /**
   * @brief
   * @param data
   * @param filterArgs
   * @param messageHandler
   * @param shouldCancel
   * @return Result<OutputActions>
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief
   * @param dataStructure
   * @param args
   * @param pipelineNode
   * @param messageHandler
   * @param shouldCancel
   * @return Result<>
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                       const std::atomic_bool& shouldCancel) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ExtractInternalSurfacesFromTriangleGeometryFilter, "e020f76f-a77f-4999-8bf1-9b7529f06d0a");
