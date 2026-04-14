#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <vector>

namespace nx::core
{

/**
 * @brief Input values for the ComputeAvgOrientations algorithm.
 *
 * All DataPath members are validated by the filter's preflight. Cell-level
 * arrays (featureIds, phases, quats) may contain millions of tuples and are
 * accessed through chunked bulk I/O in the optimized Rodrigues path.
 * Feature- and ensemble-level arrays are small enough to cache entirely in
 * local memory.
 */
struct ORIENTATIONANALYSIS_EXPORT ComputeAvgOrientationsInputValues
{
  DataPath cellFeatureIdsArrayPath;    ///< Cell-level Int32 array mapping each voxel to its feature
  DataPath cellPhasesArrayPath;        ///< Cell-level Int32 array of phase indices
  DataPath cellQuatsArrayPath;         ///< Cell-level Float32 array of quaternions (4 components)
  DataPath crystalStructuresArrayPath; ///< Ensemble-level UInt32 array of crystal structure Laue classes

  bool useRodriguesAverage;         ///< Enable the Rodrigues (running-sum) averaging method
  bool useVonMisesAverage;          ///< Enable the von Mises-Fisher EM averaging method
  bool useWatsonAverage;            ///< Enable the Watson EM averaging method
  DataPath avgQuatsArrayPath;       ///< Output: Rodrigues average quaternions (feature-level)
  DataPath avgEulerAnglesArrayPath; ///< Output: Rodrigues average Euler angles (feature-level)

  DataPath VMFQuatsArrayPath;       ///< Output: vMF average quaternions (feature-level)
  DataPath VMFEulerAnglesArrayPath; ///< Output: vMF average Euler angles (feature-level)
  DataPath VMFKappaArrayPath;       ///< Output: vMF kappa concentration values (feature-level)

  DataPath WatsonQuatsArrayPath;       ///< Output: Watson average quaternions (feature-level)
  DataPath WatsonEulerAnglesArrayPath; ///< Output: Watson average Euler angles (feature-level)
  DataPath WatsonKappaArrayPath;       ///< Output: Watson kappa concentration values (feature-level)

  uint32 RandomSeed = 43514; ///< Fixed seed for EM reproducibility
  int32 NumEMIterations = 5; ///< Number of outer EM iterations
  int32 NumIterations = 10;  ///< Number of inner iterations per EM cycle
};

/**
 * @class ComputeAvgOrientations
 * @brief Computes the average orientation of each Feature from its constituent
 *        voxel quaternions, using one or more averaging methods.
 *
 * Three independent methods are available (each can be toggled on/off):
 *   1. **Rodrigues average** -- running quaternion sum with nearest-quat selection
 *   2. **Von Mises-Fisher (vMF) average** -- EM-based estimation on the unit quaternion sphere
 *   3. **Watson average** -- EM-based estimation with antipodal symmetry
 *
 * ## OOC Optimization (Rodrigues path)
 *
 * The Rodrigues path iterates over every voxel to accumulate quaternion sums
 * per feature. In the original implementation each voxel accessed cell-level
 * DataArrays via `operator[]`, which triggers a virtual dispatch per element.
 * When the DataStore is backed by an out-of-core (OOC) chunked store this
 * causes a chunk load/evict cycle on every access, making the algorithm
 * orders of magnitude slower.
 *
 * The optimized implementation:
 *   - Caches ensemble-level crystal structures into a local `std::vector`
 *     (tiny -- one entry per phase).
 *   - Accumulates running quaternion averages in a local `std::vector`
 *     (feature-level -- one quaternion per feature, manageable).
 *   - Reads cell-level arrays (featureIds, phases, quats) in fixed-size
 *     chunks of 65536 tuples via `copyIntoBuffer()`, processing each chunk
 *     from contiguous local memory.
 *   - Writes feature-level results (avgQuats, avgEuler) back to the
 *     DataStore in a single `copyFromBuffer()` call.
 *
 * This converts O(N) random virtual dispatches into O(N/chunk) bulk I/O
 * operations, eliminating the OOC performance cliff.
 */
class ORIENTATIONANALYSIS_EXPORT ComputeAvgOrientations
{
public:
  ComputeAvgOrientations(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, ComputeAvgOrientationsInputValues* inputValues);
  ~ComputeAvgOrientations() noexcept;

  ComputeAvgOrientations(const ComputeAvgOrientations&) = delete;            // Copy Constructor Not Implemented
  ComputeAvgOrientations(ComputeAvgOrientations&&) = delete;                 // Move Constructor Not Implemented
  ComputeAvgOrientations& operator=(const ComputeAvgOrientations&) = delete; // Copy Assignment Not Implemented
  ComputeAvgOrientations& operator=(ComputeAvgOrientations&&) = delete;      // Move Assignment Not Implemented

  /**
   * @brief Executes the enabled averaging methods and populates the output arrays.
   * @return Result<> with any errors or warnings encountered during execution.
   */
  Result<> operator()();

  /**
   * @brief Thread-safe progress message emitter used by the vMF/Watson parallel path.
   * @param counter Number of features processed since last call.
   */
  void sendThreadSafeProgressMessage(usize counter);

protected:
private:
  DataStructure& m_DataStructure;
  const IFilter::MessageHandler& m_MessageHandler;
  const std::atomic_bool& m_ShouldCancel;
  const ComputeAvgOrientationsInputValues* m_InputValues = nullptr;

  /**
   * @brief Computes the Rodrigues (running-sum) average orientation per feature.
   *
   * Uses chunked bulk I/O for cell-level arrays and local buffers for
   * feature-level accumulation to avoid per-element OOC overhead.
   */
  Result<> computeRodriguesAverage();

  /**
   * @brief Computes von Mises-Fisher and/or Watson average orientations per feature
   *        using an Expectation-Maximization algorithm.
   */
  Result<> computeVmfWatsonAverage();

  // Thread safe Progress Message
  std::chrono::steady_clock::time_point m_InitialPoint = std::chrono::steady_clock::now();
  mutable std::mutex m_ProgressMessage_Mutex;
  size_t m_NumberOfFeatures = 0;
  size_t m_ProgressCounter = 0;
  size_t m_LastProgressInt = 0;
};

} // namespace nx::core
