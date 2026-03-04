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
 * @brief IFilter is the pure virtual interface for all filters, providing the
 * public API contract for metadata (name, uuid, etc.) and algorithm execution
 * (preflight/execute). All nested types that form the public API are defined here.
 */
class SIMPLNX_EXPORT IFilter
{
public:
  using UniquePointer = std::unique_ptr<IFilter>;
  using VersionType = uint64;

  struct Message
  {
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
  };

  struct ProgressMessage : public Message
  {
    int32 progress = 0;
  };

  struct MessageHandler
  {
    using Callback = std::function<void(const Message&)>;

    void operator()(const Message& message) const
    {
      if(m_Callback)
      {
        m_Callback(message);
      }
    }
    void operator()(std::string message) const
    {
      operator()(Message{Message::Type::Info, std::move(message)});
    }
    void operator()(Message::Type type, std::string message) const
    {
      operator()(Message{type, std::move(message)});
    }
    void operator()(Message::Type type, std::string message, int32 progress) const
    {
      operator()(ProgressMessage{type, std::move(message), progress});
    }
    Callback m_Callback;
  };

  struct PreflightValue
  {
    std::string name;
    std::string value;
  };

  struct PreflightResult
  {
    Result<OutputActions> outputActions;
    std::vector<PreflightValue> outputValues;
  };

  static PreflightResult MakePreflightErrorResult(int32 errorCode, const std::string& errorMessage)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{errorCode, errorMessage}})};
  }

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
  virtual std::vector<std::string> defaultTags() const = 0;

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
  virtual PreflightResult preflight(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler = {}, const std::atomic_bool& shouldCancel = false,
                                    const ExecutionContext& executionContext = ExecutionContext()) const = 0;

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
  virtual ExecuteResult execute(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode = nullptr, const MessageHandler& messageHandler = {},
                                const std::atomic_bool& shouldCancel = false, const ExecutionContext& executionContext = ExecutionContext()) const = 0;

  /**
   * @brief Converts the given arguments to a JSON representation using the filter's parameters.
   * @param args
   * @return nlohmann::json
   */
  virtual nlohmann::json toJson(const Arguments& args) const = 0;

  /**
   * @brief Converts JSON to arguments based on the filter's parameters.
   * @param json
   * @return Result<Arguments>
   */
  virtual Result<Arguments> fromJson(const nlohmann::json& json) const = 0;

  /**
   * @brief Returns the set of default arguments for this filter.
   * @return Arguments
   */
  virtual Arguments getDefaultArguments() const = 0;

protected:
  IFilter() = default;
};

using FilterCreationFunc = std::function<IFilter::UniquePointer()>;
} // namespace nx::core
