#pragma once

#include "TestOne/TestOne_export.hpp"
#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class FlushFailureFilter
 * @brief Supplies bounded completion faults for actual command-line acceptance tests.
 */
class TESTONE_EXPORT FlushFailureFilter : public IFilter
{
public:
  static constexpr StringLiteral k_Mode_Key = "completion_test_mode";

  /**
   * @brief Creates an unarmed test fixture filter.
   */
  FlushFailureFilter() = default;
  ~FlushFailureFilter() noexcept override = default;

  std::string name() const override;
  std::string className() const override;
  Uuid uuid() const override;
  std::string humanName() const override;
  VersionType parametersVersion() const override;
  std::vector<std::string> defaultTags() const override;

  /**
   * @brief Declares the bounded test-mode parameter.
   * @return A parameter set with unarmed mode as its default.
   */
  Parameters parameters() const override;

  /**
   * @brief Creates a fresh filter; arguments carry the selected fixture mode.
   * @return A new filter instance.
   */
  UniquePointer clone() const override;

protected:
  /**
   * @brief Rejects unknown fixture modes before execution.
   * @param dataStructure Is unchanged during preflight.
   * @param filterArgs Selects a mode from zero through six.
   * @param messageHandler Is unused during preflight.
   * @param shouldCancel Is checked by the IFilter wrapper.
   * @param executionContext Is supplied by the parent pipeline.
   * @return Valid actions and a sentinel, or an invalid-mode error.
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                const ExecutionContext& executionContext) const override;

  /**
   * @brief Installs literal data and arms the selected completion fault.
   * @param dataStructure Receives the small resident fixture and optional copy-fault group.
   * @param filterArgs Selects the completion mode.
   * @param pipelineNode Identifies the node for the root-only End boundary.
   * @param messageHandler Reports the executed fixture UUID.
   * @param shouldCancel Is checked by the IFilter wrapper.
   * @param executionContext Is supplied by the parent pipeline.
   * @return An original execute error only in the combined-failure mode.
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                       const ExecutionContext& executionContext) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, FlushFailureFilter, "2a98fc79-c485-45d5-8fa9-1f298f3d6729");
