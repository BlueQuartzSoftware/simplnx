#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <vector>

namespace nx::core
{
/**
 * @brief Input values for the ComputeTwinBoundaries algorithm.
 */
struct ORIENTATIONANALYSIS_EXPORT ComputeTwinBoundariesInputValues
{
  bool FindCoherence;                        ///< If true, also compute incoherence angle using face normals
  float32 AngleTolerance;                    ///< Tolerance (degrees) for the sigma-3 misorientation angle
  float32 AxisTolerance;                     ///< Tolerance (degrees) for the sigma-3 misorientation axis
  DataPath FaceLabelsArrayPath;              ///< Face-level Int32 labels (2 components: feature1, feature2)
  DataPath FaceNormalsArrayPath;             ///< Face-level Float64 normals (3 components, coherence mode only)
  DataPath AvgQuatsArrayPath;                ///< Feature-level Float32 average quaternions (4 components)
  DataPath FeaturePhasesArrayPath;           ///< Feature-level Int32 phase index per feature
  DataPath CrystalStructuresArrayPath;       ///< Ensemble-level UInt32 crystal structure Laue classes
  DataPath TwinBoundariesArrayPath;          ///< Output: Face-level mask (Bool or UInt8) flagging twin boundaries
  DataPath TwinBoundaryIncoherenceArrayPath; ///< Output: Face-level Float32 incoherence angle (degrees, coherence mode)
};

/**
 * @class ComputeTwinBoundaries
 * @brief Identifies sigma-3 twin boundaries on a triangle surface mesh and
 *        optionally computes the incoherence angle for each twin boundary.
 *
 * For each face on the surface mesh, the average orientations of the two
 * adjacent Features are compared to the sigma-3 twin misorientation
 * (60 degrees about <111>). If the misorientation falls within the user-specified
 * axis and angle tolerances, the face is flagged as a twin boundary. When
 * coherence computation is enabled, the crystal direction parallel to the face
 * normal is compared with the misorientation axis to determine the incoherence.
 *
 * Only Cubic-High (m3m) and Cubic-Low (m3) Laue classes are considered.
 *
 * ## OOC Optimization
 *
 * All input arrays are cached entirely in local `std::vector`s before the
 * parallel computation begins:
 *   - Feature-level arrays (phases, avgQuats) -- O(features), small.
 *   - Face-level arrays (faceLabels, faceNormals) -- O(faces), scales with
 *     surface area not volume, typically manageable.
 *   - Ensemble-level crystal structures -- tiny.
 *
 * The parallel workers (`CalculateTwinBoundaryImpl` and
 * `CalculateTwinBoundaryWithIncoherenceImpl`) operate exclusively on these
 * local vectors, achieving zero virtual dispatch overhead in the hot loop.
 * Results are accumulated in local output vectors and written back to
 * DataStores via bulk I/O after parallel execution completes.
 */
class ORIENTATIONANALYSIS_EXPORT ComputeTwinBoundaries
{
public:
  ComputeTwinBoundaries(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeTwinBoundariesInputValues* inputValues);
  ~ComputeTwinBoundaries() noexcept;

  ComputeTwinBoundaries(const ComputeTwinBoundaries&) = delete;
  ComputeTwinBoundaries(ComputeTwinBoundaries&&) noexcept = delete;
  ComputeTwinBoundaries& operator=(const ComputeTwinBoundaries&) = delete;
  ComputeTwinBoundaries& operator=(ComputeTwinBoundaries&&) noexcept = delete;

  /**
   * @brief Executes twin boundary identification with locally cached data.
   * @return Result<> with any errors or warnings (e.g., NaN normals, non-cubic phases).
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeTwinBoundariesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
