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
  static inline constexpr StringLiteral k_DataPath_Key = "removed_data_path";

  /**
   * @brief
   * @return
   */
  std::string name() const override;

  /**
   * @brief Returns the C++ classname of this filter.
   * @return
   */
  std::string className() const override;

  /**
   * @brief
   * @return
   */
  Uuid uuid() const override;

  /**
   * @brief
   * @return
   */
  std::string humanName() const override;

  /**
   * @brief
   * @return
   */
  Parameters parameters() const override;

  /**
   * @brief
   * @return
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
  PreflightResult preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const override;

  /**
   * @brief
   * @param data
   * @param args
   * @param pipelineNode
   * @param messageHandler
   * @return Result<>
   */
  Result<> executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, FindDifferencesMap, "5a0ee5b5-c135-4535-85d0-9c2058943099");
