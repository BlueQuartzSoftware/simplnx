#pragma once

#include <functional>
#include <optional>
#include <string>

#include <nonstd/expected.hpp>

#include "complex/Common/Result.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/Arguments.hpp"
#include "complex/Filter/Output.hpp"
#include "complex/Filter/Parameters.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
class IFilter
{
public:
  struct Message
  {
    enum class Type : u8
    {
      Info = 0,
      Debug,
      Warning,
      Error
    };

    Type type = Type::Info;
    std::string message;
  };

  using MessageHandler = std::function<void(const Message&)>;

  struct DataCheckResult
  {
    nonstd::expected<OutputActions, std::vector<Error>> result;
    std::vector<Warning> warnings;

    [[nodiscard]] bool hasErrors() const
    {
      return !result.has_value();
    }
  };

  struct ExecuteResult
  {
    std::vector<Error> errors;
    std::vector<Warning> warnings;

    static ExecuteResult makeExecuteResult(const DataCheckResult& dataCheckResult)
    {
      ExecuteResult result{dataCheckResult.result.error(), dataCheckResult.warnings};
      return result;
    }

    static ExecuteResult makeExecuteResult(DataCheckResult&& dataCheckResult)
    {
      ExecuteResult result{std::move(dataCheckResult.result.error()), std::move(dataCheckResult.warnings)};
      return result;
    }

    [[nodiscard]] bool hasErrors() const
    {
      return !errors.empty();
    }
  };

  virtual ~IFilter() noexcept = default;

  IFilter(const IFilter&) = delete;
  IFilter(IFilter&&) = delete;

  IFilter& operator=(const IFilter&) = delete;
  IFilter& operator=(IFilter&&) = delete;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] virtual std::string name() const = 0;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] virtual std::string uuid() const = 0;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] virtual std::string humanName() const = 0;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] virtual Parameters parameters() const = 0;

  /**
   * @brief
   * @param data
   * @param args
   * @param messageHandler
   * @return
   */
  DataCheckResult dataCheck(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler = {}) const;

  /**
   * @brief
   * @param data
   * @param args
   * @param messageHandler
   * @return
   */
  ExecuteResult execute(DataStructure& data, const Arguments& args, const MessageHandler& messageHandler = {});

  /**
   * @brief
   * @param args
   * @return
   */
  [[nodiscard]] nlohmann::json toJson(const Arguments& args) const;

  /**
   * @brief
   * @param json
   * @return
   */
  [[nodiscard]] nonstd::expected<Arguments, Result> fromJson(const nlohmann::json& json) const;

protected:
  IFilter() = default;

private:
  /**
   * @brief
   * @param data
   * @param args
   * @param messageHandler
   * @return
   */
  virtual DataCheckResult dataCheckImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const = 0;

  /**
   * @brief
   * @param data
   * @param args
   * @param messageHandler
   * @return
   */
  virtual ExecuteResult executeImpl(DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) = 0;
};
} // namespace complex
