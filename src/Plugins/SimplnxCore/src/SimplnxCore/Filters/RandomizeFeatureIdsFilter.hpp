#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Filter/AbstractFilter.hpp"
#include "simplnx/Filter/FilterTraits.hpp"

namespace nx::core
{
class SIMPLNXCORE_EXPORT RandomizeFeatureIdsFilter : public AbstractFilter
{
public:
  RandomizeFeatureIdsFilter() = default;
  ~RandomizeFeatureIdsFilter() noexcept override = default;

  RandomizeFeatureIdsFilter(const RandomizeFeatureIdsFilter&) = delete;
  RandomizeFeatureIdsFilter(RandomizeFeatureIdsFilter&&) noexcept = delete;

  RandomizeFeatureIdsFilter& operator=(const RandomizeFeatureIdsFilter&) = delete;
  RandomizeFeatureIdsFilter& operator=(RandomizeFeatureIdsFilter&&) noexcept = delete;

  // Parameter Keys
  static constexpr StringLiteral k_FeatureIdsPath_Key = "feature_ids_path";
  static constexpr StringLiteral k_FeatureAMPath_Key = "feature_am_path";

  /**
   * @brief Returns the filter's name.
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
   * @brief Returns the filter name as a human-readable string.
   * @return std::string
   */
  std::string humanName() const override;

  /**
   * @brief Returns the default tags for this filter.
   * @return
   */
  std::vector<std::string> defaultTags() const override;

  /**
   * @brief Returns a collection of parameters required to execute the filter.
   * @return Parameters
   */
  Parameters parameters() const override;

  /**
   * @brief Returns parameters version integer.
   * The Initial version should always be 1.
   * Should be incremented everytime the parameters change.
   * @return VersionType
   */
  VersionType parametersVersion() const override;

  /**
   * @brief Creates and returns a copy of the filter.
   * @return UniquePointer
   */
  UniquePointer clone() const override;

protected:
  /**
   * @brief
   * @param data
   * @param filterArgs
   * @param messageHandler
   * @return Result<OutputActions>
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                const ExecutionContext& executionContext) const override;

  /**
   * @brief
   * @param dataStructure
   * @param args
   * @param pipelineNode
   * @param messageHandler
   * @return Result<>
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                       const ExecutionContext& executionContext) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, RandomizeFeatureIdsFilter, "1766d576-9c03-4152-82b1-e72c4eb43630");
