#pragma once

#include "Core/Core_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class InitializeData
 * @brief This filter will ....
 */
class CORE_EXPORT InitializeData : public IFilter
{
public:
  InitializeData() = default;
  ~InitializeData() noexcept override = default;

  InitializeData(const InitializeData&) = delete;
  InitializeData(InitializeData&&) noexcept = delete;

  InitializeData& operator=(const InitializeData&) = delete;
  InitializeData& operator=(InitializeData&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_CellAttributeMatrixPaths_Key = "CellAttributeMatrixPaths";
  static inline constexpr StringLiteral k_XMin_Key = "XMin";
  static inline constexpr StringLiteral k_YMin_Key = "YMin";
  static inline constexpr StringLiteral k_ZMin_Key = "ZMin";
  static inline constexpr StringLiteral k_XMax_Key = "XMax";
  static inline constexpr StringLiteral k_YMax_Key = "YMax";
  static inline constexpr StringLiteral k_ZMax_Key = "ZMax";
  static inline constexpr StringLiteral k_InitType_Key = "InitType";
  static inline constexpr StringLiteral k_InitValue_Key = "InitValue";
  static inline constexpr StringLiteral k_InitRange_Key = "InitRange";

  /**
   * @brief Returns the name of the filter.
   * @return
   */
  std::string name() const override;

  /**
   * @brief Returns the C++ classname of this filter.
   * @return
   */
  std::string className() const override;

  /**
   * @brief Returns the uuid of the filter.
   * @return
   */
  Uuid uuid() const override;

  /**
   * @brief Returns the human readable name of the filter.
   * @return
   */
  std::string humanName() const override;

  /**
   * @brief Returns the default tags for this filter.
   * @return
   */
  std::vector<std::string> defaultTags() const override;

  /**
   * @brief Returns the parameters of the filter (i.e. its inputs)
   * @return
   */
  Parameters parameters() const override;

  /**
   * @brief Returns a copy of the filter.
   * @return
   */
  UniquePointer clone() const override;

protected:
  /**
   * @brief Takes in a DataStructure and checks that the filter can be run on it with the given arguments.
   * Returns any warnings/errors. Also returns the changes that would be applied to the DataStructure.
   * Some parts of the actions may not be completely filled out if all the required information is not available at preflight time.
   * @param ds The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<OutputActions> preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param ds The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, InitializeData, "19aacceb-b445-5753-8dcb-369f824f63d7");
