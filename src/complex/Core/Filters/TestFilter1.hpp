#pragma once

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
class COMPLEX_EXPORT TestFilter1 : public IFilter
{
public:
  TestFilter1() = default;
  ~TestFilter1() noexcept override = default;

  TestFilter1(const TestFilter1&) = delete;
  TestFilter1(TestFilter1&&) noexcept = delete;

  TestFilter1& operator=(const TestFilter1&) = delete;
  TestFilter1& operator=(TestFilter1&&) noexcept = delete;

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
   * @return
   */
  Result<OutputActions> preflightImpl(const DataStructure& data, const Arguments& args) const override;

  /**
   * @brief
   * @param data
   * @param args
   * @return
   */
  Result<> executeImpl(DataStructure& data, const Arguments& args) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex::TestFilter1, "dd92896b-26ec-4419-b905-567e93e8f39d");
