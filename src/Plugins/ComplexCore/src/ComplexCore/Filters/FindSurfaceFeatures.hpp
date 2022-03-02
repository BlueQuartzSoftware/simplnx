#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class FindSurfaceFeatures
 * @brief This Filter determines whether a Feature touches an outer surface of the sample.
 * This is accomplished by simply querying the Feature owners of the Cells that sit at either.
 * Any Feature that owns one of those Cells is said to touch an outer surface and all other
 * Features are said to not touch an outer surface of the sample.
 *
 * This Filter determines whether a Feature touches an outer Surface of the sample volume. A
 * Feature is considered touching the Surface of the sample if either of the following conditions
 * are met:
 * Any cell location is xmin, xmax, ymin, ymax, zmin or zmax.
 * Any cell has Feature ID = 0 as a neighbor.
 *
 * The output of this filter is a Feature level array of booleans where 0=Interior/Not touching
 * and 1=Surface/Touching.
 *
 * Note: If there are voxels within the volume that have Feature ID=0 then any feature touching
 * those voxels will be considered a Surface feature.
 */
class COMPLEXCORE_EXPORT FindSurfaceFeatures : public IFilter
{
public:
  FindSurfaceFeatures() = default;
  ~FindSurfaceFeatures() noexcept override = default;

  FindSurfaceFeatures(const FindSurfaceFeatures&) = delete;
  FindSurfaceFeatures(FindSurfaceFeatures&&) noexcept = delete;

  FindSurfaceFeatures& operator=(const FindSurfaceFeatures&) = delete;
  FindSurfaceFeatures& operator=(FindSurfaceFeatures&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_MarkFeature0Neighbors = "MarkFeature0Neighbors";
  static inline constexpr StringLiteral k_FeatureGeometryPath_Key = "FeatureGeometryPath";
  static inline constexpr StringLiteral k_FeatureIdsArrayPath_Key = "FeatureIdsArrayPath";
  static inline constexpr StringLiteral k_SurfaceFeaturesArrayPath_Key = "SurfaceFeaturesArrayPath";

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

COMPLEX_DEF_FILTER_TRAITS(complex, FindSurfaceFeatures, "d2b0ae3d-686a-5dc0-a844-66bc0dc8f3cb");
