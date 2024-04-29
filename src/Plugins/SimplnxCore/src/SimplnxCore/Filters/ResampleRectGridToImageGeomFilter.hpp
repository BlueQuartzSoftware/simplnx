#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ResampleRectGridToImageGeomFilter
 * @brief This filter will re-sample an existing RectilinearGrid onto a regular grid (Image Geometry) and copy cell data into the newly created Image Geometry Data Container during the process.
 */
class SIMPLNXCORE_EXPORT ResampleRectGridToImageGeomFilter : public IFilter
{
public:
  ResampleRectGridToImageGeomFilter() = default;
  ~ResampleRectGridToImageGeomFilter() noexcept override = default;

  ResampleRectGridToImageGeomFilter(const ResampleRectGridToImageGeomFilter&) = delete;
  ResampleRectGridToImageGeomFilter(ResampleRectGridToImageGeomFilter&&) noexcept = delete;

  ResampleRectGridToImageGeomFilter& operator=(const ResampleRectGridToImageGeomFilter&) = delete;
  ResampleRectGridToImageGeomFilter& operator=(ResampleRectGridToImageGeomFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_RectilinearGridPath_Key = "rectilinear_grid_path";
  static inline constexpr StringLiteral k_SelectedDataArrayPaths_Key = "input_data_array_paths";
  static inline constexpr StringLiteral k_Dimensions_Key = "dimensions";
  static inline constexpr StringLiteral k_ImageGeometryPath_Key = "input_image_geometry_path";
  static inline constexpr StringLiteral k_ImageGeomCellAttributeMatrixName_Key = "image_geom_cell_attribute_matrix_name";

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
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param ds The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                       const std::atomic_bool& shouldCancel) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ResampleRectGridToImageGeomFilter, "28ed3258-41b2-45a0-8f37-6574264650f9");
/* LEGACY UUID FOR THIS FILTER 77befd69-4536-5856-9f81-02996d038f73 */
