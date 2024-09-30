#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ReadVtkStructuredPointsFilter
 * @brief This filter will ....
 */
class SIMPLNXCORE_EXPORT ReadVtkStructuredPointsFilter : public IFilter
{
public:
  ReadVtkStructuredPointsFilter() = default;
  ~ReadVtkStructuredPointsFilter() noexcept override = default;

  ReadVtkStructuredPointsFilter(const ReadVtkStructuredPointsFilter&) = delete;
  ReadVtkStructuredPointsFilter(ReadVtkStructuredPointsFilter&&) noexcept = delete;

  ReadVtkStructuredPointsFilter& operator=(const ReadVtkStructuredPointsFilter&) = delete;
  ReadVtkStructuredPointsFilter& operator=(ReadVtkStructuredPointsFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_InputFile_Key = "input_file";
  static inline constexpr StringLiteral k_ReadPointData_Key = "read_point_data";
  static inline constexpr StringLiteral k_ReadCellData_Key = "read_cell_data";
  static inline constexpr StringLiteral k_CreatedVertexGeometryPath_Key = "output_vertex_geometry_path";
  static inline constexpr StringLiteral k_CreatedImageGeometryPath_Key = "output_image_geometry_path";
  static inline constexpr StringLiteral k_VertexAttributeMatrixName_Key = "vertex_attribute_matrix_name";
  static inline constexpr StringLiteral k_CellAttributeMatrixName_Key = "cell_attribute_matrix_name";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ReadVtkStructuredPointsFilter, "60d55662-ca7a-4be7-b0a0-ed6e785eb51b");
/* LEGACY UUID FOR THIS FILTER f2fecbf9-636b-5ef9-89db-5cb57e4c5676 */
