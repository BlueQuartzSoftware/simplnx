#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class FindCAxisLocationsFilter
 * @brief This filter determines the direction of the C-axis for each Element by applying the quaternion of the Element to the <001> direction, which is the C-axis for Hexagonal materials. This will
 * tell where the C-axis of the Element sits in the sample reference frame.
 */
class ORIENTATIONANALYSIS_EXPORT FindCAxisLocationsFilter : public IFilter
{
public:
  FindCAxisLocationsFilter() = default;
  ~FindCAxisLocationsFilter() noexcept override = default;

  FindCAxisLocationsFilter(const FindCAxisLocationsFilter&) = delete;
  FindCAxisLocationsFilter(FindCAxisLocationsFilter&&) noexcept = delete;

  FindCAxisLocationsFilter& operator=(const FindCAxisLocationsFilter&) = delete;
  FindCAxisLocationsFilter& operator=(FindCAxisLocationsFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_QuatsArrayPath_Key = "quats_array_path";
  static inline constexpr StringLiteral k_CellPhasesArrayPath_Key = "cell_phases_array_path";
  static inline constexpr StringLiteral k_CrystalStructuresArrayPath_Key = "crystal_structures_array_path";
  static inline constexpr StringLiteral k_CAxisLocationsArrayName_Key = "c_axis_locations_array_name";

  /**
   * @brief Reads SIMPL json and converts it complex Arguments.
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
} // namespace complex

COMPLEX_DEF_FILTER_TRAITS(complex, FindCAxisLocationsFilter, "a51c257a-ddc1-499a-9b21-f2d25a19d098");
/* LEGACY UUID FOR THIS FILTER 68ae7b7e-b9f7-5799-9f82-ce21d0ccd55e */
