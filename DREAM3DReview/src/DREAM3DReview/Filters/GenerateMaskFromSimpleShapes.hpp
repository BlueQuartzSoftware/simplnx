#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class GenerateMaskFromSimpleShapes
 * @brief This filter will ....
 */
class DREAM3DREVIEW_EXPORT GenerateMaskFromSimpleShapes : public IFilter
{
public:
  GenerateMaskFromSimpleShapes() = default;
  ~GenerateMaskFromSimpleShapes() noexcept override = default;

  GenerateMaskFromSimpleShapes(const GenerateMaskFromSimpleShapes&) = delete;
  GenerateMaskFromSimpleShapes(GenerateMaskFromSimpleShapes&&) noexcept = delete;

  GenerateMaskFromSimpleShapes& operator=(const GenerateMaskFromSimpleShapes&) = delete;
  GenerateMaskFromSimpleShapes& operator=(GenerateMaskFromSimpleShapes&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_MaskShape_Key = "MaskShape";
  static inline constexpr StringLiteral k_MaskArrayPath_Key = "MaskArrayPath";
  static inline constexpr StringLiteral k_CentersArrayPath_Key = "CentersArrayPath";
  static inline constexpr StringLiteral k_AxesLengthArrayPath_Key = "AxesLengthArrayPath";
  static inline constexpr StringLiteral k_BoxDimensionsArrayPath_Key = "BoxDimensionsArrayPath";
  static inline constexpr StringLiteral k_CylinderRadiusArrayPath_Key = "CylinderRadiusArrayPath";
  static inline constexpr StringLiteral k_CylinderHeightArrayPath_Key = "CylinderHeightArrayPath";

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

COMPLEX_DEF_FILTER_TRAITS(complex, GenerateMaskFromSimpleShapes, "e4d6fda0-1143-56cc-9d00-c09a94e2f501");
