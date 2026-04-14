#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <vector>

namespace nx::core
{

/**
 * @struct ComputeIPFColorsInputValues
 * @brief Holds the user-facing parameters for the Compute IPF Colors algorithm.
 *
 * Inverse Pole Figure (IPF) colors map a crystal orientation to a position on the
 * unit stereographic triangle for the crystal's Laue class, producing a unique RGB
 * color. The reference direction determines which sample axis is projected into the
 * crystal frame before the color lookup.
 */
struct ORIENTATIONANALYSIS_EXPORT ComputeIPFColorsInputValues
{
  std::vector<float> referenceDirection; ///< Sample-frame reference direction (typically [0,0,1]) that is projected into the crystal frame for IPF color computation.
  bool useGoodVoxels = false;            ///< When true, voxels whose mask value is false/0 are colored black (skipped).
  DataPath goodVoxelsArrayPath;          ///< Path to the boolean or uint8 mask array. Only used when useGoodVoxels is true.
  DataPath cellPhasesArrayPath;          ///< Path to the Int32 array of per-voxel phase IDs (1-based; 0 = unindexed).
  DataPath cellEulerAnglesArrayPath;     ///< Path to the Float32 array of Euler angles (phi1, Phi, phi2) in radians, 3 components per tuple.
  DataPath crystalStructuresArrayPath;   ///< Path to the UInt32 ensemble array mapping phase ID -> EbsdLib crystal structure enum.
  DataPath cellIpfColorsArrayPath;       ///< Path to the output UInt8 array of RGB colors, 3 components per tuple.
};

/**
 * @class ComputeIPFColors
 * @brief Dispatcher that selects between in-core and out-of-core IPF color algorithms.
 *
 * This class serves as the entry point called by ComputeIPFColorsFilter::executeImpl().
 * It inspects the backing storage of the Euler-angle, phase, and IPF-color arrays using
 * DispatchAlgorithm<ComputeIPFColorsDirect, ComputeIPFColorsScanline>:
 *
 * - **In-core (ComputeIPFColorsDirect)**: Uses ParallelDataAlgorithm for multi-threaded
 *   random access when all arrays reside in contiguous RAM.
 * - **Out-of-core (ComputeIPFColorsScanline)**: Reads and writes data in fixed-size
 *   chunks via copyIntoBuffer()/copyFromBuffer() to avoid OOC chunk thrashing.
 *
 * @see ComputeIPFColorsDirect, ComputeIPFColorsScanline, DispatchAlgorithm
 */
class ORIENTATIONANALYSIS_EXPORT ComputeIPFColors
{
public:
  /**
   * @brief Constructs the dispatcher.
   * @param dataStructure The DataStructure containing all input and output arrays.
   * @param msgHandler Message handler for progress/info messages.
   * @param shouldCancel Atomic flag checked periodically to support user cancellation.
   * @param inputValues Pointer to the parameter struct; must outlive this object.
   */
  ComputeIPFColors(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, ComputeIPFColorsInputValues* inputValues);
  ~ComputeIPFColors() noexcept;

  ComputeIPFColors(const ComputeIPFColors&) = delete;
  ComputeIPFColors(ComputeIPFColors&&) = delete;
  ComputeIPFColors& operator=(const ComputeIPFColors&) = delete;
  ComputeIPFColors& operator=(ComputeIPFColors&&) = delete;

  /**
   * @brief Dispatches to ComputeIPFColorsDirect or ComputeIPFColorsScanline based
   *        on whether any of the involved arrays use out-of-core storage.
   * @return Result<> with any errors (e.g., phase mismatch warnings).
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;                             ///< Reference to the live DataStructure.
  const IFilter::MessageHandler& m_MessageHandler;            ///< Message handler for user-facing messages.
  const std::atomic_bool& m_ShouldCancel;                     ///< Cancellation flag.
  const ComputeIPFColorsInputValues* m_InputValues = nullptr; ///< Borrowed pointer to input parameters.
};

} // namespace nx::core
