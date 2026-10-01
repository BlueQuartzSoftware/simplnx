#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ReadOnScaleTableFileFilter
 * @brief Imports an OnScale table file as a Rectilinear Grid Geometry and material data.
 */
class SIMPLNXCORE_EXPORT ReadOnScaleTableFileFilter : public IFilter
{
public:
  /** @brief Constructs a filter with an independent preflight-header cache entry. */
  ReadOnScaleTableFileFilter();
  ~ReadOnScaleTableFileFilter() noexcept override;

  ReadOnScaleTableFileFilter(const ReadOnScaleTableFileFilter&) = delete;
  ReadOnScaleTableFileFilter(ReadOnScaleTableFileFilter&&) noexcept = delete;
  ReadOnScaleTableFileFilter& operator=(const ReadOnScaleTableFileFilter&) = delete;
  ReadOnScaleTableFileFilter& operator=(ReadOnScaleTableFileFilter&&) noexcept = delete;

  // Parameter Keys
  static constexpr StringLiteral k_InputFile_Key = "input_file";
  static constexpr StringLiteral k_Origin_Key = "fallback_origin";
  static constexpr StringLiteral k_Spacing_Key = "fallback_spacing";
  static constexpr StringLiteral k_CreatedRectGridGeometryPath_Key = "output_rect_grid_geometry_path";
  static constexpr StringLiteral k_CellAttributeMatrixName_Key = "cell_attribute_matrix_name";
  static constexpr StringLiteral k_FeatureIdsArrayName_Key = "feature_ids_array_name";
  static constexpr StringLiteral k_PhaseAttributeMatrixName_Key = "phase_attribute_matrix_name";
  static constexpr StringLiteral k_MaterialNamesArrayName_Key = "material_names_array_name";

  /**
   * @brief Converts a legacy ImportOnScaleTableFile JSON object to filter arguments.
   * @param json Legacy filter JSON object.
   * @return Converted arguments or conversion errors.
   */
  static Result<Arguments> FromSIMPLJson(const nlohmann::json& json);

  /** @brief Returns the internal filter name. @return Internal filter name. */
  std::string name() const override;
  /** @brief Returns the C++ class name. @return Filter class name. */
  std::string className() const override;
  /** @brief Returns the filter UUID. @return Filter UUID. */
  Uuid uuid() const override;
  /** @brief Returns the user-visible filter name. @return Human-readable filter name. */
  std::string humanName() const override;
  /** @brief Returns the filter tags. @return Tags that describe the filter. */
  std::vector<std::string> defaultTags() const override;
  /** @brief Returns the filter parameters. @return Parameter definitions in display order. */
  Parameters parameters() const override;
  /** @brief Returns the parameter schema version. @return Parameter schema version. */
  VersionType parametersVersion() const override;
  /** @brief Creates a new filter instance. @return New filter instance. */
  UniquePointer clone() const override;

protected:
  /**
   * @brief Reads the cached file header and declares the imported geometry and arrays.
   * @param dataStructure Contains existing objects that validate creation paths.
   * @param filterArgs Contains the parameter values.
   * @param messageHandler Receives filter messages.
   * @param shouldCancel Indicates that preflight must stop.
   * @param executionContext Resolves relative paths.
   * @return Creation actions, geometry information, or a file-format error.
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                const ExecutionContext& executionContext) const override;

  /**
   * @brief Reads the OnScale table data into the created outputs.
   * @param dataStructure Contains the created geometry and arrays.
   * @param filterArgs Contains the parameter values.
   * @param pipelineNode Identifies the executing pipeline node.
   * @param messageHandler Receives filter messages.
   * @param shouldCancel Indicates that execution must stop.
   * @param executionContext Resolves relative paths.
   * @return Success, cancellation, or a file-format error.
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                       const ExecutionContext& executionContext) const override;

private:
  int32 m_InstanceId;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ReadOnScaleTableFileFilter, "232dc909-d77f-4e62-8fd9-1ff0442b5709");
/* LEGACY UUID FOR THIS FILTER 06dd6e66-84fb-5170-a923-d925dc39bb94 */
