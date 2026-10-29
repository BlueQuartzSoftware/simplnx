#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/Filter/FilterTraits.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @class ComputeCAxisReferenceMisalignmentsFilter
 * @brief Computes the antipodal angle between each hexagonal c-axis and a sample-frame reference direction.
 */
class ORIENTATIONANALYSIS_EXPORT ComputeCAxisReferenceMisalignmentsFilter : public IFilter
{
public:
  /**
   * @brief Constructs the filter with default parameters.
   */
  ComputeCAxisReferenceMisalignmentsFilter() = default;
  /**
   * @brief Destroys the filter.
   */
  ~ComputeCAxisReferenceMisalignmentsFilter() noexcept override = default;

  ComputeCAxisReferenceMisalignmentsFilter(const ComputeCAxisReferenceMisalignmentsFilter&) = delete;
  ComputeCAxisReferenceMisalignmentsFilter(ComputeCAxisReferenceMisalignmentsFilter&&) noexcept = delete;

  ComputeCAxisReferenceMisalignmentsFilter& operator=(const ComputeCAxisReferenceMisalignmentsFilter&) = delete;
  ComputeCAxisReferenceMisalignmentsFilter& operator=(ComputeCAxisReferenceMisalignmentsFilter&&) noexcept = delete;

  // Parameter Keys
  static constexpr StringLiteral k_ReferenceDir_Key = "reference_dir";
  static constexpr StringLiteral k_UseMask_Key = "use_mask";
  static constexpr StringLiteral k_MaskArrayPath_Key = "mask_array_path";
  static constexpr StringLiteral k_QuatsArrayPath_Key = "quats_array_path";
  static constexpr StringLiteral k_PhasesArrayPath_Key = "phases_array_path";
  static constexpr StringLiteral k_CrystalStructuresArrayPath_Key = "crystal_structures_array_path";
  static constexpr StringLiteral k_CAxisReferenceMisalignmentsArrayName_Key = "c_axis_reference_misalignments_array_name";

  /**
   * @brief Returns the name of the filter.
   * @return Registered filter name.
   */
  std::string name() const override;

  /**
   * @brief Returns the C++ classname of this filter.
   * @return C++ class name.
   */
  std::string className() const override;

  /**
   * @brief Returns the uuid of the filter.
   * @return Unique filter identifier.
   */
  Uuid uuid() const override;

  /**
   * @brief Returns the human-readable name of the filter.
   * @return Name shown in the user interface.
   */
  std::string humanName() const override;

  /**
   * @brief Returns the default tags for this filter.
   * @return Search tags for crystallographic processing.
   */
  std::vector<std::string> defaultTags() const override;

  /**
   * @brief Defines the parameters and mask linkage.
   * @return Reference, mask, input selection, and output name parameters.
   */
  Parameters parameters() const override;

  /**
   * @brief Identifies the parameter schema version.
   * @return Version 1.
   */
  VersionType parametersVersion() const override;

  /**
   * @brief Returns a copy of the filter.
   * @return A new filter instance.
   */
  UniquePointer clone() const override;

protected:
  /**
   * @brief Validates the reference and tuple counts, then declares the sibling output.
   * @param dataStructure Contains the selected arrays.
   * @param filterArgs Supplies parameter values.
   * @param messageHandler Receives filter messages.
   * @param shouldCancel Signals cancellation.
   * @param executionContext Supplies the execution environment.
   * @return A float32 array creation action, or an error for invalid inputs.
   */
  PreflightResult preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                const ExecutionContext& executionContext) const override;

  /**
   * @brief Runs the shared chunked algorithm after output creation.
   * @param dataStructure Contains input arrays and the output array.
   * @param filterArgs Supplies parameter values.
   * @param pipelineNode Identifies this filter in the pipeline.
   * @param messageHandler Receives progress updates.
   * @param shouldCancel Signals cancellation at a chunk boundary.
   * @param executionContext Supplies the execution environment.
   * @return Algorithm errors and warnings. Errors or cancellation can leave partial output.
   */
  Result<> executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                       const ExecutionContext& executionContext) const override;
};
} // namespace nx::core

SIMPLNX_DEF_FILTER_TRAITS(nx::core, ComputeCAxisReferenceMisalignmentsFilter, "5d320484-c6be-41b0-89df-a8218acd4cc1");
