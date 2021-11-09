#pragma once

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

#include "ComplexCore/ComplexCore_export.hpp"

namespace complex
{
class COMPLEXCORE_EXPORT RobustAutomaticThreshold : public IFilter
{
public:
  RobustAutomaticThreshold() = default;
  ~RobustAutomaticThreshold() noexcept override = default;

  RobustAutomaticThreshold(const RobustAutomaticThreshold&) = delete;
  RobustAutomaticThreshold(RobustAutomaticThreshold&&) noexcept = delete;

  RobustAutomaticThreshold& operator=(const RobustAutomaticThreshold&) = delete;
  RobustAutomaticThreshold& operator=(RobustAutomaticThreshold&&) noexcept = delete;

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
   * @brief Returns the filter's Uuid.
   * @return Uuid
   */
  Uuid uuid() const override;

  /**
   * @brief Returns the name to display to filter's users.
   * @return std::string
   */
  std::string humanName() const override;

  /**
   * @brief Returns the filter's Parameters.
   * @return Parameters
   */
  Parameters parameters() const override;

  /**
   * @brief Creates and returns a copy of the filter.
   * @return UniquePointer
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

COMPLEX_DEF_FILTER_TRAITS(complex, RobustAutomaticThreshold, "ade392e6-f0da-4cf3-bf11-dfe69e200b49");
