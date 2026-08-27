#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
class ORIENTATIONANALYSIS_EXPORT ReadEbsdPatternFileFilter : public IFilter
{
public:
  ReadEbsdPatternFileFilter() = default;
  ~ReadEbsdPatternFileFilter() noexcept override = default;

  ReadEbsdPatternFileFilter(const ReadEbsdPatternFileFilter&) = delete;
  ReadEbsdPatternFileFilter(ReadEbsdPatternFileFilter&&) noexcept = delete;
  ReadEbsdPatternFileFilter& operator=(const ReadEbsdPatternFileFilter&) = delete;
  ReadEbsdPatternFileFilter& operator=(ReadEbsdPatternFileFilter&&) noexcept = delete;

  static constexpr StringLiteral k_InputFile_Key = "input_file";
  static constexpr StringLiteral k_SetScanDimensions_Key = "set_scan_dimensions";
  static constexpr StringLiteral k_NumberOfRows_Key = "number_of_rows";
  static constexpr StringLiteral k_NumberOfColumns_Key = "number_of_columns";
  static constexpr StringLiteral k_OutputArrayPath_Key = "output_array_path";

  static Result<Arguments> FromSIMPLJson(const nlohmann::json& json);

  std::string name() const override;
  std::string className() const override;
  Uuid uuid() const override;
  std::string humanName() const override;
  std::vector<std::string> defaultTags() const override;
  Parameters parameters() const override;
  VersionType parametersVersion() const override;
  UniquePointer clone() const override;

protected:
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                const ExecutionContext& executionContext) const override;
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                       const ExecutionContext& executionContext) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ReadEbsdPatternFileFilter, "cd975a45-53de-4ace-ac16-d99638a44d6a");
