#pragma once

#include "complex/Filter/FilterHandle.hpp"
#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

#include "TestOne/TestOne_export.hpp"
namespace complex
{
/**
 * @brief The ErrorWarningFilter class. See [Filter documentation](@ref ErrorWarningFilter) for details.
 */
class TESTONE_EXPORT ErrorWarningFilter : public complex::IFilter
{
public:
  ErrorWarningFilter() = default;
  ~ErrorWarningFilter() noexcept override = default;

  ErrorWarningFilter(const ErrorWarningFilter&) = delete;
  ErrorWarningFilter(ErrorWarningFilter&&) noexcept = delete;

  ErrorWarningFilter& operator=(const ErrorWarningFilter&) = delete;
  ErrorWarningFilter& operator=(ErrorWarningFilter&&) noexcept = delete;

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
  PreflightResult preflightImpl(const complex::DataStructure& data, const complex::Arguments& args, const MessageHandler& messageHandler) const override;
  complex::Result<> executeImpl(complex::DataStructure& data, const complex::Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, ErrorWarningFilter, "3ede4bcd-944a-4bca-a0d4-ade65403641e");
