#pragma once

#include "complex/Filter/IFilter.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
class COMPLEX_EXPORT ImportTextFilter : public IFilter
{
public:
  ImportTextFilter();
  ~ImportTextFilter() noexcept override = default;

  ImportTextFilter(const ImportTextFilter&) = delete;
  ImportTextFilter(ImportTextFilter&&) noexcept = delete;

  ImportTextFilter& operator=(const ImportTextFilter&) = delete;
  ImportTextFilter& operator=(ImportTextFilter&&) noexcept = delete;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] std::string name() const override;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] Uuid uuid() const override;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] std::string humanName() const override;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] Parameters const& parameters() const override;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] UniquePointer clone() const override;

private:
  /**
   * @brief
   * @param data
   * @param args
   * @param messageHandler
   * @return
   */
  Result<OutputActions> preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const override;

  /**
   * @brief
   * @param data
   * @param args
   * @param messageHandler
   * @return
   */
  Result<> executeImpl(DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const override;

  /**
   * @brief
   */
  Parameters m_params;
};
} // namespace complex
