#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Uuid.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/Arguments.hpp"
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

  /**
   * @brief Represents a message from a filter during execution.
   */
  struct Message
  {
    /**
     * @brief Message type enumeration.
     */
    enum class Type : uint8
    {
      Info = 0,
      Debug,
      Progress,
      Warning,
      Error
    };

    Type type = Type::Info;
    std::string message;

    /**
     * @brief Percent complete, clamped to [0, 100], for driving a progress bar. A value of -1 means
     * this message carries no progress value. The human-readable form of the progress lives in
     * `message`, which is already rendered by the sender; this field exists only for consumers that
     * need a number.
     */
    int32 progress = -1;
  };

  /**
   * @brief Retained as an alias so existing aggregate initializations keep compiling. Progress
   * information now lives on Message itself.
   */
  using ProgressMessage = Message;

  /**
   * @brief Handler for processing filter messages during execution.
   */
  struct MessageHandler
  {
    using Callback = std::function<void(const Message&)>;

    /**
     * @brief Sends a message.
     * @param message The message to send
     */
    void sendMessage(const Message& message) const
    {
      if(m_Callback)
      {
        m_Callback(message);
      }
    }

    /**
     * @brief Sends a message of the given type.
     * @param type The message type
     * @param message The message text
     */
    void sendMessage(Message::Type type, std::string message) const
    {
      sendMessage(Message{type, std::move(message)});
    }

    /**
     * @brief Sends an informational message.
     * @param message The message text
     */
    void sendInfoMessage(std::string message) const
    {
      sendMessage(Message{Message::Type::Info, std::move(message)});
    }

    /**
     * @brief Sends a debug message.
     * @param message The message text
     */
    void sendDebugMessage(std::string message) const
    {
      sendMessage(Message{Message::Type::Debug, std::move(message)});
    }

    /**
     * @brief Sends a warning message.
     * @param message The message text
     */
    void sendWarningMessage(std::string message) const
    {
      sendMessage(Message{Message::Type::Warning, std::move(message)});
    }

    /**
     * @brief Sends an error message.
     * @param message The message text
     */
    void sendErrorMessage(std::string message) const
    {
      sendMessage(Message{Message::Type::Error, std::move(message)});
    }

    /**
     * @brief Sends a progress message whose text has already been rendered by the caller. Prefer
     * sendProgressCount() or sendProgressPercent(), which record what the sender intended to
     * display; reach for this only when neither form fits.
     * @param message The fully rendered message text
     * @param percent Percent complete for the progress bar, clamped to [0, 100]
     */
    void sendProgressMessage(std::string message, int32 percent) const;

    /**
     * @brief Sends progress as a count of completed items, rendered as "<label>: <current>/<max>".
     * Use this when the counts are meaningful to a user, e.g. tuples or slices.
     * @param label Describes the work being done, with no trailing punctuation
     * @param current Items completed so far
     * @param max Total items
     */
    void sendProgressCount(std::string label, usize current, usize max) const;

    /**
     * @brief Sends progress as a percentage, rendered as "<label>: <percent>%". Use this when the
     * counts are too large to be readable, where a percentage with a decimal place or two conveys
     * more than the raw numbers would.
     * @param label Describes the work being done, with no trailing punctuation
     * @param current Items completed so far
     * @param max Total items
     * @param decimals Number of decimal places to display
     */
    void sendProgressPercent(std::string label, usize current, usize max, int32 decimals = 2) const;

    /**
     * @brief Invokes the callback with a message.
     * @param message The message to send
     */
    void operator()(const Message& message) const
    {
      sendMessage(message);
    }

    /**
     * @brief Invokes the callback with an info message.
     * @param message The message text
     */
    void operator()(std::string message) const
    {
      operator()(Message{Message::Type::Info, std::move(message)});
    }

    /**
     * @brief Invokes the callback with a typed message.
     * @param type The message type
     * @param message The message text
     */
    void operator()(Message::Type type, std::string message) const
    {
      operator()(Message{type, std::move(message)});
    }

    /**
     * @brief Invokes the callback with a progress message.
     * @param type The message type
     * @param message The message text
     * @param progress The progress value
     */
    void operator()(Message::Type type, std::string message, int32 progress) const
    {
      operator()(ProgressMessage{type, std::move(message), progress});
    }
    Callback m_Callback;
  };

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
  virtual Result<Arguments> fromJson(const nlohmann::json& json) const;

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
