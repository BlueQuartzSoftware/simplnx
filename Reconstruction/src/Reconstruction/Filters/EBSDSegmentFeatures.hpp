#pragma once

#include "Reconstruction/Reconstruction_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class EBSDSegmentFeatures
 * @brief This filter will ....
 */
class RECONSTRUCTION_EXPORT EBSDSegmentFeatures : public IFilter
{
public:
  EBSDSegmentFeatures() = default;
  ~EBSDSegmentFeatures() noexcept override = default;

  EBSDSegmentFeatures(const EBSDSegmentFeatures&) = delete;
  EBSDSegmentFeatures(EBSDSegmentFeatures&&) noexcept = delete;

  EBSDSegmentFeatures& operator=(const EBSDSegmentFeatures&) = delete;
  EBSDSegmentFeatures& operator=(EBSDSegmentFeatures&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_MisorientationTolerance_Key = "MisorientationTolerance";
  static inline constexpr StringLiteral k_UseGoodVoxels_Key = "UseGoodVoxels";
  static inline constexpr StringLiteral k_QuatsArrayPath_Key = "QuatsArrayPath";
  static inline constexpr StringLiteral k_CellPhasesArrayPath_Key = "CellPhasesArrayPath";
  static inline constexpr StringLiteral k_GoodVoxelsArrayPath_Key = "GoodVoxelsArrayPath";
  static inline constexpr StringLiteral k_CrystalStructuresArrayPath_Key = "CrystalStructuresArrayPath";
  static inline constexpr StringLiteral k_FeatureIdsArrayName_Key = "FeatureIdsArrayName";
  static inline constexpr StringLiteral k_CellFeatureAttributeMatrixName_Key = "CellFeatureAttributeMatrixName";
  static inline constexpr StringLiteral k_ActiveArrayName_Key = "ActiveArrayName";

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

COMPLEX_DEF_FILTER_TRAITS(complex, EBSDSegmentFeatures, "4b6297ce-e2f0-56ed-98c6-e5a68fa24943");
