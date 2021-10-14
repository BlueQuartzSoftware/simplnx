#pragma once

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
/**
 * @class ScaleVolume
 * @brief This filter will ....
 */
class COMPLEX_EXPORT ScaleVolume : public IFilter
{
public:
  ScaleVolume() = default;
  ~ScaleVolume() noexcept override = default;

  ScaleVolume(const ScaleVolume&) = delete;
  ScaleVolume(ScaleVolume&&) noexcept = delete;

  ScaleVolume& operator=(const ScaleVolume&) = delete;
  ScaleVolume& operator=(ScaleVolume&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_ScaleFactor_Key = "ScaleFactor";
  static inline constexpr StringLiteral k_ApplyToVoxelVolume_Key = "ApplyToVoxelVolume";
  static inline constexpr StringLiteral k_DataContainerName_Key = "DataContainerName";
  static inline constexpr StringLiteral k_ApplyToSurfaceMesh_Key = "ApplyToSurfaceMesh";
  static inline constexpr StringLiteral k_SurfaceDataContainerName_Key = "SurfaceDataContainerName";

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

COMPLEX_DEF_FILTER_TRAITS(complex::ScaleVolume, "390ce8ba-47de-5544-840f-3e70a6c9ebfd");
