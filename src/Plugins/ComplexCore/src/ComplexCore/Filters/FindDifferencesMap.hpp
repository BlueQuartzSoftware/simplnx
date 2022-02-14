#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Common/StringLiteral.hpp"
#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class FindDifferencesMap
 * @brief
 */
class COMPLEXCORE_EXPORT FindDifferencesMap : public IFilter
{
public:
  FindDifferencesMap() = default;
  ~FindDifferencesMap() noexcept override = default;

  FindDifferencesMap(const FindDifferencesMap&) = delete;
  FindDifferencesMap(FindDifferencesMap&&) noexcept = delete;

  FindDifferencesMap& operator=(const FindDifferencesMap&) = delete;
  FindDifferencesMap& operator=(FindDifferencesMap&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_FirstInputArrayPath_Key = "first_input_array_path";
  static inline constexpr StringLiteral k_SecondInputArrayPath_Key = "second_input_array_path";
  static inline constexpr StringLiteral k_DifferenceMapArrayPath_Key = "difference_map_array_path";

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

COMPLEX_DEF_FILTER_TRAITS(complex, FindDifferencesMap, "5a0ee5b5-c135-4535-85d0-9c2058943099");
