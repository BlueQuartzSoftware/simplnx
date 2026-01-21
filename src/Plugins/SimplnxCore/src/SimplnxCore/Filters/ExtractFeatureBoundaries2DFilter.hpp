#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ExtractFeatureBoundaries2DFilter
 * @brief This filter extracts 2D feature boundaries from an Image Geometry and creates an Edge Geometry.
 */
class SIMPLNXCORE_EXPORT ExtractFeatureBoundaries2DFilter : public IFilter
{
public:
  ExtractFeatureBoundaries2DFilter() = default;
  ~ExtractFeatureBoundaries2DFilter() noexcept override = default;

  ExtractFeatureBoundaries2DFilter(const ExtractFeatureBoundaries2DFilter&) = delete;
  ExtractFeatureBoundaries2DFilter(ExtractFeatureBoundaries2DFilter&&) noexcept = delete;

  ExtractFeatureBoundaries2DFilter& operator=(const ExtractFeatureBoundaries2DFilter&) = delete;
  ExtractFeatureBoundaries2DFilter& operator=(ExtractFeatureBoundaries2DFilter&&) noexcept = delete;

  // Parameter Keys
  static inline constexpr StringLiteral k_InputImageGeometryPath_Key = "input_image_geometry_path";
  static inline constexpr StringLiteral k_FeatureIdsArrayPath_Key = "feature_ids_array_path";
  static inline constexpr StringLiteral k_OutputEdgeGeometryPath_Key = "output_edge_geometry_path";
  static inline constexpr StringLiteral k_ZValueChoice_Key = "z_value_choice_index";
  static inline constexpr StringLiteral k_CustomZValue_Key = "custom_z_value";
  static inline constexpr StringLiteral k_ExtractVirtualSampleEdges_Key = "extract_virtual_sample_edges";

  // Z Value Choice indices
  enum class ZValueChoiceType : uint64
  {
    UseMinZValue = 0,
    UseMaxZValue = 1,
    UseCustomZValue = 2
  };

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
   * The Initial version should always be 1.
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

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ExtractFeatureBoundaries2DFilter, "e7d2f9b8-4a3c-5d1e-8f6a-9b0c2d3e4f5a");
