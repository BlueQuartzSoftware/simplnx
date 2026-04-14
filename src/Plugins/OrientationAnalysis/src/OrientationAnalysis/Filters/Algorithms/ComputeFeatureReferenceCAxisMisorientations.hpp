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
 * @brief Input values for the ComputeFeatureReferenceCAxisMisorientations algorithm.
 */
struct ORIENTATIONANALYSIS_EXPORT ComputeFeatureReferenceCAxisMisorientationsInputValues
{
  // Input Geometry
  DataPath ImageGeometryPath; ///< ImageGeom providing the voxel grid dimensions

  // Input Cell Data
  DataPath FeatureIdsArrayPath; ///< Cell-level Int32 feature ID per voxel
  DataPath CellPhasesArrayPath; ///< Cell-level Int32 phase index per voxel
  DataPath QuatsArrayPath;      ///< Cell-level Float32 quaternions (4 components)

  // Input Feature Data
  DataPath AvgCAxesArrayPath; ///< Feature-level Float32 average c-axis (3 components)

  // Input Ensemble Data
  DataPath CrystalStructuresArrayPath; ///< Ensemble-level UInt32 crystal structure Laue classes

  // Output Cell Data
  DataPath FeatureReferenceCAxisMisorientationsArrayPath; ///< Output: Cell-level Float32 c-axis misorientation (degrees)

  // Output Feature Data
  DataPath FeatureAvgCAxisMisorientationsArrayPath;   ///< Output: Feature-level Float32 average c-axis misorientation
  DataPath FeatureStdevCAxisMisorientationsArrayPath; ///< Output: Feature-level Float32 standard deviation
};

/**
 * @class ComputeFeatureReferenceCAxisMisorientations
 * @brief Computes the misorientation angle between each voxel's c-axis and
 *        the average c-axis of its Feature, plus per-Feature mean and standard
 *        deviation of those angles.
 *
 * Only Hexagonal-High (6/mmm) and Hexagonal-Low (6/m) Laue classes are
 * supported; non-hexagonal phases are skipped with zero output.
 *
 * ## OOC Optimization
 *
 * The algorithm processes one Z-slice at a time. For each slice:
 *   1. Cell-level arrays (featureIds, phases, quats) are bulk-read into
 *      local buffers via `copyIntoBuffer()`.
 *   2. Feature-level avgCAxes and ensemble-level crystal structures are
 *      cached in local vectors at algorithm start (small arrays).
 *   3. The per-cell output is accumulated in a local buffer and
 *      bulk-written via `copyFromBuffer()`.
 *   4. A second Z-slice pass re-reads the output to compute the
 *      per-Feature standard deviation.
 *
 * This Z-slice strategy gives predictable memory usage (one slice at a time)
 * and sequential I/O patterns that perform well with OOC chunked storage.
 */
class ORIENTATIONANALYSIS_EXPORT ComputeFeatureReferenceCAxisMisorientations
{
public:
  ComputeFeatureReferenceCAxisMisorientations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                              ComputeFeatureReferenceCAxisMisorientationsInputValues* inputValues);
  ~ComputeFeatureReferenceCAxisMisorientations() noexcept;

  ComputeFeatureReferenceCAxisMisorientations(const ComputeFeatureReferenceCAxisMisorientations&) = delete;
  ComputeFeatureReferenceCAxisMisorientations(ComputeFeatureReferenceCAxisMisorientations&&) noexcept = delete;
  ComputeFeatureReferenceCAxisMisorientations& operator=(const ComputeFeatureReferenceCAxisMisorientations&) = delete;
  ComputeFeatureReferenceCAxisMisorientations& operator=(ComputeFeatureReferenceCAxisMisorientations&&) noexcept = delete;

  /**
   * @brief Executes the c-axis misorientation computation using Z-slice bulk I/O.
   * @return Result<> with any errors or warnings encountered.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeFeatureReferenceCAxisMisorientationsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
