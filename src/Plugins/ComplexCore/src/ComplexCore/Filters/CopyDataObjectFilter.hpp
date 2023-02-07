#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Common/StringLiteral.hpp"
#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
class COMPLEXCORE_EXPORT CopyDataObjectFilter : public IFilter
{
public:
  CopyDataObjectFilter() = default;
  ~CopyDataObjectFilter() noexcept override = default;

  CopyDataObjectFilter(const CopyDataObjectFilter&) = delete;
  CopyDataObjectFilter(CopyDataObjectFilter&&) noexcept = delete;

  CopyDataObjectFilter& operator=(const CopyDataObjectFilter&) = delete;
  CopyDataObjectFilter& operator=(CopyDataObjectFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_DataPath_Key = "existing_data_path";
  static inline constexpr StringLiteral k_UseNewParent_Key = "use_new_parent";
  static inline constexpr StringLiteral k_NewPath_Key = "new_data_path";
  static inline constexpr StringLiteral k_NewPathSuffix_Key = "new_path_suffix";

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

COMPLEX_DEF_FILTER_TRAITS(complex, CopyDataObjectFilter, "ac8d51d8-9167-5628-a060-95a8863a76b1");
