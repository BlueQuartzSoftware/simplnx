#pragma once

#include "complex/Filter/IFilter.hpp"
#include "complex/complex_export.hpp"
#include "complex/Core/FilterHandle.hpp"

class TestFilter : public complex::IFilter
{
public:
  static const complex::Uuid ID;
  
  TestFilter() = default;
  ~TestFilter() noexcept override = default;

  TestFilter(const TestFilter&) = delete;
  TestFilter(TestFilter&&) = delete;

  TestFilter& operator=(const TestFilter&) = delete;
  TestFilter& operator=(TestFilter&&) = delete;
  
    /**
   * @brief
   * @return
   */
  [[nodiscard]] std::string name() const override;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] complex::Uuid uuid() const override;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] std::string humanName() const override;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] complex::Parameters parameters() const override;

protected:  
  /**
   * @brief
   * @param data
   * @param args
   * @param messageHandler
   * @return
   */
  complex::Result<complex::OutputActions> preflightImpl(const complex::DataStructure& data, const complex::Arguments& args, const complex::IFilter::MessageHandler& messageHandler) const override;

  /**
   * @brief
   * @param data
   * @param args
   * @param messageHandler
   * @return
   */
  complex::Result<> executeImpl(complex::DataStructure& data, const complex::Arguments& args, const complex::IFilter::MessageHandler& messageHandler) const override;
};
