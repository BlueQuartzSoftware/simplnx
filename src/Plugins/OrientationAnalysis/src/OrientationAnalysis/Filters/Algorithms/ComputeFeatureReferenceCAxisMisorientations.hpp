#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"

namespace nx::core
{

/**
 * @struct ComputeFeatureReferenceCAxisMisorientationsInputValues
 * @brief Identifies feature-reference c-axis misorientation inputs.
 */
struct ORIENTATIONANALYSIS_EXPORT ComputeFeatureReferenceCAxisMisorientationsInputValues
{
  DataPath ImageGeometryPath;
  DataPath FeatureIdsArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath QuatsArrayPath;
  DataPath AvgCAxesArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath FeatureReferenceCAxisMisorientationsArrayPath;
  DataPath FeatureAvgCAxisMisorientationsArrayPath;
  DataPath FeatureStdevCAxisMisorientationsArrayPath;
};

/**
 * @class ComputeFeatureReferenceCAxisMisorientations
 * @brief Computes c-axis misorientation to each feature reference.
 *
 * Hexagonal cells compare their c axes to the feature-average c axis. The
 * executor writes cell angles in degrees and calculates feature mean and
 * population standard deviation. It uses local feature and ensemble data with
 * one Z-slice of cell data.
 */
class ORIENTATIONANALYSIS_EXPORT ComputeFeatureReferenceCAxisMisorientations
{
public:
  /**
   * @brief Initializes feature-reference c-axis misorientation computation.
   * @param dataStructure Provides selected arrays and the geometry.
   * @param mesgHandler Supplies progress messages.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Identifies selected arrays.
   * @pre dataStructure, mesgHandler, shouldCancel, and inputValues outlive this
   *      executor.
   */
  ComputeFeatureReferenceCAxisMisorientations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                              ComputeFeatureReferenceCAxisMisorientationsInputValues* inputValues);
  /**
   * @brief Destroys the feature-reference c-axis executor.
   */
  ~ComputeFeatureReferenceCAxisMisorientations() noexcept;

  ComputeFeatureReferenceCAxisMisorientations(const ComputeFeatureReferenceCAxisMisorientations&) = delete;
  ComputeFeatureReferenceCAxisMisorientations(ComputeFeatureReferenceCAxisMisorientations&&) noexcept = delete;
  ComputeFeatureReferenceCAxisMisorientations& operator=(const ComputeFeatureReferenceCAxisMisorientations&) = delete;
  ComputeFeatureReferenceCAxisMisorientations& operator=(ComputeFeatureReferenceCAxisMisorientations&&) noexcept = delete;

  /**
   * @brief Computes feature-reference c-axis misorientations.
   * @pre Positive cell feature and phase IDs are within their selected arrays.
   * @return An error if no hexagonal phase exists, or a warning for skipped
   *         non-hexagonal phases.
   *
   * Cancellation returns success with completed slices and feature outputs
   * preserved.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeFeatureReferenceCAxisMisorientationsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
