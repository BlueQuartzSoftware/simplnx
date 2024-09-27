#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Common/StringLiteral.hpp"
#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
class SIMPLNXCORE_EXPORT InitializeDataFilter : public IFilter
{
public:
  InitializeDataFilter() = default;
  ~InitializeDataFilter() noexcept override = default;

  InitializeDataFilter(const InitializeDataFilter&) = delete;
  InitializeDataFilter(InitializeDataFilter&&) noexcept = delete;

  InitializeDataFilter& operator=(const InitializeDataFilter&) = delete;
  InitializeDataFilter& operator=(InitializeDataFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_ArrayPath_Key = "array_path";
  static inline constexpr StringLiteral k_InitType_Key = "init_type_index";
  static inline constexpr StringLiteral k_InitValue_Key = "init_value";
  static inline constexpr StringLiteral k_StartingFillValue_Key = "starting_fill_value";
  static inline constexpr StringLiteral k_StepOperation_Key = "step_operation_index";
  static inline constexpr StringLiteral k_StepValue_Key = "step_value";
  static inline constexpr StringLiteral k_UseSeed_Key = "use_seed";
  static inline constexpr StringLiteral k_SeedValue_Key = "seed_value";
  static inline constexpr StringLiteral k_SeedArrayName_Key = "seed_array_name";
  static inline constexpr StringLiteral k_InitStartRange_Key = "init_start_range";
  static inline constexpr StringLiteral k_InitEndRange_Key = "init_end_range";
  static inline constexpr StringLiteral k_StandardizeSeed_Key = "standardize_seed";

  /**
   * @brief
   * @return std::string
   */
  std::string name() const override;

  /**
   * @brief Returns the C++ classname of this filter.
   * @return std::string
   */
  std::string className() const override;

  /**
   * @brief
   * @return Uuid
   */
  Uuid uuid() const override;

  /**
   * @brief
   * @return std::string
   */
  std::string humanName() const override;

  /**
   * @brief Returns the default tags for this filter.
   * @return
   */
  std::vector<std::string> defaultTags() const override;

  /**
   * @brief
   * @return Parameters
   */
  Parameters parameters() const override;

  /**
   * @brief Returns parameters version integer.
   * Initial version should always be 1.
   * Should be incremented everytime the parameters change.
   * @return VersionType
   */
  VersionType parametersVersion() const override;

  /**
   * @brief
   * @return UniquePointer
   */
  UniquePointer clone() const override;

protected:
  /**
   * @brief
   * @param data
   * @param args
   * @param messageHandler
   * @param shouldCancel
   * @return PreflightResult
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief
   * @param data
   * @param args
   * @param pipelineNode
   * @param messageHandler
   * @param shouldCancel
   * @return Result<>
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                       const std::atomic_bool& shouldCancel) const override;

private:
  uint64 m_Seed = 0;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, InitializeDataFilter, "01c82d15-ba52-4ffa-a7a5-487ee5a613f5");
