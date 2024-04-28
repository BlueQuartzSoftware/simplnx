#pragma once

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include "TestOne/TestOne_export.hpp"

namespace nx::core
{
class TESTONE_EXPORT ExampleFilter2Filter : public IFilter
{
public:
  ExampleFilter2Filter() = default;
  ~ExampleFilter2Filter() noexcept override = default;

  ExampleFilter2Filter(const ExampleFilter2Filter&) = delete;
  ExampleFilter2Filter(ExampleFilter2Filter&&) noexcept = delete;

  ExampleFilter2Filter& operator=(const ExampleFilter2Filter&) = delete;
  ExampleFilter2Filter& operator=(ExampleFilter2Filter&&) noexcept = delete;

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ExampleFilter2Filter, "1307bbbc-112d-4aaa-941f-58253787b17e");
