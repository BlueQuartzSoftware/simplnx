#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

namespace nx::core
{

struct ComputeGBCDPoleFigureInputValues;

/**
 * @class ComputeGBCDPoleFigureScanline
 * @brief Out-of-core (Scanline) algorithm for generating a GBCD stereographic pole figure.
 *
 * This algorithm is selected by the dispatcher when the GBCD array is backed by
 * chunked (OOC) storage on disk. The full GBCD array can be very large (millions of
 * float64 elements across all phases), but for a single pole figure only the bins
 * belonging to one phase are accessed.
 *
 * **OOC optimization**: Instead of caching the entire GBCD array (as the Direct
 * variant does), this algorithm caches only the single-phase slice of the GBCD via
 * a single copyIntoBuffer() call. For a typical GBCD with 5D bin resolution, one
 * phase slice is on the order of hundreds of thousands of float64 elements -- far
 * smaller than the full multi-phase array. This dramatically reduces the memory
 * footprint and avoids random-access chunk thrashing across phase boundaries.
 *
 * Once the phase slice is cached in a local buffer, the actual pole-figure
 * computation is identical to the Direct variant and runs multi-threaded using
 * ParallelData2DAlgorithm on the cached raw pointers.
 *
 * **Memory footprint**: O(totalGbcdBins) for one phase, plus O(outputDim^2) for
 * the pole figure cache. Both are bounded by the GBCD bin resolution, not by the
 * total number of phases.
 *
 * @see ComputeGBCDPoleFigureDirect for the in-core variant that caches the full GBCD.
 */
class ORIENTATIONANALYSIS_EXPORT ComputeGBCDPoleFigureScanline
{
public:
  /**
   * @brief Constructs the OOC GBCD pole figure algorithm.
   * @param dataStructure The DataStructure containing all input/output arrays.
   * @param mesgHandler Message handler for progress/info messages.
   * @param shouldCancel Atomic cancellation flag.
   * @param inputValues Pointer to the shared parameter struct; must outlive this object.
   */
  ComputeGBCDPoleFigureScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeGBCDPoleFigureInputValues* inputValues);
  ~ComputeGBCDPoleFigureScanline() noexcept;

  ComputeGBCDPoleFigureScanline(const ComputeGBCDPoleFigureScanline&) = delete;
  ComputeGBCDPoleFigureScanline(ComputeGBCDPoleFigureScanline&&) noexcept = delete;
  ComputeGBCDPoleFigureScanline& operator=(const ComputeGBCDPoleFigureScanline&) = delete;
  ComputeGBCDPoleFigureScanline& operator=(ComputeGBCDPoleFigureScanline&&) noexcept = delete;

  /**
   * @brief Generates the pole figure by caching only the phase-of-interest GBCD slice.
   * @return Result<> (currently always succeeds).
   */
  Result<> operator()();

  /**
   * @brief Returns the cancellation flag reference.
   * @return const reference to the atomic cancellation flag.
   */
  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;                                  ///< Reference to the live DataStructure.
  const ComputeGBCDPoleFigureInputValues* m_InputValues = nullptr; ///< Borrowed pointer to input parameters.
  const std::atomic_bool& m_ShouldCancel;                          ///< Cancellation flag.
  const IFilter::MessageHandler& m_MessageHandler;                 ///< Message handler for user-facing messages.
};

} // namespace nx::core
