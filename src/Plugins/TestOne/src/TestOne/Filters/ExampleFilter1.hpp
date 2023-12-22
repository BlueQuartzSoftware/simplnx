#pragma once

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include "TestOne/TestOne_export.hpp"

namespace nx::core
{
class TESTONE_EXPORT ExampleFilter1 : public IFilter
{
public:
  ExampleFilter1() = default;
  ~ExampleFilter1() noexcept override = default;

  ExampleFilter1(const ExampleFilter1&) = delete;
  ExampleFilter1(ExampleFilter1&&) noexcept = delete;

  ExampleFilter1& operator=(const ExampleFilter1&) = delete;
  ExampleFilter1& operator=(ExampleFilter1&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_InputDir_Key = "input_dir";
  static inline constexpr StringLiteral k_InputFile_Key = "input_file";
  static inline constexpr StringLiteral k_OutputDir_Key = "output_dir";
  static inline constexpr StringLiteral k_OutputFile_Key = "output_file";

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
   * @brief Returns the default tags for this filter.
   * @return
   */
  std::vector<std::string> defaultTags() const override;

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
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ExampleFilter1, "dd92896b-26ec-4419-b905-567e93e8f39d");
