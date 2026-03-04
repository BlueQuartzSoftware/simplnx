#pragma once

#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class AbstractFilter
 * @brief AbstractFilter provides the concrete template-method implementations
 * of the IFilter interface: parameter validation, preflight orchestration,
 * execution pipeline, JSON serialization, and default arguments.
 * Concrete filters inherit from AbstractFilter and implement preflightImpl()
 * and executeImpl().
 */
class SIMPLNX_EXPORT AbstractFilter : public IFilter
{
public:
  ~AbstractFilter() noexcept override;

  AbstractFilter(const AbstractFilter&) = delete;
  AbstractFilter(AbstractFilter&&) noexcept = delete;

  AbstractFilter& operator=(const AbstractFilter&) = delete;
  AbstractFilter& operator=(AbstractFilter&&) noexcept = delete;

  /**
   * @brief Returns the default tags for this filter.
   * @return std::vector<std::string>
   */
  std::vector<std::string> defaultTags() const override;

  /**
   * @brief Takes in a DataStructure and checks that the filter can be run on it with the given arguments.
   * Returns any warnings/errors. Also returns the changes that would be applied to the DataStructure.
   * Some parts of the actions may not be completely filled out if all the required information is not available at preflight time.
   * @param data
   * @param args
   * @param messageHandler
   * @param shouldCancel
   * @return PreflightResult
   */
  PreflightResult preflight(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler = {}, const std::atomic_bool& shouldCancel = false,
                            const ExecutionContext& executionContext = ExecutionContext()) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param dataStructure
   * @param args
   * @param pipelineNode = nullptr
   * @param messageHandler = {}
   * @param shouldCancel
   * @return ExecuteResult
   */
  ExecuteResult execute(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode = nullptr, const MessageHandler& messageHandler = {},
                        const std::atomic_bool& shouldCancel = false, const ExecutionContext& executionContext = ExecutionContext()) const override;

  /**
   * @brief Converts the given arguments to a JSON representation using the filter's parameters.
   * @param args
   * @return nlohmann::json
   */
  nlohmann::json toJson(const Arguments& args) const override;

  /**
   * @brief Converts JSON to arguments based on the filter's parameters.
   * @param json
   * @return Result<Arguments>
   */
  Result<Arguments> fromJson(const nlohmann::json& json) const override;

  /**
   * @brief Returns the set of default arguments for this filter.
   * @return Arguments
   */
  Arguments getDefaultArguments() const override;

protected:
  AbstractFilter() = default;

  /**
   * @brief Classes that implement AbstractFilter must provide this function for preflight.
   * Runs after the filter runs the checks in its parameters.
   * @param data
   * @param args
   * @param messageHandler
   * @param shouldCancel
   * @return PreflightResult
   */
  virtual PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                        const ExecutionContext& executionContext) const = 0;

  /**
   * @brief Classes that implement AbstractFilter must provide this function for execute.
   * Runs after the filter applies the OutputActions from preflight.
   * @param data
   * @param args
   * @param pipelineNode
   * @param messageHandler
   * @param shouldCancel
   * @return Result<>
   */
  virtual Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                               const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const = 0;
};
} // namespace nx::core
