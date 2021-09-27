#pragma once

#include <functional>
#include <optional>
#include <string>

#include <nonstd/expected.hpp>

#include "complex/Common/Result.hpp"
#include "complex/Common/Uuid.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/Arguments.hpp"
#include "complex/Filter/Output.hpp"
#include "complex/Filter/Parameters.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @class IFilter
 * @brief IFilter is the interface for filters providing access to both metadata (e.g. name, uuid, etc.)
 * and the algorithm itself (i.e. preflight/execute).
 */
class COMPLEX_EXPORT IFilter
{
public:
  using UniquePointer = std::unique_ptr<IFilter>;

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
    void operator()(const std::string& message) const
    {
      operator()(Message{Message::Type::Info, message});
    }
    void operator()(Message::Type type, const std::string& message) const
    {
      operator()(Message{type, message});
    }

    Callback m_Callback;
  };

  virtual ~IFilter() noexcept;

  IFilter(const IFilter&) = delete;
  IFilter(IFilter&&) noexcept = delete;

  IFilter& operator=(const IFilter&) = delete;
  IFilter& operator=(IFilter&&) noexcept = delete;

  /**
   * @brief Returns the name of the filter.
   * @return
   */
  [[nodiscard]] virtual std::string name() const = 0;

  /**
   * @brief Returns the uuid of the filter.
   * @return
   */
  [[nodiscard]] virtual Uuid uuid() const = 0;

  /**
   * @brief Returns the human readable name of the filter.
   * @return
   */
  [[nodiscard]] virtual std::string humanName() const = 0;

  /**
   * @brief Returns the parameters of the filter (i.e. its inputs)
   * @return
   */
  [[nodiscard]] virtual Parameters parameters() const = 0;

  /**
   * @brief Returns a copy of the filter.
   * @return
   */
  [[nodiscard]] virtual UniquePointer clone() const = 0;

  /**
   * @brief Takes in a DataStructure and checks that the filter can be run on it with the given arguments.
   * Returns any warnings/errors. Also returns the changes that would be applied to the DataStructure.
   * Some parts of the actions may not be completely filled out if all the required information is not available at preflight time.
   * @param data
   * @param args
   * @param messageHandler
   * @return
   */
  Result<OutputActions> preflight(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler = {}) const;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarentee that the DataStructure is in a correct state.
   * @param data
   * @param args
   * @param messageHandler
   * @return
   */
  Result<> execute(DataStructure& data, const Arguments& args, const MessageHandler& messageHandler = {}) const;

  /**
   * @brief Converts the given arguments to a JSON representation using the filter's parameters.
   * @param args
   * @return
   */
  [[nodiscard]] nlohmann::json toJson(const Arguments& args) const;

  /**
   * @brief Converts JSON to arguments based on the filter's parameters.
   * @param json
   * @return
   */
  [[nodiscard]] Result<Arguments> fromJson(const nlohmann::json& json) const;

protected:
  IFilter() = default;

  /**
   * @brief Classes that implement IFilter must provide this function for preflight.
   * Runs after the filter runs the checks in its parameters.
   * @param data
   * @param args
   * @param messageHandler
   * @return
   */
  virtual Result<OutputActions> preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler = {}) const = 0;

  /**
   * @brief Classes that implement IFilter must provide this function for execute.
   * Runs after the filter applies the OutputActions from preflight.
   * @param data
   * @param args
   * @param messageHandler
   * @return
   */
  virtual Result<> executeImpl(DataStructure& data, const Arguments& args, const MessageHandler& messageHandler = {}) const = 0;
};

using FilterCreationFunc = IFilter::UniquePointer (*)();
} // namespace complex
