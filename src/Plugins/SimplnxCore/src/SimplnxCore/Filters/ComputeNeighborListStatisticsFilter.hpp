#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ComputeNeighborListStatisticsFilter
 * @brief
 */
class SIMPLNXCORE_EXPORT ComputeNeighborListStatisticsFilter : public IFilter
{
public:
  ComputeNeighborListStatisticsFilter() = default;
  ~ComputeNeighborListStatisticsFilter() noexcept override = default;

  ComputeNeighborListStatisticsFilter(const ComputeNeighborListStatisticsFilter&) = delete;
  ComputeNeighborListStatisticsFilter(ComputeNeighborListStatisticsFilter&&) noexcept = delete;

  ComputeNeighborListStatisticsFilter& operator=(const ComputeNeighborListStatisticsFilter&) = delete;
  ComputeNeighborListStatisticsFilter& operator=(ComputeNeighborListStatisticsFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_FindLength_Key = "find_length";
  static inline constexpr StringLiteral k_FindMinimum_Key = "find_minimum";
  static inline constexpr StringLiteral k_FindMaximum_Key = "find_maximum";
  static inline constexpr StringLiteral k_FindMean_Key = "find_mean";
  static inline constexpr StringLiteral k_FindMedian_Key = "find_median";
  static inline constexpr StringLiteral k_FindStandardDeviation_Key = "find_standard_deviation";
  static inline constexpr StringLiteral k_FindSummation_Key = "find_summation";
  static inline constexpr StringLiteral k_InputNeighborListPath_Key = "input_neighbor_list_path";
  static inline constexpr StringLiteral k_LengthName_Key = "length_array_name";
  static inline constexpr StringLiteral k_MinimumName_Key = "minimum_array_name";
  static inline constexpr StringLiteral k_MaximumName_Key = "maximum_array_name";
  static inline constexpr StringLiteral k_MeanName_Key = "mean_array_name";
  static inline constexpr StringLiteral k_MedianName_Key = "median_array_name";
  static inline constexpr StringLiteral k_StandardDeviationName_Key = "standard_deviation_array_name";
  static inline constexpr StringLiteral k_SummationName_Key = "summation_array_name";

  /**
   * @brief Reads SIMPL json and converts it simplnx Arguments.
   * @param json
   * @return Result<Arguments>
   */
  static Result<Arguments> FromSIMPLJson(const nlohmann::json& json);

  /**
   * @brief Returns the name of the filter.
   * @return std::string
   */
  std::string name() const override;

  /**
   * @brief Returns the C++ classname of this filter.
   * @return std::string
   */
  std::string className() const override;

  /**
   * @brief Returns the filter's UUID.
   * @return Uuid
   */
  Uuid uuid() const override;

  /**
   * @brief Returns the filter name name presented to the user.
   * @return std::string
   */
  std::string humanName() const override;

  /**
   * @brief Returns the default tags for this filter.
   * @return
   */
  std::vector<std::string> defaultTags() const override;

  /**
   * @brief Returns the parameters required to run the filter.
   * @return Parameters
   */
  Parameters parameters() const override;

  /**
   * @brief Creates a copy of the filter.
   * @return IFilter::UniquePointer
   */
  UniquePointer clone() const override;

protected:
  /**
   * @brief
   * @param data
   * @param args
   * @param messageHandler
   * @return PreflightResult
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param pipelineNode The PipelineNode object that called this filter
   * @param messageHandler The MessageHandler object
   * @param shouldCancel The atomic boolean that holds if the filter should be canceled
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                       const std::atomic_bool& shouldCancel) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ComputeNeighborListStatisticsFilter, "270a824e-414b-455e-bb7e-b38a0848990d");
