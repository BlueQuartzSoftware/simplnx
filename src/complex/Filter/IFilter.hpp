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
class FilterMessage;
class FilterObserver;

/**
 * @class IFilter
 * @brief
 */
class COMPLEX_EXPORT IFilter
{
  friend class FilterObserver;

public:
  using UniquePointer = std::unique_ptr<IFilter>;

  virtual ~IFilter() noexcept;

  IFilter(const IFilter&) = delete;
  IFilter(IFilter&&) noexcept = delete;

  IFilter& operator=(const IFilter&) = delete;
  IFilter& operator=(IFilter&&) noexcept = delete;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] virtual std::string name() const = 0;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] virtual Uuid uuid() const = 0;

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
   * @return
   */
  [[nodiscard]] virtual UniquePointer clone() const = 0;

  /**
   * @brief
   * @param data
   * @param args
   * @param messageHandler
   * @return
   */
  Result<OutputActions> preflight(const DataStructure& data, const Arguments& args) const;

  /**
   * @brief
   * @param data
   * @param args
   * @param messageHandler
   * @return
   */
  Result<> execute(DataStructure& data, const Arguments& args) const;

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
  [[nodiscard]] Result<Arguments> fromJson(const nlohmann::json& json) const;

protected:
  IFilter() = default;

  /**
   * @brief
   * @param data
   * @param args
   * @return
   */
  virtual Result<OutputActions> preflightImpl(const DataStructure& data, const Arguments& args) const = 0;

  /**
   * @brief
   * @param data
   * @param args
   * @return
   */
  virtual Result<> executeImpl(DataStructure& data, const Arguments& args) const = 0;

  /**
   * @brief Notifies known observers of the provided message.
   * @param msg
   */
  void notify(const std::shared_ptr<FilterMessage>& msg);

  /**
   * @brief Creates an info message from the provided string and notifies known
   * observers to the created message.
   * @param msg
   */
  void notifyInfo(const std::string& msg);

  /**
   * @brief Creates an error message from the provided string and notifies known
   * observers to the created message.
   * @param msg
   */
  void notifyError(const std::string& msg);

  /**
   * @brief Creates a warning message from the provided string and notifies known
   * observers to the created message.
   * @param msg
   */
  void notifyWarning(const std::string& msg);

  /**
   * @brief Adds the specified observer to the list of known observers.
   * @param obs
   */
  void addObserver(FilterObserver* obs);

  /**
   * @brief Removes the specified observer from the list of known observers.
   * @param obs
   */
  void removeObserver(FilterObserver* obs);

private:
  std::vector<FilterObserver*> m_Observers;
};

using FilterCreationFunc = IFilter::UniquePointer (*)();
} // namespace complex
