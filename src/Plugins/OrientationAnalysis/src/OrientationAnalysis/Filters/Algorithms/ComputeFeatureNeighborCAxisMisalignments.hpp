#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

/**
 * @struct ComputeFeatureNeighborCAxisMisalignmentsInputValues
 * @brief Identifies neighbor c-axis misalignment inputs.
 */
struct ORIENTATIONANALYSIS_EXPORT ComputeFeatureNeighborCAxisMisalignmentsInputValues
{
  bool FindAvgMisals;
  DataPath NeighborListArrayPath;
  DataPath AvgQuatsArrayPath;
  DataPath FeaturePhasesArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath CAxisMisalignmentListArrayName;
  DataPath AvgCAxisMisalignmentsArrayName;
};

/**
 * @class ComputeFeatureNeighborCAxisMisalignments
 * @brief Computes c-axis misalignments between feature neighbors.
 *
 * Same-phase hexagonal neighbors produce angles in degrees. Other pairs produce
 * NaN values. Feature phases and average quaternions stay local because the
 * neighbor traversal uses random feature indices.
 */
class ORIENTATIONANALYSIS_EXPORT ComputeFeatureNeighborCAxisMisalignments
{
public:
  /**
   * @brief Initializes neighbor c-axis misalignment computation.
   * @param dataStructure Provides selected arrays.
   * @param mesgHandler Supplies the filter message handler.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Identifies selected arrays and options.
   * @pre dataStructure, mesgHandler, shouldCancel, and inputValues outlive this
   *      executor.
   */
  ComputeFeatureNeighborCAxisMisalignments(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                           ComputeFeatureNeighborCAxisMisalignmentsInputValues* inputValues);
  /**
   * @brief Destroys the neighbor c-axis misalignment executor.
   */
  ~ComputeFeatureNeighborCAxisMisalignments() noexcept;

  ComputeFeatureNeighborCAxisMisalignments(const ComputeFeatureNeighborCAxisMisalignments&) = delete;
  ComputeFeatureNeighborCAxisMisalignments(ComputeFeatureNeighborCAxisMisalignments&&) noexcept = delete;
  ComputeFeatureNeighborCAxisMisalignments& operator=(const ComputeFeatureNeighborCAxisMisalignments&) = delete;
  ComputeFeatureNeighborCAxisMisalignments& operator=(ComputeFeatureNeighborCAxisMisalignments&&) noexcept = delete;

  /**
   * @brief Computes neighbor c-axis misalignments.
   * @pre Feature phase IDs are within the crystal-structure array.
   * @return An error if no hexagonal phase exists, or a warning for skipped
   *         non-hexagonal phases.
   *
   * Cancellation returns success with completed feature lists preserved.
   * Current bulk-I/O Result values are not inspected.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeFeatureNeighborCAxisMisalignmentsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
