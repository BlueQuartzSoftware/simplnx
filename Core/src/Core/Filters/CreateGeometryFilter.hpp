#pragma once

#include "Core/Core_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class CreateGeometryFilter
 * @brief This filter will ....
 */
class CORE_EXPORT CreateGeometryFilter : public IFilter
{
public:
  CreateGeometryFilter() = default;
  ~CreateGeometryFilter() noexcept override = default;

  CreateGeometryFilter(const CreateGeometryFilter&) = delete;
  CreateGeometryFilter(CreateGeometryFilter&&) noexcept = delete;

  CreateGeometryFilter& operator=(const CreateGeometryFilter&) = delete;
  CreateGeometryFilter& operator=(CreateGeometryFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_GeometryType_Key = "geometry_type";
  static inline constexpr StringLiteral k_GeometryName_Key = "geometry_name";
  static inline constexpr StringLiteral k_WarningsAsErrors_Key = "warnings_as_errors";
  static inline constexpr StringLiteral k_ArrayHandling_Key = "array_handling";
  // Image Geometry
  static inline constexpr StringLiteral k_Dimensions_Key = "dimensions";
  static inline constexpr StringLiteral k_Origin_Key = "origin";
  static inline constexpr StringLiteral k_Spacing_Key = "spacing";
  // RectilinearGrid Geometry
  static inline constexpr StringLiteral k_XBounds_Key = "x_bounds";
  static inline constexpr StringLiteral k_YBounds_Key = "y_bounds";
  static inline constexpr StringLiteral k_ZBounds_Key = "z_bounds";
  // Vertex, Edge, Triangle, Quadrilateral, Tetrahedral, & Hexahedral Geometry
  static inline constexpr StringLiteral k_VertexListName_Key = "vertex_list_name";
  static inline constexpr StringLiteral k_VertexAttributeMatrixName_Key = "vertex_attribute_matrix_name";
  // Edge Geometry
  static inline constexpr StringLiteral k_EdgeListName_Key = "edge_list_name";
  static inline constexpr StringLiteral k_EdgeAttributeMatrixName_Key = "edge_attribute_matrix_name";
  // Triangle Geometry
  static inline constexpr StringLiteral k_TriangleListName_Key = "triangle_list_name";
  // Quadrilateral Geometry
  static inline constexpr StringLiteral k_QuadrilateralListName_Key = "quadrilateral_list_name";
  // Triangle & Quadrilateral Geometry
  static inline constexpr StringLiteral k_FaceAttributeMatrixName_Key = "face_attribute_matrix_name";
  // Tetrahedral Geometry
  static inline constexpr StringLiteral k_TetrahedralListName_Key = "tetrahedral_list_name";
  // Hexahedral Geometry
  static inline constexpr StringLiteral k_HexahedralListName_Key = "hexahedral_list_name";
  // Image, RectilinearGrid, Tetrahedral, & Hexahedral Geometry
  static inline constexpr StringLiteral k_CellAttributeMatrixName_Key = "cell_attribute_matrix_name";

  // GeometryType values
  static inline constexpr uint64 k_ImageGeometry = 0;
  static inline constexpr uint64 k_RectGridGeometry = 1;
  static inline constexpr uint64 k_VertexGeometry = 2;
  static inline constexpr uint64 k_EdgeGeometry = 3;
  static inline constexpr uint64 k_TriangleGeometry = 4;
  static inline constexpr uint64 k_QuadGeometry = 5;
  static inline constexpr uint64 k_TetGeometry = 6;
  static inline constexpr uint64 k_HexGeometry = 7;
  // ArrayHandling values
  static inline constexpr uint64 k_CopyArray = 0;
  static inline constexpr uint64 k_MoveArray = 1;
  static inline constexpr uint64 k_ReferenceArray = 2;

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
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, CreateGeometryFilter, "24768170-5b90-4a9d-82ac-9aeecd9f892e");
