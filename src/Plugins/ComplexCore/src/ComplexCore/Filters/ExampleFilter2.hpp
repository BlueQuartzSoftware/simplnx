#pragma once

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

#include "ComplexCore/ComplexCore_export.hpp"

namespace complex
{
class COMPLEXCORE_EXPORT ExampleFilter2 : public IFilter
{
public:
  ExampleFilter2() = default;
  ~ExampleFilter2() noexcept override = default;

  ExampleFilter2(const ExampleFilter2&) = delete;
  ExampleFilter2(ExampleFilter2&&) noexcept = delete;

  ExampleFilter2& operator=(const ExampleFilter2&) = delete;
  ExampleFilter2& operator=(ExampleFilter2&&) noexcept = delete;

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

COMPLEX_DEF_FILTER_TRAITS(complex, ExampleFilter2, "1307bbbc-112d-4aaa-941f-58253787b17e");
