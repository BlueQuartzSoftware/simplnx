#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Uuid.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/MessageHandler.hpp"
#include "simplnx/Filter/Output.hpp"
#include "simplnx/Filter/Parameters.hpp"
#include "simplnx/simplnx_export.hpp"

#include <nonstd/expected.hpp>

#include <atomic>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace nx::core
{
class PipelineFilter;

/**
 * @class IFilter
 * @brief IFilter is the interface for filters providing access to both metadata (e.g. name, uuid, etc.)
 * and the algorithm itself (i.e. preflight/execute).
 */
class SIMPLNX_EXPORT IFilter
{
public:
  using UniquePointer = std::unique_ptr<IFilter>;
  using VersionType = uint64;

  // Backward-compatible type aliases — all existing code using IFilter::Message,
  // IFilter::ProgressMessage, IFilter::MessageHandler continues to compile unchanged.
  using Message = nx::core::Message;
  using ProgressMessage = nx::core::ProgressMessage;
  using MessageHandler = nx::core::MessageHandler;

  /**
   * @brief Represents an output value from preflight.
   */
  struct PreflightValue
  {
    std::string name;
    std::string value;
  };

  /**
   * @brief Result of the preflight operation including output actions and values.
   */
  struct PreflightResult
  {
    Result<OutputActions> outputActions;
    std::vector<PreflightValue> outputValues;
  };

  /**
   * @brief Creates a preflight error result with the given error code and message.
   * @param errorCode The error code
   * @param errorMessage The error message
   * @return PreflightResult containing the error
   */
  static PreflightResult MakePreflightErrorResult(int32 errorCode, const std::string& errorMessage)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{errorCode, errorMessage}})};
  }

  /**
   * @brief Result of the execute operation including any errors and output values.
   */
  struct ExecuteResult
  {
    Result<> result;
    std::vector<PreflightValue> outputValues;
  };

  virtual ~IFilter() noexcept;

  IFilter(const IFilter&) = delete;
  IFilter(IFilter&&) noexcept = delete;

  IFilter& operator=(const IFilter&) = delete;
  IFilter& operator=(IFilter&&) noexcept = delete;

  /**
   * @brief Returns the name of the filter.
   * @return std::string
   */
  virtual std::string name() const = 0;

  /**
   * @brief Returns the C++ classname of this filter.
   * @return std::string
   */
  virtual std::string className() const = 0;

  /**
   * @brief Returns the uuid of the filter.
   * @return Uuid
   */
  virtual Uuid uuid() const = 0;

  /**
   * @brief Returns the human readable name of the filter.
   * @return std::string
   */
  virtual std::string humanName() const = 0;

  /**
   * @brief Returns the default tags for this filter.
   * @return std::vector<std::string>
   */
  virtual std::vector<std::string> defaultTags() const;

  /**
   * @brief Returns the parameters of the filter (i.e. its inputs)
   * @return Parameters
   */
  virtual Parameters parameters() const = 0;

  /**
   * @brief Returns parameters version integer.
   * The Initial version should always be 1.
   * Should be incremented everytime the parameters change.
   * @return VersionType
   */
  virtual VersionType parametersVersion() const = 0;

  /**
   * @brief Returns a copy of the filter.
   * @return UniquePointer
   */
  virtual UniquePointer clone() const = 0;

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
                            const ExecutionContext& executionContext = ExecutionContext()) const;

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
                        const std::atomic_bool& shouldCancel = false, const ExecutionContext& executionContext = ExecutionContext()) const;

  /**
   * @brief Converts the given arguments to a JSON representation using the filter's parameters.
   * @param args
   * @return nlohmann::json
   */
  virtual nlohmann::json toJson(const Arguments& args) const;

  /**
   * @brief Converts JSON to arguments based on the filter's parameters.
   * @param json
   * @return Result<Arguments>
   */
  Result<Arguments> fromJson(const nlohmann::json& json) const;

  /**
   * @brief Returns the set of default arguments for this filter.
   * @return Arguments
   */
  Arguments getDefaultArguments() const;

protected:
  /**
   * @brief Protected default constructor.
   */
  IFilter() = default;

  /**
   * @brief Classes that implement IFilter must provide this function for preflight.
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
   * @brief Classes that implement IFilter must provide this function for execute.
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

using FilterCreationFunc = std::function<IFilter::UniquePointer()>;
} // namespace nx::core
