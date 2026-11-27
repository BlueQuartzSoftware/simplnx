#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class BlendImageFilter
 * @brief Modulates IPF color values by a normalized confidence index to produce
 * a quality-weighted color image. Low-CI points darken toward black; high-CI
 * points retain their full IPF color.
 */
class ORIENTATIONANALYSIS_EXPORT BlendImageFilter : public IFilter
{
public:
  BlendImageFilter() = default;
  ~BlendImageFilter() noexcept override = default;

  BlendImageFilter(const BlendImageFilter&) = delete;
  BlendImageFilter(BlendImageFilter&&) noexcept = delete;

  BlendImageFilter& operator=(const BlendImageFilter&) = delete;
  BlendImageFilter& operator=(BlendImageFilter&&) noexcept = delete;

  // Parameter Keys
  static constexpr StringLiteral k_CellIPFColorsArrayPath_Key = "cell_ipf_colors_array_path";
  static constexpr StringLiteral k_CIArrayPath_Key = "ci_array_path";
  static constexpr StringLiteral k_OutputArrayName_Key = "output_array_name";
  // Blend Mode selector with three states (Basic, Advanced, Advanced with Background Leveling),
  // which reproduce the optional behavior of the IDL makeebsdimage_ang routine. The leveling state
  // is a dedicated choice state so its Image Geometry input is only required when leveling is used.
  static constexpr StringLiteral k_BlendMode_Key = "blend_mode";
  static constexpr StringLiteral k_UseBlackout_Key = "use_blackout";
  static constexpr StringLiteral k_CriterionArrayPath_Key = "criterion_array_path";
  static constexpr StringLiteral k_BlackoutThreshold_Key = "blackout_threshold";
  static constexpr StringLiteral k_InputImageGeometryPath_Key = "input_image_geometry_path";
  static constexpr StringLiteral k_LevelingDegree_Key = "leveling_degree";
  static constexpr StringLiteral k_UseFinalRescale_Key = "use_final_rescale";

  /**
   * @brief Returns the name of the filter.
   */
  std::string name() const override;

  /**
   * @brief Returns the C++ classname of this filter.
   */
  std::string className() const override;

  /**
   * @brief Returns the uuid of the filter.
   */
  Uuid uuid() const override;

  /**
   * @brief Returns the human-readable name of the filter.
   */
  std::string humanName() const override;

  /**
   * @brief Returns the default tags for this filter.
   */
  std::vector<std::string> defaultTags() const override;

  /**
   * @brief Returns the parameters of the filter (i.e. its inputs).
   */
  Parameters parameters() const override;

  /**
   * @brief Returns parameters version integer.
   * The Initial version should always be 1.
   * Should be incremented everytime the parameters change.
   */
  VersionType parametersVersion() const override;

  /**
   * @brief Returns a copy of the filter.
   */
  UniquePointer clone() const override;

protected:
  /**
   * @brief Takes in a DataStructure and checks that the filter can be run on it with the given arguments.
   * Returns any warnings/errors. Also returns the changes that would be applied to the DataStructure.
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                const ExecutionContext& executionContext) const override;

  /**
   * @brief Applies the filter's algorithm to the DataStructure with the given arguments.
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                       const ExecutionContext& executionContext) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, BlendImageFilter, "2a5fe82f-dcf5-41ca-bdef-d9da63ad2089");
