#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class NearestPointFuseRegularGridsFilter
 * @brief This filter will ....
 */
class SIMPLNXCORE_EXPORT NearestPointFuseRegularGridsFilter : public IFilter
{
public:
  NearestPointFuseRegularGridsFilter() = default;
  ~NearestPointFuseRegularGridsFilter() noexcept override = default;

  NearestPointFuseRegularGridsFilter(const NearestPointFuseRegularGridsFilter&) = delete;
  NearestPointFuseRegularGridsFilter(NearestPointFuseRegularGridsFilter&&) noexcept = delete;

  NearestPointFuseRegularGridsFilter& operator=(const NearestPointFuseRegularGridsFilter&) = delete;
  NearestPointFuseRegularGridsFilter& operator=(NearestPointFuseRegularGridsFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_ReferenceGeometryPath_Key = "input_reference_image_geometry__path";
  static inline constexpr StringLiteral k_SamplingGeometryPath_Key = "input_sampling_image_geometry_path";
  static inline constexpr StringLiteral k_ReferenceCellAttributeMatrixPath_Key = "reference_cell_attribute_matrix_path";
  static inline constexpr StringLiteral k_SamplingCellAttributeMatrixPath_Key = "sampling_cell_attribute_matrix_path";
  static inline constexpr StringLiteral k_UseFill_Key = "use_fill";
  static inline constexpr StringLiteral k_FillValue_Key = "fill_value";

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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, NearestPointFuseRegularGridsFilter, "c8746bf8-fa5f-48b0-8a47-11023402e543");
/* LEGACY UUID FOR THIS FILTER cbaf9e68-5ded-560c-9440-509289100ea8 */
