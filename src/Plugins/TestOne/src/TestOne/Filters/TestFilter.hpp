#pragma once

#include "complex/Filter/FilterHandle.hpp"
#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

#include "TestOne/TestOne_export.hpp"
namespace complex
{
class TESTONE_EXPORT TestFilter : public complex::IFilter
{
public:
  TestFilter();
  ~TestFilter() override;

  /**
   * @brief Returns the name of the filter.
   * @return std::string
   */
  std::string name() const override;

  /**
   * @brief Returns the C++ classname of this filter.
   * @return
   */
  std::string className() const override;

  /**
   * @brief Returns the filters ID as a std::string.
   * @return Uuid
   */
  complex::Uuid uuid() const override;

  /**
   * @brief Returns the filter's human label.
   * @return std::string
   */
  std::string humanName() const override;

  /**
   * @brief Returns a collection of parameters used.
   * @return Parameters
   */
  complex::Parameters parameters() const override;

  /**
   * @brief Returns a unique_pointer to a copy of the filter.
   * @return complex::IFilter::UniquePointer
   */
  UniquePointer clone() const override;

protected:
  complex::Result<complex::OutputActions> preflightImpl(const complex::DataStructure& data, const complex::Arguments& args, const MessageHandler& messageHandler) const override;
  complex::Result<> executeImpl(complex::DataStructure& data, const complex::Arguments& args, const MessageHandler& messageHandler) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, TestFilter, "5502c3f7-37a8-4a86-b003-1c856be02491");
