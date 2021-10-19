#pragma once

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

#include "ComplexCore/ComplexCore_export.hpp"

namespace complex
{
class COMPLEXCORE_EXPORT ExampleFilter1 : public IFilter
{
public:
  ExampleFilter1() = default;
  ~ExampleFilter1() noexcept override = default;

  ExampleFilter1(const ExampleFilter1&) = delete;
  ExampleFilter1(ExampleFilter1&&) noexcept = delete;

  ExampleFilter1& operator=(const ExampleFilter1&) = delete;
  ExampleFilter1& operator=(ExampleFilter1&&) noexcept = delete;

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
   * @return
   */
  Result<OutputActions> preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const override;

  /**
   * @brief
   * @param data
   * @param args
   * @param messageHandler
   * @return
   */
  Result<> executeImpl(DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, ExampleFilter1, "dd92896b-26ec-4419-b905-567e93e8f39d");
