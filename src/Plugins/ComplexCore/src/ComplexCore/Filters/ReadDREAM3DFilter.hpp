#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Filter/Arguments.hpp"
#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Filter/Parameters.hpp"

namespace complex
{
/**
 * @class ReadDREAM3DFilter
 * @brief The ReadDREAM3DFilter is an IFilter class designed to import data
 * from a target DREAM.3D file.
 */
class COMPLEXCORE_EXPORT ReadDREAM3DFilter : public IFilter
{
public:
  ReadDREAM3DFilter() = default;
  ~ReadDREAM3DFilter() noexcept override = default;

  ReadDREAM3DFilter(const ReadDREAM3DFilter&) = delete;
  ReadDREAM3DFilter(ReadDREAM3DFilter&&) noexcept = delete;

  ReadDREAM3DFilter& operator=(const ReadDREAM3DFilter&) = delete;
  ReadDREAM3DFilter& operator=(ReadDREAM3DFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_ImportFileData = "import_file_data";

  /**
   * @brief Reads SIMPL json and converts it complex Arguments.
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
   * @brief Returns the ReadDREAM3DFilter class's UUID.
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

  /**
   * @brief Converts the given arguments to a JSON representation using the filter's parameters.
   * @param args
   * @return nlohmann::json
   */
  nlohmann::json toJson(const Arguments& args) const override;

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
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, ReadDREAM3DFilter, "0dbd31c7-19e0-4077-83ef-f4a6459a0e2d");
