#pragma once

#include "complex/Common/StringLiteral.hpp"
#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
class COMPLEX_EXPORT CreateDataArray : public IFilter
{
public:
  CreateDataArray() = default;
  ~CreateDataArray() noexcept override = default;

  CreateDataArray(const CreateDataArray&) = delete;
  CreateDataArray(CreateDataArray&&) noexcept = delete;

  CreateDataArray& operator=(const CreateDataArray&) = delete;
  CreateDataArray& operator=(CreateDataArray&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_NumericType_Key = "numeric_type";
  static inline constexpr StringLiteral k_NumComps_Key = "component_count";
  static inline constexpr StringLiteral k_NumTuples_Key = "tuple_count";
  static inline constexpr StringLiteral k_DataPath_Key = "output_data_array";

  /**
   * @brief
   * @return
   */
  [[nodiscard]] std::string name() const override;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] Uuid uuid() const override;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] std::string humanName() const override;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] Parameters parameters() const override;

  /**
   * @brief
   * @return
   */
  [[nodiscard]] UniquePointer clone() const override;

protected:
  /**
   * @brief
   * @param data
   * @param args
   * @return
   */
  Result<OutputActions> preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler = {}) const override;

  /**
   * @brief
   * @param data
   * @param args
   * @return
   */
  Result<> executeImpl(DataStructure& data, const Arguments& args, const MessageHandler& messageHandler = {}) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex::CreateDataArray, "67041f9b-bdc6-4122-acc6-c9fe9280e90d");
