#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/AbstractFilter.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/Parameters.hpp"

namespace nx::core
{
/**
 * @class WriteDREAM3DFilter
 * @brief The WriteDREAM3DFilter is an IFilter class designed to export the
 * DataStructure to a target HDF5 file.
 */
class SIMPLNXCORE_EXPORT WriteDREAM3DFilter : public AbstractFilter
{
public:
  WriteDREAM3DFilter() = default;
  ~WriteDREAM3DFilter() noexcept override = default;

  WriteDREAM3DFilter(const WriteDREAM3DFilter&) = delete;
  WriteDREAM3DFilter(WriteDREAM3DFilter&&) noexcept = delete;

  WriteDREAM3DFilter& operator=(const WriteDREAM3DFilter&) = delete;
  WriteDREAM3DFilter& operator=(WriteDREAM3DFilter&&) noexcept = delete;

  // Parameter Keys
  static constexpr StringLiteral k_ExportFilePath = "export_file_path";
  static constexpr StringLiteral k_WriteXdmf = "write_xdmf_file";

  /**
   * @brief Reads SIMPL json and converts it simplnx Arguments.
   * @param json
   * @return Result<Arguments>
   */
  static Result<Arguments> FromSIMPLJson(const nlohmann::json& json);

  /**
   * @brief Returns the name of the filter class.
   * @return std::string
   */
  std::string name() const override;

  /**
   * @brief Returns the C++ classname of this filter.
   * @return
   */
  std::string className() const override;

  /**
   * @brief Returns the WriteDREAM3DFilter class's UUID.
   * @return Uuid
   */
  Uuid uuid() const override;

  /**
   * @brief Returns the human readable name of the filter.
   * @return std::string
   */
  std::string humanName() const override;

  /**
   * @brief Returns the default tags for this filter.
   * @return
   */
  std::vector<std::string> defaultTags() const override;

  /**
   * @brief Returns a collection of the filter's parameters (i.e. its inputs)
   * @return Parameters
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
   * @brief Returns a copy of the filter as a std::unique_ptr.
   * @return UniquePointer
   */
  UniquePointer clone() const override;

protected:
  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param pipelineNode The node in the pipeline that is being executed
   * @param messageHandler The MessageHandler object
   * @param shouldCancel Atomic boolean value that can be checked to cancel the filter
   * @param executionContext The ExecutionContext that can be used to determine the correct absolute path from a relative path
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                const ExecutionContext& executionContext) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param pipelineNode The node in the pipeline that is being executed
   * @param messageHandler The MessageHandler object
   * @param shouldCancel Atomic boolean value that can be checked to cancel the filter
   * @param executionContext The ExecutionContext that can be used to determine the correct absolute path from a relative path
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                       const ExecutionContext& executionContext) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, WriteDREAM3DFilter, "b3a95784-2ced-41ec-8d3d-0242ac130003");
