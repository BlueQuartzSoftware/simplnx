#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class CombineStlFilesFilter
 * @brief This filter will combine all of the STL files from a given directory into a single triangle geometry
 */
class SIMPLNXCORE_EXPORT CombineStlFilesFilter : public IFilter
{
public:
  CombineStlFilesFilter() = default;
  ~CombineStlFilesFilter() noexcept override = default;

  CombineStlFilesFilter(const CombineStlFilesFilter&) = delete;
  CombineStlFilesFilter(CombineStlFilesFilter&&) noexcept = delete;

  CombineStlFilesFilter& operator=(const CombineStlFilesFilter&) = delete;
  CombineStlFilesFilter& operator=(CombineStlFilesFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_StlFilesPath_Key = "stl_files_path";
  static inline constexpr StringLiteral k_TriangleGeometryPath_Key = "output_triangle_geometry_path";
  static inline constexpr StringLiteral k_FaceAttributeMatrixName_Key = "face_attribute_matrix_name";
  static inline constexpr StringLiteral k_FaceNormalsArrayName_Key = "face_normals_array_name";
  static inline constexpr StringLiteral k_VertexAttributeMatrixName_Key = "vertex_attribute_matrix_name";
  static inline constexpr StringLiteral k_LabelFaces_Key = "label_faces";
  static inline constexpr StringLiteral k_FaceLabelName_Key = "face_label_name";
  static inline constexpr StringLiteral k_LabelVertices_Key = "label_vertices";
  static inline constexpr StringLiteral k_VertexLabelName_Key = "vertex_label_name";
  static inline constexpr StringLiteral k_CellFeatureAttributeMatrixName_Key = "cell_feature_attribute_matrix_name";
  static inline constexpr StringLiteral k_ActiveArrayName_Key = "active_array_name";
  static inline constexpr StringLiteral k_FileListName_Key = "output_file_list_name";

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
   * @brief Returns parameters version integer.
   * Initial version should always be 1.
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
   * @param ds The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param ds The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                       const std::atomic_bool& shouldCancel) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, CombineStlFilesFilter, "76b56f80-fcbe-4d48-a34d-a73d0fc6e5ae");
/* LEGACY UUID FOR THIS FILTER 71d46128-1d2d-58fd-9924-1714695768c3 */
