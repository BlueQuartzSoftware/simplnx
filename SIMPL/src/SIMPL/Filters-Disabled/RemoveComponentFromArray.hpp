#pragma once

#include "SIMPL/SIMPL_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class RemoveComponentFromArray
 * @brief This filter will ....
 */
class SIMPL_EXPORT RemoveComponentFromArray : public IFilter
{
public:
  RemoveComponentFromArray() = default;
  ~RemoveComponentFromArray() noexcept override = default;

  RemoveComponentFromArray(const RemoveComponentFromArray&) = delete;
  RemoveComponentFromArray(RemoveComponentFromArray&&) noexcept = delete;

  RemoveComponentFromArray& operator=(const RemoveComponentFromArray&) = delete;
  RemoveComponentFromArray& operator=(RemoveComponentFromArray&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_CompNumber_Key = "CompNumber";
  static inline constexpr StringLiteral k_SelectedArrayPath_Key = "SelectedArrayPath";
  static inline constexpr StringLiteral k_NewArrayArrayName_Key = "NewArrayArrayName";
  static inline constexpr StringLiteral k_ReducedArrayArrayName_Key = "ReducedArrayArrayName";
  static inline constexpr StringLiteral k_SaveRemovedComponent_Key = "SaveRemovedComponent";

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
  PreflightResult preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param ds The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& data, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;
};
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, RemoveComponentFromArray, "c29d1d17-c2e8-4c85-8272-d16be01adbf5");
