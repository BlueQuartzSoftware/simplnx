#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

namespace nx::core
{

/**
 * @struct ComputeGBCDPoleFigureInputValues
 * @brief Holds user-facing parameters for the GBCD Pole Figure computation.
 *
 * The Grain Boundary Character Distribution (GBCD) is a 5-dimensional histogram that
 * captures the statistical distribution of grain boundary planes as a function of the
 * misorientation between the two grains that meet at the boundary. This struct packages
 * the parameters needed to extract a 2D stereographic projection (pole figure) from that
 * 5D distribution for a specific misorientation and crystal phase.
 */
struct ORIENTATIONANALYSIS_EXPORT ComputeGBCDPoleFigureInputValues
{
  int32 PhaseOfInterest;                                    ///< 1-based phase index selecting which crystallographic phase's GBCD to visualize.
  VectorFloat32Parameter::ValueType MisorientationRotation; ///< Misorientation as [angle(deg), axis_x, axis_y, axis_z]. The axis is normalized internally.
  DataPath GBCDArrayPath;                                   ///< Path to the Float64 GBCD array (tuples = phases, 5D component shape from ComputeGBCD).
  DataPath CrystalStructuresArrayPath;                      ///< Path to the UInt32 ensemble array mapping phase ID -> EbsdLib crystal structure enum.
  int32 OutputImageDimension;                               ///< Side length (pixels) of the square output pole figure image.
  DataPath ImageGeometryPath;                               ///< Path to the output ImageGeometry that will hold the pole figure.
  std::string CellAttributeMatrixName;                      ///< Name of the cell AttributeMatrix created under the output ImageGeometry.
  std::string CellIntensityArrayName;                       ///< Name of the Float64 intensity array written into the cell AttributeMatrix.
};

/**
 * @class ComputeGBCDPoleFigureDirect
 * @brief In-core (Direct) algorithm for generating a GBCD stereographic pole figure.
 *
 * This algorithm is selected by the dispatcher when the GBCD array resides entirely
 * in contiguous in-memory storage. It performs the following steps:
 *
 *   1. Caches the entire GBCD array, crystal structures, and output pole figure into
 *      local heap buffers (the GBCD can be large -- millions of float64 elements --
 *      but fits in RAM when the data is in-core).
 *   2. For each pixel (x, y) in the output stereographic projection, computes the
 *      corresponding unit-sphere direction via inverse stereographic projection.
 *   3. Loops over all pairs of crystal symmetry operators (nSym x nSym) and for each
 *      pair computes the symmetrically-equivalent misorientation. If the equivalent
 *      misorientation falls in the fundamental zone (all three Euler angles < pi/2),
 *      the 5D GBCD bin is looked up and accumulated.
 *   4. The procedure is repeated in the second crystal reference frame (using the
 *      transpose of the misorientation matrix) to account for the bicrystal symmetry.
 *   5. The accumulated sum is averaged over the count of valid symmetry pairs.
 *   6. Uses ParallelData2DAlgorithm for multi-threaded computation across output pixels.
 *   7. Writes the final pole figure back to the DataStore via copyFromBuffer().
 *
 * @see ComputeGBCDPoleFigureScanline for the OOC-optimized variant.
 */
class ORIENTATIONANALYSIS_EXPORT ComputeGBCDPoleFigureDirect
{
public:
  /**
   * @brief Constructs the in-core GBCD pole figure algorithm.
   * @param dataStructure The DataStructure containing all input/output arrays.
   * @param mesgHandler Message handler for progress/info messages.
   * @param shouldCancel Atomic cancellation flag.
   * @param inputValues Pointer to the shared parameter struct; must outlive this object.
   */
  ComputeGBCDPoleFigureDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeGBCDPoleFigureInputValues* inputValues);
  ~ComputeGBCDPoleFigureDirect() noexcept;

  ComputeGBCDPoleFigureDirect(const ComputeGBCDPoleFigureDirect&) = delete;
  ComputeGBCDPoleFigureDirect(ComputeGBCDPoleFigureDirect&&) noexcept = delete;
  ComputeGBCDPoleFigureDirect& operator=(const ComputeGBCDPoleFigureDirect&) = delete;
  ComputeGBCDPoleFigureDirect& operator=(ComputeGBCDPoleFigureDirect&&) noexcept = delete;

  /**
   * @brief Generates the pole figure using multi-threaded parallel pixel computation.
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
