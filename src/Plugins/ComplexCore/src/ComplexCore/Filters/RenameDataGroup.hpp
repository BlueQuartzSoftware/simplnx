#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Common/StringLiteral.hpp"
#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
class COMPLEXCORE_EXPORT RenameDataGroup : public IFilter
{
public:
  RenameDataGroup() = default;
  ~RenameDataGroup() noexcept override = default;

  RenameDataGroup(const RenameDataGroup&) = delete;
  RenameDataGroup(RenameDataGroup&&) noexcept = delete;

  RenameDataGroup& operator=(const RenameDataGroup&) = delete;
  RenameDataGroup& operator=(RenameDataGroup&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_DataGroup_Key = "data_group";
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

COMPLEX_DEF_FILTER_TRAITS(complex, RenameDataGroup, "d53c808f-004d-5fac-b125-0fffc8cc78d6");
