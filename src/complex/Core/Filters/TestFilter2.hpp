#pragma once

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
class COMPLEX_EXPORT TestFilter2 : public IFilter
{
public:
  TestFilter2() = default;
  ~TestFilter2() noexcept override = default;

  TestFilter2(const TestFilter2&) = delete;
  TestFilter2(TestFilter2&&) noexcept = delete;

  TestFilter2& operator=(const TestFilter2&) = delete;
  TestFilter2& operator=(TestFilter2&&) noexcept = delete;

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

COMPLEX_DEF_FILTER_TRAITS(complex::TestFilter2, "1307bbbc-112d-4aaa-941f-58253787b17e");
