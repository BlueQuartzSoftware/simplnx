#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class FindDifferencesMap
 * @brief
 */
class SIMPLNXCORE_EXPORT FindDifferencesMap : public IFilter
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
   * @param dataStructure
   * @param args
   * @param messageHandler
   * @return PreflightResult
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

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
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, FindDifferencesMap, "5a0ee5b5-c135-4535-85d0-9c2058943099");
