#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ReadAngDataFilter
 * @brief Reads one .ang file into a new Image Geometry.
 *
 * The filter provides image data directly without an intermediate .h5ebsd file.
 */
class ORIENTATIONANALYSIS_EXPORT ReadAngDataFilter : public IFilter
{
public:
  ReadAngDataFilter() = default;
  ~ReadAngDataFilter() noexcept override = default;

  ReadAngDataFilter(const ReadAngDataFilter&) = delete;
  ReadAngDataFilter(ReadAngDataFilter&&) noexcept = delete;

  ReadAngDataFilter& operator=(const ReadAngDataFilter&) = delete;
  ReadAngDataFilter& operator=(ReadAngDataFilter&&) noexcept = delete;

  // Parameter Keys
  static constexpr StringLiteral k_InputFile_Key = "input_file";
  static constexpr StringLiteral k_CreatedImageGeometryPath_Key = "output_image_geometry_path";
  static constexpr StringLiteral k_CellAttributeMatrixName_Key = "cell_attribute_matrix_name";
  static constexpr StringLiteral k_CellEnsembleAttributeMatrixName_Key = "cell_ensemble_attribute_matrix_name";

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
   * @brief Returns the human-readable name of the filter.
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
   * The Initial version should always be 1.
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
   * @brief Validates arguments and prepares actions without changing the DataStructure.
   * @param dataStructure Input DataStructure.
   * @param filterArgs Filter parameter values.
   * @param messageHandler Receives progress messages.
   * @param shouldCancel Cancellation flag.
   * @param executionContext Resolves relative paths.
   * @return Preflight actions, warnings, and errors.
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                const ExecutionContext& executionContext) const override;

  /**
   * @brief Reads the .ang file into the configured Image Geometry.
   * @param dataStructure DataStructure to update.
   * @param filterArgs Filter parameter values.
   * @param pipelineNode Optional pipeline node.
   * @param messageHandler Receives progress messages.
   * @param shouldCancel Cancellation flag.
   * @param executionContext Resolves relative paths.
   * @return Execution warnings and errors. A failure can leave partial output.
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                       const ExecutionContext& executionContext) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ReadAngDataFilter, "5b062816-79ac-47ce-93cb-e7966896bcbd");
