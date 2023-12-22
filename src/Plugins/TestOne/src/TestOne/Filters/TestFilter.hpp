#pragma once

#include "simplnx/Filter/FilterHandle.hpp"
#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include "TestOne/TestOne_export.hpp"
namespace nx::core
{
class TESTONE_EXPORT TestFilter : public nx::core::IFilter
{
public:
  TestFilter();
  ~TestFilter() override;

  /**
   * @brief Returns the name of the filter.
   * @return std::string
   */
  std::string name() const override;

  /**
   * @brief Returns the C++ classname of this filter.
   * @return
   */
  std::string className() const override;

  /**
   * @brief Returns the filters ID as a std::string.
   * @return Uuid
   */
  nx::core::Uuid uuid() const override;

  /**
   * @brief Returns the filter's human label.
   * @return std::string
   */
  std::string humanName() const override;

  /**
   * @brief Returns the default tags for this filter.
   * @return
   */
  std::vector<std::string> defaultTags() const override;

  /**
   * @brief Returns a collection of parameters used.
   * @return Parameters
   */
  nx::core::Parameters parameters() const override;

  /**
   * @brief Returns a unique_pointer to a copy of the filter.
   * @return nx::core::IFilter::UniquePointer
   */
  UniquePointer clone() const override;

protected:
  PreflightResult preflightImpl(const nx::core::DataStructure& data, const nx::core::Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;
  nx::core::Result<> executeImpl(nx::core::DataStructure& data, const nx::core::Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                 const std::atomic_bool& shouldCancel) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, TestFilter, "5502c3f7-37a8-4a86-b003-1c856be02491");
