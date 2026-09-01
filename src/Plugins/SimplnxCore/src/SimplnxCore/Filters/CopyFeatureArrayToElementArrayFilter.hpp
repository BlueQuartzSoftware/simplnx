#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class CopyFeatureArrayToElementArrayFilter
 * @brief Copies selected feature tuples to the cells that reference each feature.
 *
 * XDMF visualization uses cell attributes. This filter projects feature-level
 * values to cell-level arrays for spatial display.
 */
class SIMPLNXCORE_EXPORT CopyFeatureArrayToElementArrayFilter : public IFilter
{
public:
  CopyFeatureArrayToElementArrayFilter() = default;
  ~CopyFeatureArrayToElementArrayFilter() noexcept override = default;

  CopyFeatureArrayToElementArrayFilter(const CopyFeatureArrayToElementArrayFilter&) = delete;
  CopyFeatureArrayToElementArrayFilter(CopyFeatureArrayToElementArrayFilter&&) noexcept = delete;

  CopyFeatureArrayToElementArrayFilter& operator=(const CopyFeatureArrayToElementArrayFilter&) = delete;
  CopyFeatureArrayToElementArrayFilter& operator=(CopyFeatureArrayToElementArrayFilter&&) noexcept = delete;

  // Parameter Keys
  static constexpr StringLiteral k_SelectedFeatureArrayPaths_Key = "selected_feature_array_paths";
  static constexpr StringLiteral k_CellFeatureIdsArrayPath_Key = "feature_ids_path";
  static constexpr StringLiteral k_CreatedArraySuffix_Key = "created_array_suffix";

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
   * @brief Projects selected feature arrays to cell arrays.
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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, CopyFeatureArrayToElementArrayFilter, "4c8c976a-993d-438b-bd8e-99f71114b9a1");
