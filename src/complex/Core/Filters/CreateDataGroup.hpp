#pragma once

#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/Arguments.hpp"
#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Filter/Parameters.hpp"
#include "complex/complex_export.hpp"

#include <string>

namespace complex
{
class COMPLEX_EXPORT CreateDataGroup : public IFilter
{
public:
  CreateDataGroup() = default;
  ~CreateDataGroup() noexcept override = default;

  CreateDataGroup(const CreateDataGroup&) = delete;
  CreateDataGroup(CreateDataGroup&&) noexcept = delete;

  CreateDataGroup& operator=(const CreateDataGroup&) = delete;
  CreateDataGroup& operator=(CreateDataGroup&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_DataObjectPath = "Data_Object_Path";
  /**
   * @brief Returns the name of this filter
   * @return
   */
  [[nodiscard]] std::string name() const override;

  /**
   * @brief Returns the UUID of this filter. All filters have a unique UUID
   * @return
   */
  [[nodiscard]] Uuid uuid() const override;

  /**
   * @brief Returns a 'human readable' string that represents the filter name.
   * @return
   */
  [[nodiscard]] std::string humanName() const override;

  /**
   * @brief Returns the parameter descriptors for this filter
   * @return
   */
  [[nodiscard]] Parameters parameters() const override;

  /**
   * @brief Creates a copy of this filter.
   * @return
   */
  [[nodiscard]] UniquePointer clone() const override;

protected:
  /**
   * @brief Internal implementation of the 'preflight' function. This gives the filter an opportunity to sanity check
   * the inputs to the filter
   * @param dataStructure The input data structure
   * @param filterParameterValues The value of each parameter that is needed by this filter
   * @param messageHandler = {}
   * @return
   */
  Result<OutputActions> preflightImpl(const DataStructure& dataStructure, const Arguments& filterParameterValues, const MessageHandler& messageHandler = {}) const override;

  /**
   * @brief Internal implementation of the 'execute' function which actually will run the filter's algorithm on the
   * supplied input parameters
   * @param dataStructure The input data structure
   * @param filterParameterValues The value of each parameter that is needed by this filter
   * @param messageHandler = {}
   * @return
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterParameterValues, const MessageHandler& messageHandler = {}) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex::CreateDataGroup, "e7d2f9b8-4131-4b28-a843-ea3c6950f101");
