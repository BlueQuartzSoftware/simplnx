#pragma once

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

#include "test/testplugin_export.hpp"

class TESTPLUGIN_EXPORT TestFilter : public complex::IFilter
{
public:
  TestFilter();
  ~TestFilter() override;

  /**
   * @brief Returns the name of the filter.
   * @return std::string
   */
  [[nodiscard]] std::string name() const override;

  /**
   * @brief Returns the filters ID as a std::string.
   * @return Uuid
   */
  [[nodiscard]] complex::Uuid uuid() const override;

  /**
   * @brief Returns the filter's human label.
   * @return std::string
   */
  [[nodiscard]] std::string humanName() const override;

  /**
   * @brief Returns a collection of parameters used.
   * @return Parameters
   */
  [[nodiscard]] complex::Parameters parameters() const override;

  /**
   * @brief Returns a unique_pointer to a copy of the filter.
   * @return complex::IFilter::UniquePointer
   */
  [[nodiscard]] UniquePointer clone() const override;

protected:
  complex::Result<complex::OutputActions> preflightImpl(const complex::DataStructure& data, const complex::Arguments& args) const override;
  complex::Result<> executeImpl(complex::DataStructure& data, const complex::Arguments& args) const override;
};

COMPLEX_DEF_FILTER_TRAITS(TestFilter, "5502c3f7-37a8-4a86-b003-1c856be02491");
