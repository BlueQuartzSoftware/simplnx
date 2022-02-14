#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Common/StringLiteral.hpp"
#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
class COMPLEXCORE_EXPORT CopyDataGroup : public IFilter
{
public:
  CopyDataGroup() = default;
  ~CopyDataGroup() noexcept override = default;

  CopyDataGroup(const CopyDataGroup&) = delete;
  CopyDataGroup(CopyDataGroup&&) noexcept = delete;

  CopyDataGroup& operator=(const CopyDataGroup&) = delete;
  CopyDataGroup& operator=(CopyDataGroup&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_DataPath_Key = "existing_data_path";
  static inline constexpr StringLiteral k_NewPath_Key = "new_data_path";

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

COMPLEX_DEF_FILTER_TRAITS(complex, CopyDataGroup, "ac8d51d8-9167-5628-a060-95a8863a76b1");
