#pragma once

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

#include "TestOne/TestOne_export.hpp"

namespace complex
{
class TESTONE_EXPORT CreateOutOfCoreArray : public IFilter
{
public:
  CreateOutOfCoreArray() = default;
  ~CreateOutOfCoreArray() noexcept override = default;

  CreateOutOfCoreArray(const CreateOutOfCoreArray&) = delete;
  CreateOutOfCoreArray(CreateOutOfCoreArray&&) noexcept = delete;

  CreateOutOfCoreArray& operator=(const CreateOutOfCoreArray&) = delete;
  CreateOutOfCoreArray& operator=(CreateOutOfCoreArray&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_NumericType_Key = "numeric_type";
  static inline constexpr StringLiteral k_NumComps_Key = "component_count";
  static inline constexpr StringLiteral k_TupleDims_Key = "tuple_dimensions";
  static inline constexpr StringLiteral k_DataPath_Key = "output_data_array";
  static inline constexpr StringLiteral k_InitilizationValue_Key = "initialization_value";

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
   * @return Result<OutputActions>
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

COMPLEX_DEF_FILTER_TRAITS(complex, CreateOutOfCoreArray, "f1ac8cb2-ec9b-4764-a6f8-9211de3f8975");
