#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Common/StringLiteral.hpp"
#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
class COMPLEXCORE_EXPORT RenameAttributeArray : public IFilter
{
public:
  RenameAttributeArray() = default;
  ~RenameAttributeArray() noexcept override = default;

  RenameAttributeArray(const RenameAttributeArray&) = delete;
  RenameAttributeArray(RenameAttributeArray&&) noexcept = delete;

  RenameAttributeArray& operator=(const RenameAttributeArray&) = delete;
  RenameAttributeArray& operator=(RenameAttributeArray&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_ArrayPath_Key = "array_path";
  static inline constexpr StringLiteral k_NewName_Key = "new_name";

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
   * @param filterArgs
   * @param messageHandler
   * @return Result<OutputActions>
   */
  PreflightResult preflightImpl(const DataStructure& data, const Arguments& filterArgs, const MessageHandler& messageHandler) const override;

  /**
   * @brief
   * @param dataStructure
   * @param args
   * @param pipelineNode
   * @param messageHandler
   * @return Result<>
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, RenameAttributeArray, "53a5f731-2858-5e3e-bd43-8f2cf45d90ec");
