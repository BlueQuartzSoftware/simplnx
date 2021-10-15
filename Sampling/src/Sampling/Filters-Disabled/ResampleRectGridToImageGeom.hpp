#pragma once

#include "Sampling/Sampling_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class ResampleRectGridToImageGeom
 * @brief This filter will ....
 */
class SAMPLING_EXPORT ResampleRectGridToImageGeom : public IFilter
{
public:
  ResampleRectGridToImageGeom() = default;
  ~ResampleRectGridToImageGeom() noexcept override = default;

  ResampleRectGridToImageGeom(const ResampleRectGridToImageGeom&) = delete;
  ResampleRectGridToImageGeom(ResampleRectGridToImageGeom&&) noexcept = delete;

  ResampleRectGridToImageGeom& operator=(const ResampleRectGridToImageGeom&) = delete;
  ResampleRectGridToImageGeom& operator=(ResampleRectGridToImageGeom&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_RectilinearGridPath_Key = "RectilinearGridPath";
  static inline constexpr StringLiteral k_SelectedDataArrayPaths_Key = "SelectedDataArrayPaths";
  static inline constexpr StringLiteral k_RectGridGeometryDesc_Key = "RectGridGeometryDesc";
  static inline constexpr StringLiteral k_Dimensions_Key = "Dimensions";
  static inline constexpr StringLiteral k_CreatedGeometryDescription_Key = "CreatedGeometryDescription";
  static inline constexpr StringLiteral k_ImageGeometryPath_Key = "ImageGeometryPath";
  static inline constexpr StringLiteral k_ImageGeomCellAttributeMatrix_Key = "ImageGeomCellAttributeMatrix";

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

COMPLEX_DEF_FILTER_TRAITS(complex, ResampleRectGridToImageGeom, "75e973cb-3225-55f1-945a-622a21ea464c");
