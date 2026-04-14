#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"

namespace nx::core
{

/**
 * @brief Input values for the ComputeFeatureReferenceMisorientations algorithm.
 */
struct ORIENTATIONANALYSIS_EXPORT ComputeFeatureReferenceMisorientationsInputValues
{
  ChoicesParameter::ValueType ReferenceOrientation;  ///< 0 = average orientation, 1 = orientation farthest from boundary
  DataPath FeatureAttributeMatrixPath;               ///< Feature-level AttributeMatrix (used for tuple count in mode 1)
  DataPath FeatureIdsArrayPath;                      ///< Cell-level Int32 feature ID per voxel
  DataPath CellPhasesArrayPath;                      ///< Cell-level Int32 phase index per voxel
  DataPath QuatsArrayPath;                           ///< Cell-level Float32 quaternions (4 components)
  DataPath GBEuclideanDistancesArrayPath;            ///< Cell-level Float32 grain boundary Euclidean distances (mode 1 only)
  DataPath AvgQuatsArrayPath;                        ///< Feature-level Float32 average quaternions (mode 0 only)
  DataPath CrystalStructuresArrayPath;               ///< Ensemble-level UInt32 crystal structure Laue classes
  DataPath FeatureReferenceMisorientationsArrayName; ///< Output: Cell-level Float32 misorientation angle (degrees)
  DataPath FeatureAvgMisorientationsArrayName;       ///< Output: Feature-level Float32 average misorientation (degrees)
  DataPath FeatureEuclideanCentersPath;              ///< Output: Feature-level Float32 Euclidean center coordinates (mode 1)
};

/**
 * @class ComputeFeatureReferenceMisorientations
 * @brief Computes the misorientation angle between each voxel and its Feature's
 *        reference orientation, plus the per-Feature average of those angles.
 *
 * Two reference modes are supported:
 *   - **Mode 0**: Reference is the Feature's average quaternion (from AvgQuats).
 *   - **Mode 1**: Reference is the voxel farthest from the grain boundary
 *     (identified by maximum grain-boundary Euclidean distance).
 *
 * ## OOC Optimization
 *
 * All cell-level arrays (featureIds, phases, quats, GB distances) are read
 * in chunks of 65536 tuples via `copyIntoBuffer()`. Feature-level arrays
 * (avgQuats, crystal structures) are cached entirely in local vectors at
 * algorithm start. The cell-level misorientation output is written back in
 * matching chunks via `copyFromBuffer()`. In mode 1, the center-voxel
 * identification pass also uses chunked I/O. This strategy converts per-element
 * virtual dispatch into bulk sequential I/O, eliminating OOC performance cliffs.
 */
class ORIENTATIONANALYSIS_EXPORT ComputeFeatureReferenceMisorientations
{
public:
  ComputeFeatureReferenceMisorientations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                         ComputeFeatureReferenceMisorientationsInputValues* inputValues);
  ~ComputeFeatureReferenceMisorientations() noexcept;

  ComputeFeatureReferenceMisorientations(const ComputeFeatureReferenceMisorientations&) = delete;
  ComputeFeatureReferenceMisorientations(ComputeFeatureReferenceMisorientations&&) noexcept = delete;
  ComputeFeatureReferenceMisorientations& operator=(const ComputeFeatureReferenceMisorientations&) = delete;
  ComputeFeatureReferenceMisorientations& operator=(ComputeFeatureReferenceMisorientations&&) noexcept = delete;

  /**
   * @brief Executes the misorientation computation using chunked bulk I/O.
   * @return Result<> with any errors encountered during execution.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeFeatureReferenceMisorientationsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
