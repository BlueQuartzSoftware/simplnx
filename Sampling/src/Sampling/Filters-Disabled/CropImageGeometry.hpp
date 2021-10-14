#pragma once

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @class CropImageGeometry
 * @brief This filter will ....
 */
class COMPLEX_EXPORT CropImageGeometry : public IFilter
{
public:
  CropImageGeometry() = default;
  ~CropImageGeometry() noexcept override = default;

  CropImageGeometry(const CropImageGeometry&) = delete;
  CropImageGeometry(CropImageGeometry&&) noexcept = delete;

  CropImageGeometry& operator=(const CropImageGeometry&) = delete;
  CropImageGeometry& operator=(CropImageGeometry&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_XMin_Key = "XMin";
  static inline constexpr StringLiteral k_YMin_Key = "YMin";
  static inline constexpr StringLiteral k_ZMin_Key = "ZMin";
  static inline constexpr StringLiteral k_XMax_Key = "XMax";
  static inline constexpr StringLiteral k_YMax_Key = "YMax";
  static inline constexpr StringLiteral k_ZMax_Key = "ZMax";
  static inline constexpr StringLiteral k_OldBoxDimensions_Key = "OldBoxDimensions";
  static inline constexpr StringLiteral k_NewBoxDimensions_Key = "NewBoxDimensions";
  static inline constexpr StringLiteral k_UpdateOrigin_Key = "UpdateOrigin";
  static inline constexpr StringLiteral k_SaveAsNewDataContainer_Key = "SaveAsNewDataContainer";
  static inline constexpr StringLiteral k_NewDataContainerName_Key = "NewDataContainerName";
  static inline constexpr StringLiteral k_CellAttributeMatrixPath_Key = "CellAttributeMatrixPath";
  static inline constexpr StringLiteral k_RenumberFeatures_Key = "RenumberFeatures";
  static inline constexpr StringLiteral k_FeatureIdsArrayPath_Key = "FeatureIdsArrayPath";
  static inline constexpr StringLiteral k_CellFeatureAttributeMatrixPath_Key = "CellFeatureAttributeMatrixPath";

  /**
   * @brief Returns the name of the filter.
   * @return
   */
  std::string name() const override;

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

COMPLEX_DEF_FILTER_TRAITS(complex::CropImageGeometry, "e73ba983-fca2-5557-9189-ffd6d08bfac8");
