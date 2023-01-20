#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class ExecuteProcessFilter
 * @brief This filter will ....
 */
class COMPLEXCORE_EXPORT ExecuteProcessFilter : public IFilter
{
public:
  ExecuteProcessFilter() = default;
  ~ExecuteProcessFilter() noexcept override = default;

  ExecuteProcessFilter(const ExecuteProcessFilter&) = delete;
  ExecuteProcessFilter(ExecuteProcessFilter&&) noexcept = delete;

  ExecuteProcessFilter& operator=(const ExecuteProcessFilter&) = delete;
  ExecuteProcessFilter& operator=(ExecuteProcessFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_Arguments_Key = "arguments";
  static inline constexpr StringLiteral k_Blocking_Key = "blocking";
  static inline constexpr StringLiteral k_Timeout_Key = "timeout";
  static inline constexpr StringLiteral k_OutputLogFile_Key = "output_log_file";

  /**
   * @brief Returns the name of the filter.
   * @return
   */
  std::string name() const override;

  /**
   * @brief Returns the C++ classname of this filter.
   * @return
   */
  std::string className() const override;

  /**
   * @brief Returns the uuid of the filter.
   * @return
   */
  Uuid uuid() const override;

  /**
   * @brief Returns the human readable name of the filter.
   * @return
   */
  std::string humanName() const override;

  /**
   * @brief Returns the default tags for this filter.
   * @return
   */
  std::vector<std::string> defaultTags() const override;

  /**
   * @brief Returns the parameters of the filter (i.e. its inputs)
   * @return
   */
  Parameters parameters() const override;

  /**
   * @brief Returns a copy of the filter.
   * @return
   */
  UniquePointer clone() const override;

protected:
  /**
   * @brief Takes in a DataStructure and checks that the filter can be run on it with the given arguments.
   * Returns any warnings/errors. Also returns the changes that would be applied to the DataStructure.
   * Some parts of the actions may not be completely filled out if all the required information is not available at preflight time.
   * @param ds The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  PreflightResult preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param ds The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, ExecuteProcessFilter, "fb511a70-2175-4595-8c11-d1b5b6794221");
/* LEGACY UUID FOR THIS FILTER 8a2308ec-86cd-5636-9a0a-6c7d383e9e7f */
