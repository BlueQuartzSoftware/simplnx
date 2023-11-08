#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Common/StringLiteral.hpp"
#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
class COMPLEXCORE_EXPORT InitializeData : public IFilter
{
public:
  InitializeData() = default;
  ~InitializeData() noexcept override = default;

  InitializeData(const InitializeData&) = delete;
  InitializeData(InitializeData&&) noexcept = delete;

  InitializeData& operator=(const InitializeData&) = delete;
  InitializeData& operator=(InitializeData&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_CellArrayPaths_Key = "cell_arrays";
  static inline constexpr StringLiteral k_InitType_Key = "init_type";
  static inline constexpr StringLiteral k_InitValue_Key = "init_value";
  static inline constexpr StringLiteral k_InitRange_Key = "init_range";
  static inline constexpr StringLiteral k_UseSeed_Key = "use_seed";
  static inline constexpr StringLiteral k_SeedValue_Key = "seed_value";
  static inline constexpr StringLiteral k_SeedArrayName_Key = "seed_array_name";
  static inline constexpr StringLiteral k_UseMultiCompArrays_Key = "use_multi_comp_arrays";
  static inline constexpr StringLiteral k_SingleCompArrayPaths_Key = "single_comp_array_paths";
  static inline constexpr StringLiteral k_MultiCompArrayPaths_Key = "multi_comp_array_paths";
  static inline constexpr StringLiteral k_MultiFillValue_Key = "multi_fill_value";
  static inline constexpr StringLiteral k_StartingFillValue_Key = "starting_fill_value";
  static inline constexpr StringLiteral k_StepOperation_Key = "step_operation";
  static inline constexpr StringLiteral k_StepValue_Key = "step_value";

  enum class InitType : uint64
  {
    Manual = 0,
    Random,
    RandomWithRange
  };

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
  PreflightResult preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief
   * @param data
   * @param args
   * @param pipelineNode
   * @param messageHandler
   * @param shouldCancel
   * @return Result<>
   */
  Result<> executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

private:
  uint64 m_Seed = 0;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, InitializeData, "01c82d15-ba52-4ffa-a7a5-487ee5a613f5");
