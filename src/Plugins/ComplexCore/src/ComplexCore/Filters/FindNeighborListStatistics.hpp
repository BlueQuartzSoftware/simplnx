#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Common/StringLiteral.hpp"
#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class FindNeighborListStatistics
 * @brief
 */
class COMPLEXCORE_EXPORT FindNeighborListStatistics : public IFilter
{
public:
  FindNeighborListStatistics() = default;
  ~FindNeighborListStatistics() noexcept override = default;

  FindNeighborListStatistics(const FindNeighborListStatistics&) = delete;
  FindNeighborListStatistics(FindNeighborListStatistics&&) noexcept = delete;

  FindNeighborListStatistics& operator=(const FindNeighborListStatistics&) = delete;
  FindNeighborListStatistics& operator=(FindNeighborListStatistics&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_FindLength_Key = "find_length";
  static inline constexpr StringLiteral k_FindMinimum_Key = "find_minimum";
  static inline constexpr StringLiteral k_FindMaximum_Key = "find_maximum";
  static inline constexpr StringLiteral k_FindMean_Key = "find_mean";
  static inline constexpr StringLiteral k_FindMedian_Key = "find_median";
  static inline constexpr StringLiteral k_FindStandardDeviation_Key = "find_standard_deviation";
  static inline constexpr StringLiteral k_FindSummation_Key = "find_summation";
  static inline constexpr StringLiteral k_InputArray_Key = "input_array";
  static inline constexpr StringLiteral k_Length_Key = "length";
  static inline constexpr StringLiteral k_Minimum_Key = "minimum";
  static inline constexpr StringLiteral k_Maximum_Key = "maximum";
  static inline constexpr StringLiteral k_Mean_Key = "mean";
  static inline constexpr StringLiteral k_Median_Key = "median";
  static inline constexpr StringLiteral k_StandardDeviation_Key = "standard_deviation";
  static inline constexpr StringLiteral k_Summation_Key = "summation";

  /**
   * @brief Reads SIMPL json and converts it complex Arguments.
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
  OutputActions createCompatibleArrays(const DataStructure& data, const Arguments& args) const;

  /**
   * @brief
   * @param data
   * @param args
   * @param messageHandler
   * @return PreflightResult
   */
  PreflightResult preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief
   * @param data
   * @param args
   * @param pipelineNode
   * @param messageHandler
   * @return Result<>
   */
  Result<> executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, FindNeighborListStatistics, "270a824e-414b-455e-bb7e-b38a0848990d");
