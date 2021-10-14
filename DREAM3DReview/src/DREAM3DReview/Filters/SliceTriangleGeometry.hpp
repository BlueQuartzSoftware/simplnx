#pragma once

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @class SliceTriangleGeometry
 * @brief This filter will ....
 */
class COMPLEX_EXPORT SliceTriangleGeometry : public IFilter
{
public:
  SliceTriangleGeometry() = default;
  ~SliceTriangleGeometry() noexcept override = default;

  SliceTriangleGeometry(const SliceTriangleGeometry&) = delete;
  SliceTriangleGeometry(SliceTriangleGeometry&&) noexcept = delete;

  SliceTriangleGeometry& operator=(const SliceTriangleGeometry&) = delete;
  SliceTriangleGeometry& operator=(SliceTriangleGeometry&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_SliceDirection_Key = "SliceDirection";
  static inline constexpr StringLiteral k_SliceRange_Key = "SliceRange";
  static inline constexpr StringLiteral k_Zstart_Key = "Zstart";
  static inline constexpr StringLiteral k_Zend_Key = "Zend";
  static inline constexpr StringLiteral k_SliceResolution_Key = "SliceResolution";
  static inline constexpr StringLiteral k_HaveRegionIds_Key = "HaveRegionIds";
  static inline constexpr StringLiteral k_CADDataContainerName_Key = "CADDataContainerName";
  static inline constexpr StringLiteral k_RegionIdArrayPath_Key = "RegionIdArrayPath";
  static inline constexpr StringLiteral k_SliceDataContainerName_Key = "SliceDataContainerName";
  static inline constexpr StringLiteral k_EdgeAttributeMatrixName_Key = "EdgeAttributeMatrixName";
  static inline constexpr StringLiteral k_SliceIdArrayName_Key = "SliceIdArrayName";
  static inline constexpr StringLiteral k_SliceAttributeMatrixName_Key = "SliceAttributeMatrixName";

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

COMPLEX_DEF_FILTER_TRAITS(complex::SliceTriangleGeometry, "c6d42b50-5860-52ed-bde4-0f7bebb8623b");
