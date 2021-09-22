#pragma once

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
class COMPLEX_EXPORT ImportTextFilter : public IFilter
{
public:
  ImportTextFilter() = default;
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
  [[nodiscard]] Parameters parameters() const override;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] UniquePointer clone() const override;

protected:
  /**
   * @brief
   * @param data
   * @param args
   * @param messageHandler = {}
   * @return
   */
  Result<OutputActions> preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler = {}) const override;

  /**
   * @brief
   * @param data
   * @param args
   * @param messageHandler = {}
   * @return
   */
  Result<> executeImpl(DataStructure& data, const Arguments& args, const MessageHandler& messageHandler = {}) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex::ImportTextFilter, "25f7df3e-ca3e-4634-adda-732c0e56efd4");
