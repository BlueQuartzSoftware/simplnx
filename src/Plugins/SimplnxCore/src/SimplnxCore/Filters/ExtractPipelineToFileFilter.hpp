#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Filter/Parameters.hpp"

namespace nx::core
{
/**
 * @class ExtractPipelineToFileFilter
 * @brief The ExtractPipelineToFileFilter is an IFilter class designed to extract the pipeline data
 * from a target DREAM.3D file and export it out to it's own file.
 */
class SIMPLNXCORE_EXPORT ExtractPipelineToFileFilter : public IFilter
{
public:
  ExtractPipelineToFileFilter() = default;
  ~ExtractPipelineToFileFilter() noexcept override = default;

  ExtractPipelineToFileFilter(const ExtractPipelineToFileFilter&) = delete;
  ExtractPipelineToFileFilter(ExtractPipelineToFileFilter&&) noexcept = delete;

  ExtractPipelineToFileFilter& operator=(const ExtractPipelineToFileFilter&) = delete;
  ExtractPipelineToFileFilter& operator=(ExtractPipelineToFileFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_ImportFileData = "import_file_data";
  static inline constexpr StringLiteral k_OutputFile = "output_file";

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
   * @return std::string
   */
  std::string className() const override;

  /**
   * @brief Returns the ExtractPipelineToFileFilter class's UUID.
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
   * @brief Returns a copy of the filter as a std::unique_ptr.
   * @return UniquePointer
   */
  UniquePointer clone() const override;

protected:
  /**
   * @brief Classes that implement IFilter must provide this function for preflight.
   * Runs after the filter runs the checks in its parameters.
   * @param dataStructure
   * @param args
   * @param messageHandler
   * @return Result<OutputActions>
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief Classes that implement IFilter must provide this function for execute.
   * Runs after the filter applies the OutputActions from preflight.
   * @param dataStructure
   * @param args
   * @param pipelineNode
   * @param messageHandler
   * @return Result<>
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                       const std::atomic_bool& shouldCancel) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ExtractPipelineToFileFilter, "27203c99-9975-4528-88cc-42b270165d79");
