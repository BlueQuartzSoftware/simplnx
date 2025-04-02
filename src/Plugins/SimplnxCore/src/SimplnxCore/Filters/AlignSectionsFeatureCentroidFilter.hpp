#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class AlignSectionsFeatureCentroidFilter
 *
 * @brief This filter will ....
 */
class SIMPLNXCORE_EXPORT AlignSectionsFeatureCentroidFilter : public IFilter
{
public:
  AlignSectionsFeatureCentroidFilter() = default;
  ~AlignSectionsFeatureCentroidFilter() noexcept override = default;

  AlignSectionsFeatureCentroidFilter(const AlignSectionsFeatureCentroidFilter&) = delete;
  AlignSectionsFeatureCentroidFilter(AlignSectionsFeatureCentroidFilter&&) noexcept = delete;

  AlignSectionsFeatureCentroidFilter& operator=(const AlignSectionsFeatureCentroidFilter&) = delete;
  AlignSectionsFeatureCentroidFilter& operator=(AlignSectionsFeatureCentroidFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_UseReferenceSlice_Key = "use_reference_slice";
  static inline constexpr StringLiteral k_ReferenceSlice_Key = "reference_slice";
  static inline constexpr StringLiteral k_MaskArrayPath_Key = "mask_array_path";
  static inline constexpr StringLiteral k_SelectedImageGeometryPath_Key = "input_image_geometry_path";

  static inline constexpr StringLiteral k_StoreAlignmentShifts_Key = "store_alignment_shifts";
  static inline constexpr StringLiteral k_AlignmentAMName_Key = "alignment_attribute_matrix_name";
  static inline constexpr StringLiteral k_SlicesArrayName_Key = "slices_array_name";
  static inline constexpr StringLiteral k_RelativeShiftsArrayName_Key = "relative_shifts_array_name";
  static inline constexpr StringLiteral k_CumulativeShiftsArrayName_Key = "cumulative_shifts_array_name";
  static inline constexpr StringLiteral k_CentroidsArrayName_Key = "centroids_array_name";

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
   * @brief Returns the human-readable name of the filter.
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
   * @brief Returns parameters version integer.
   * Initial version should always be 1.
   * Should be incremented everytime the parameters change.
   * @return VersionType
   */
  VersionType parametersVersion() const override;

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
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param messageHandler The MessageHandler object
   * @param shouldCancel Atomic boolean value that can be checked to cancel the filter
   * @param executionContext The ExecutionContext that can be used to determine the correct absolute path from a relative path
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                const ExecutionContext& executionContext) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments. Returns any warnings/errors.
   * On failure, there is no guarantee that the DataStructure is in a correct state.
   * @param dataStructure The input DataStructure instance
   * @param filterArgs These are the input values for each parameter that is required for the filter
   * @param pipelineNode The node in the pipeline that is being executed
   * @param messageHandler The MessageHandler object
   * @param shouldCancel Atomic boolean value that can be checked to cancel the filter
   * @param executionContext The ExecutionContext that can be used to determine the correct absolute path from a relative path
   * @return Returns a Result object with error or warning values if any of those occurred during execution of this function
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                       const ExecutionContext& executionContext) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, AlignSectionsFeatureCentroidFilter, "b83f9bae-9ccf-4932-96c3-7f2fdb091452");
