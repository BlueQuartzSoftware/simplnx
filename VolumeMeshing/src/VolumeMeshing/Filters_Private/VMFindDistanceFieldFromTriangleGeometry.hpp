#pragma once

#include "VolumeMeshing/VolumeMeshing_export.hpp"

#include "complex/Filter/FilterTraits.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{
/**
 * @class VMFindDistanceFieldFromTriangleGeometry
 * @brief This filter will ....
 */
class VOLUMEMESHING_EXPORT VMFindDistanceFieldFromTriangleGeometry : public IFilter
{
public:
  VMFindDistanceFieldFromTriangleGeometry() = default;
  ~VMFindDistanceFieldFromTriangleGeometry() noexcept override = default;

  VMFindDistanceFieldFromTriangleGeometry(const VMFindDistanceFieldFromTriangleGeometry&) = delete;
  VMFindDistanceFieldFromTriangleGeometry(VMFindDistanceFieldFromTriangleGeometry&&) noexcept = delete;

  VMFindDistanceFieldFromTriangleGeometry& operator=(const VMFindDistanceFieldFromTriangleGeometry&) = delete;
  VMFindDistanceFieldFromTriangleGeometry& operator=(VMFindDistanceFieldFromTriangleGeometry&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_DistanceFieldType_Key = "distance_field_type";
  static inline constexpr StringLiteral k_Spacing_Key = "spacing";
  static inline constexpr StringLiteral k_StoreClosestTriangle_Key = "store_closest_triangle";
  static inline constexpr StringLiteral k_TriangleDataContainerName_Key = "triangle_data_container_name";
  static inline constexpr StringLiteral k_ImageDataContainerName_Key = "image_data_container_name";
  static inline constexpr StringLiteral k_CellAttributeMatrixName_Key = "cell_attribute_matrix_name";
  static inline constexpr StringLiteral k_SignedDistanceFieldName_Key = "signed_distance_field_name";
  static inline constexpr StringLiteral k_ClosestTriangleName_Key = "closest_triangle_name";

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

COMPLEX_DEF_FILTER_TRAITS(complex, VMFindDistanceFieldFromTriangleGeometry, "b90d6860-4921-4586-b424-66658f281d8b");
