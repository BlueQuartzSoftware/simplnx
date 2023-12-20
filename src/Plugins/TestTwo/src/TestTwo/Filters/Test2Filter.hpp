#pragma once

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include "TestTwo/TestTwo_export.hpp"
namespace nx::core
{
class TESTTWO_EXPORT Test2Filter : public nx::core::IFilter
{
public:
  Test2Filter();
  ~Test2Filter() override;

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
   * @return nx::core::Uuid
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
  /**
   * @brief Filter-specifics for performing dataCheck.
   * @param data
   * @param args
   * @param messageHandler
   * @return Result<OutputActions>
   */
  PreflightResult preflightImpl(const nx::core::DataStructure& data, const nx::core::Arguments& args, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief Filter-specifics for performing execute.
   * @param data
   * @param args
   * @param pipelineNode
   * @param messageHandler
   * @return Result<>
   */
  nx::core::Result<> executeImpl(nx::core::DataStructure& data, const nx::core::Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                 const std::atomic_bool& shouldCancel) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, Test2Filter, "ad9cf22b-bc5e-41d6-b02e-bb49ffd12c04");
