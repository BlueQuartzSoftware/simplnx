#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Common/StringLiteral.hpp"
#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
class COMPLEXCORE_EXPORT MultiThresholdObjects : public IFilter
{
public:
  MultiThresholdObjects() = default;
  ~MultiThresholdObjects() noexcept override = default;

  MultiThresholdObjects(const MultiThresholdObjects&) = delete;
  MultiThresholdObjects(MultiThresholdObjects&&) noexcept = delete;

  MultiThresholdObjects& operator=(const MultiThresholdObjects&) = delete;
  MultiThresholdObjects& operator=(MultiThresholdObjects&&) noexcept = delete;

  static inline constexpr StringLiteral k_ArrayThresholds_Key = "array_thresholds";
  static inline constexpr StringLiteral k_CreatedDataPath_Key = "created_data_path";
  static inline constexpr StringLiteral k_CreatedMaskType_Key = "created_mask_type";

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

COMPLEX_DEF_FILTER_TRAITS(complex, MultiThresholdObjects, "4246245e-1011-4add-8436-0af6bed19228");
