#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class GenerateVectorColorsFilter
 * @brief This filter will ....
 */
class SIMPLNXCORE_EXPORT GenerateVectorColorsFilter : public IFilter
{
public:
  GenerateVectorColorsFilter() = default;
  ~GenerateVectorColorsFilter() noexcept override = default;

  GenerateVectorColorsFilter(const GenerateVectorColorsFilter&) = delete;
  GenerateVectorColorsFilter(GenerateVectorColorsFilter&&) noexcept = delete;

  GenerateVectorColorsFilter& operator=(const GenerateVectorColorsFilter&) = delete;
  GenerateVectorColorsFilter& operator=(GenerateVectorColorsFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_UseMask_Key = "use_mask";
  static inline constexpr StringLiteral k_VectorsArrayPath_Key = "vectors_array_path";
  static inline constexpr StringLiteral k_MaskArrayPath_Key = "mask_array_path";
  static inline constexpr StringLiteral k_CellVectorColorsArrayName_Key = "cell_vector_colors_array_name";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, GenerateVectorColorsFilter, "1a35650d-a9f5-4ea2-af70-5b9ce894e45f");
/* LEGACY UUID FOR THIS FILTER ef28de7e-5bdd-57c2-9318-60ba0dfaf7bc */
