#pragma once

#include "simplnx/Filter/FilterHandle.hpp"
#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include "TestOne/TestOne_export.hpp"
namespace nx::core
{
/**
 * @brief The ErrorWarningFilter class. See [Filter documentation](@ref ErrorWarningFilter) for details.
 */
class TESTONE_EXPORT ErrorWarningFilter : public nx::core::IFilter
{
public:
  ErrorWarningFilter() = default;
  ~ErrorWarningFilter() noexcept override = default;

  ErrorWarningFilter(const ErrorWarningFilter&) = delete;
  ErrorWarningFilter(ErrorWarningFilter&&) noexcept = delete;

  ErrorWarningFilter& operator=(const ErrorWarningFilter&) = delete;
  ErrorWarningFilter& operator=(ErrorWarningFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_PreflightWarning_Key = "preflight_warning";
  static inline constexpr StringLiteral k_PreflightError_Key = "preflight_error";
  static inline constexpr StringLiteral k_PreflightException_Key = "preflight_exception";
  static inline constexpr StringLiteral k_ExecuteWarning_Key = "execute_warning";
  static inline constexpr StringLiteral k_ExecuteError_Key = "execute_error";
  static inline constexpr StringLiteral k_ExecuteException_Key = "execute_exception";

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
  nx::core::Uuid uuid() const override;

  /**
   * @brief Returns the filter's human label.
   * @return std::string
   */
  std::string humanName() const override;

  /**
   * @brief Returns the default tags for this filter.
   * @return
   */
  std::vector<std::string> defaultTags() const override;

  /**
   * @brief Returns a collection of parameters used.
   * @return Parameters
   */
  nx::core::Parameters parameters() const override;

  /**
   * @brief Returns parameters version integer.
   * Initial version should always be 1.
   * Should be incremented everytime the parameters change.
   * @return VersionType
   */
  VersionType parametersVersion() const override;

  /**
   * @brief Returns a unique_pointer to a copy of the filter.
   * @return nx::core::IFilter::UniquePointer
   */
  UniquePointer clone() const override;

protected:
  PreflightResult preflightImpl(const nx::core::DataStructure& data, const nx::core::Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;
  nx::core::Result<> executeImpl(nx::core::DataStructure& data, const nx::core::Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                 const std::atomic_bool& shouldCancel) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ErrorWarningFilter, "3ede4bcd-944a-4bca-a0d4-ade65403641e");
