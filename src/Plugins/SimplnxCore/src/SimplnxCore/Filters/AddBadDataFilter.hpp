#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class AddBadDataFilter
 * @brief This filter will ....
 */
class SIMPLNXCORE_EXPORT AddBadDataFilter : public IFilter
{
public:
  AddBadDataFilter() = default;
  ~AddBadDataFilter() noexcept override = default;

  AddBadDataFilter(const AddBadDataFilter&) = delete;
  AddBadDataFilter(AddBadDataFilter&&) noexcept = delete;

  AddBadDataFilter& operator=(const AddBadDataFilter&) = delete;
  AddBadDataFilter& operator=(AddBadDataFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_PoissonNoise_Key = "poisson_noise";
  static inline constexpr StringLiteral k_PoissonVolFraction_Key = "poisson_vol_fraction";
  static inline constexpr StringLiteral k_BoundaryNoise_Key = "boundary_noise";
  static inline constexpr StringLiteral k_BoundaryVolFraction_Key = "boundary_vol_fraction";
  static inline constexpr StringLiteral k_GBEuclideanDistancesArrayPath_Key = "gb_euclidean_distances_array_path";
  static inline constexpr StringLiteral k_ImageGeometryPath_Key = "image_geometry_path";
  static inline constexpr StringLiteral k_UseSeed_Key = "use_seed";
  static inline constexpr StringLiteral k_SeedValue_Key = "seed_value";
  static inline constexpr StringLiteral k_SeedArrayName_Key = "seed_array_name";

  /**
   * @brief Reads SIMPL json and converts it simplnx Arguments.
   * @param json
   * @return Result<Arguments>
   */
  static Result<Arguments> FromSIMPLJson(const nlohmann::json& json);

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
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, AddBadDataFilter, "f44c66d1-a095-4e33-871c-c7699d89a011");
/* LEGACY UUID FOR THIS FILTER ac99b706-d1e0-5f78-9246-fbbe1efd93d2 */
