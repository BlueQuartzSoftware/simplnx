#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

#include <chrono>

namespace nx::core
{

/**
 * @brief Input values for the ComputeKernelAvgMisorientations algorithm.
 */
struct ORIENTATIONANALYSIS_EXPORT ComputeKernelAvgMisorientationsInputValues
{
  VectorInt32Parameter::ValueType KernelSize;     ///< Half-widths {kX, kY, kZ} of the kernel in each dimension
  DataPath FeatureIdsArrayPath;                   ///< Cell-level Int32 feature ID per voxel
  DataPath CellPhasesArrayPath;                   ///< Cell-level Int32 phase index per voxel
  DataPath QuatsArrayPath;                        ///< Cell-level Float32 quaternions (4 components)
  DataPath CrystalStructuresArrayPath;            ///< Ensemble-level UInt32 crystal structure Laue classes
  DataPath KernelAverageMisorientationsArrayName; ///< Output: Cell-level Float32 KAM value (degrees)
  DataPath InputImageGeometry;                    ///< ImageGeom providing voxel grid dimensions
};

/**
 * @class ComputeKernelAvgMisorientations
 * @brief Computes the Kernel Average Misorientation (KAM) for each voxel in
 *        an ImageGeom.
 *
 * For each voxel, the misorientation angle between the voxel and every
 * neighbor within the user-specified kernel is calculated (using
 * crystallographic symmetry operators). The average of these angles is
 * stored as the KAM value. Only neighbors belonging to the same Feature
 * (same featureId) are included.
 *
 * ## OOC Optimization (Major Rewrite)
 *
 * The original implementation used `ParallelData3DAlgorithm` with per-element
 * `operator[]` access, which causes catastrophic performance with OOC storage
 * because the kernel neighborhood access pattern triggers random chunk
 * load/evict cycles.
 *
 * The optimized implementation uses a **slab-based** strategy:
 *   - For each Z-plane, a slab spanning `[plane - kZ, plane + kZ]` is
 *     bulk-read via `copyIntoBuffer()`. This slab contains all data needed
 *     for every neighbor lookup of voxels in the current plane.
 *   - All kernel neighbor accesses index into local contiguous buffers
 *     (zero virtual dispatch overhead).
 *   - The output for each plane is bulk-written via `copyFromBuffer()`.
 *   - Ensemble-level crystal structures are cached in a local vector.
 *
 * The slab approach has predictable memory usage proportional to
 * `(2*kZ + 1) * X * Y` and provides sequential I/O that OOC stores handle
 * efficiently. Parallelization is deliberately disabled because the slab
 * I/O pattern already provides good throughput and avoids thread-safety
 * issues with DataStore access.
 */
class ORIENTATIONANALYSIS_EXPORT ComputeKernelAvgMisorientations
{
public:
  ComputeKernelAvgMisorientations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                  ComputeKernelAvgMisorientationsInputValues* inputValues);
  ~ComputeKernelAvgMisorientations() noexcept;

  ComputeKernelAvgMisorientations(const ComputeKernelAvgMisorientations&) = delete;
  ComputeKernelAvgMisorientations(ComputeKernelAvgMisorientations&&) noexcept = delete;
  ComputeKernelAvgMisorientations& operator=(const ComputeKernelAvgMisorientations&) = delete;
  ComputeKernelAvgMisorientations& operator=(ComputeKernelAvgMisorientations&&) noexcept = delete;

  /**
   * @brief Executes the KAM computation using slab-based bulk I/O.
   * @return Result<> with any errors encountered during execution.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeKernelAvgMisorientationsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
