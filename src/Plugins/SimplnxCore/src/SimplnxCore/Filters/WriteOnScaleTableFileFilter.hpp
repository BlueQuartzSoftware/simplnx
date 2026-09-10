#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class WriteOnScaleTableFileFilter
 * @brief Writes grid coordinates, phase names, and feature IDs to an OnScale table file.
 */
class SIMPLNXCORE_EXPORT WriteOnScaleTableFileFilter : public IFilter
{
public:
  WriteOnScaleTableFileFilter() = default;
  ~WriteOnScaleTableFileFilter() noexcept override = default;

  WriteOnScaleTableFileFilter(const WriteOnScaleTableFileFilter&) = delete;
  WriteOnScaleTableFileFilter(WriteOnScaleTableFileFilter&&) noexcept = delete;

  WriteOnScaleTableFileFilter& operator=(const WriteOnScaleTableFileFilter&) = delete;
  WriteOnScaleTableFileFilter& operator=(WriteOnScaleTableFileFilter&&) noexcept = delete;

  // Parameter Keys
  static constexpr StringLiteral k_OutputPath_Key = "output_path";
  static constexpr StringLiteral k_FilePrefix_Key = "file_prefix";
  static constexpr StringLiteral k_NumKeypoints_Key = "num_keypoints";
  static constexpr StringLiteral k_InputGeometryPath_Key = "input_geometry_path";
  static constexpr StringLiteral k_FeatureIdsArrayPath_Key = "feature_ids_array_path";
  static constexpr StringLiteral k_PhaseNamesArrayPath_Key = "phase_names_array_path";

  /**
   * @brief Converts a legacy ExportOnScaleTableFile JSON object to filter arguments.
   * @param json Legacy filter JSON object.
   * @return Converted arguments or conversion errors.
   */
  static Result<Arguments> FromSIMPLJson(const nlohmann::json& json);

  /**
   * @brief Returns the filter name.
   * @return Internal filter name.
   */
  std::string name() const override;

  /**
   * @brief Returns the C++ class name.
   * @return Filter class name.
   */
  std::string className() const override;

  /**
   * @brief Returns the filter UUID.
   * @return Filter UUID.
   */
  Uuid uuid() const override;

  /**
   * @brief Returns the user-visible filter name.
   * @return Human-readable filter name.
   */
  std::string humanName() const override;

  /**
   * @brief Returns the filter tags.
   * @return Tags that describe the filter.
   */
  std::vector<std::string> defaultTags() const override;

  /**
   * @brief Returns the filter parameters.
   * @return Parameter definitions in display order.
   */
  Parameters parameters() const override;

  /**
   * @brief Returns the parameter schema version.
   * @return Parameter schema version.
   */
  VersionType parametersVersion() const override;

  /**
   * @brief Creates a new filter instance.
   * @return New filter instance.
   */
  UniquePointer clone() const override;

protected:
  /**
   * @brief Validates the output directory, geometry dimensions, and selected arrays.
   * @param dataStructure Contains the input geometry and arrays.
   * @param filterArgs Contains the parameter values.
   * @param messageHandler Receives filter messages.
   * @param shouldCancel Indicates that execution must stop.
   * @param executionContext Resolves relative paths.
   * @return Empty output actions or validation errors.
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                const ExecutionContext& executionContext) const override;

  /**
   * @brief Writes the OnScale table file.
   * @param dataStructure Contains the input geometry and arrays.
   * @param filterArgs Contains the parameter values.
   * @param pipelineNode Identifies the executing pipeline node.
   * @param messageHandler Receives filter messages.
   * @param shouldCancel Indicates that execution must stop.
   * @param executionContext Resolves relative paths.
   * @return Success, cancellation, or an export error.
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                       const ExecutionContext& executionContext) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, WriteOnScaleTableFileFilter, "494a4c6d-01c9-4f55-a0d4-32ee6e67c329");
/* LEGACY UUID FOR THIS FILTER 8efc447d-1c92-5ec5-885c-60b4a597835c */
