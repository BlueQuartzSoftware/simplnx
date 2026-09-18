#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

namespace nx::core
{

struct ComputeKernelAvgMisorientationsInputValues;

/**
 * @struct ComputeKernelAvgMisorientationsWorkingSet
 * @brief Defines the bounded buffers for a scanline KAM traversal.
 *
 * The planner uses a rolling Z window when its input slices and one output
 * slice fit the policy cap. Otherwise, it uses a fixed input-block cache and
 * one output block. The one-tuple fallback allocation can exceed the policy
 * cap, CapBytes, when one tuple does not fit.
 *
 * CapBytes is the smaller of one quarter of the budget and one half of the
 * available budget. SliceTuples is the X-by-Y tuple count. WindowSlices is the
 * clamped 2*kZ+1 depth. RollingBytes includes input slices and one output
 * slice. UseRollingWindow identifies the selected plan. BlockTuples and
 * CacheSlots remain at least one for the fallback.
 */
struct ComputeKernelAvgMisorientationsWorkingSet
{
  uint64 CapBytes = 0;
  usize SliceTuples = 0;
  usize WindowSlices = 0;
  uint64 RollingBytes = 0;
  bool UseRollingWindow = false;
  usize BlockTuples = 1;
  usize CacheSlots = 1;
};

/**
 * @brief Creates a bounded KAM working-set plan.
 * @param dimensions Identifies the Image Geometry dimensions in X, Y, Z order.
 * @param kernelSize Specifies the kernel radii in X, Y, Z order.
 * @param cacheBudgetBytes Specifies the cache budget in bytes.
 * @param cacheUsedBytes Specifies the currently accounted cache use in bytes.
 * @param imageGeometryPath Identifies the Image Geometry in error messages.
 * @return A rolling-window or fixed-block plan, or an error for invalid radii
 *         or an arithmetic overflow.
 *
 * The policy cap is the smaller of one quarter of the budget and one half of
 * the available budget. A fixed fallback bounds every valid kernel when a
 * full rolling window does not fit. The one-tuple fallback can exceed the
 * policy cap when one tuple does not fit.
 */
ORIENTATIONANALYSIS_EXPORT Result<ComputeKernelAvgMisorientationsWorkingSet> CreateComputeKernelAvgMisorientationsWorkingSet(const SizeVec3& dimensions,
                                                                                                                             const VectorInt32Parameter::ValueType& kernelSize, uint64 cacheBudgetBytes,
                                                                                                                             uint64 cacheUsedBytes, const DataPath& imageGeometryPath);

/**
 * @class ComputeKernelAvgMisorientationsScanline
 * @brief Computes the Kernel Average Misorientation (KAM) for each ImageGeom
 *        cell.
 *
 * The executor averages crystallographic misorientation angles in degrees for
 * each valid focal cell. Same-feature mode admits only cells in the focal
 * feature. Same-phase mode admits positive feature IDs in the focal phase.
 *
 * CacheMemoryBudgetManager supplies the budget and current use for target
 * payload planning. A rolling traversal copies Feature ID, Cell Phase, and
 * quaternion slices before parallel workers read local buffers and write
 * disjoint local output elements. A fixed block-cache traversal performs all
 * DataStore access sequentially when the rolling window does not fit. Its
 * one-tuple fallback can exceed the policy cap. These designs do not make
 * generic DataArray or DataStore access concurrent-safe.
 */
class ORIENTATIONANALYSIS_EXPORT ComputeKernelAvgMisorientationsScanline
{
public:
  /**
   * @brief Initializes the scanline KAM executor.
   * @param dataStructure Provides the selected arrays and Image Geometry.
   * @param msgHandler Receives progress messages.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Identifies the selected arrays and KAM settings.
   * @pre dataStructure, msgHandler, shouldCancel, and inputValues remain valid
   *      while this executor runs.
   */
  ComputeKernelAvgMisorientationsScanline(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel,
                                          const ComputeKernelAvgMisorientationsInputValues* inputValues);

  /**
   * @brief Destroys the scanline KAM executor.
   */
  ~ComputeKernelAvgMisorientationsScanline() noexcept;

  ComputeKernelAvgMisorientationsScanline(const ComputeKernelAvgMisorientationsScanline&) = delete;
  ComputeKernelAvgMisorientationsScanline(ComputeKernelAvgMisorientationsScanline&&) noexcept = delete;
  ComputeKernelAvgMisorientationsScanline& operator=(const ComputeKernelAvgMisorientationsScanline&) = delete;
  ComputeKernelAvgMisorientationsScanline& operator=(ComputeKernelAvgMisorientationsScanline&&) noexcept = delete;

  /**
   * @brief Executes the cache-budgeted scanline KAM computation.
   * @return Success, an input or working-set overflow error, or a DataStore
   *         bulk-I/O error.
   *
   * Cancellation is checked before each output plane and fallback block. The
   * method returns success and preserves already written output.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeKernelAvgMisorientationsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
