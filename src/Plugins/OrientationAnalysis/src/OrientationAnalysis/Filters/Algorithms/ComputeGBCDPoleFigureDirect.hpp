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
 * @brief Holds GBCD pole-figure input values.
 *
 * A GBCD is a five-dimensional grain-boundary distribution. These values select one phase,
 * misorientation, and square output image.
 *
 * PhaseOfInterest is one-based. MisorientationRotation stores an angle in degrees followed by an
 * axis. The executors normalize the axis. OutputImageDimension specifies the square side length
 * in pixels.
 */
struct ORIENTATIONANALYSIS_EXPORT ComputeGBCDPoleFigureInputValues
{
  int32 PhaseOfInterest;
  VectorFloat32Parameter::ValueType MisorientationRotation;
  DataPath GBCDArrayPath;
  DataPath CrystalStructuresArrayPath;
  int32 OutputImageDimension;
  DataPath ImageGeometryPath;
  std::string CellAttributeMatrixName;
  std::string CellIntensityArrayName;
};

/**
 * @class ComputeGBCDPoleFigureDirect
 * @brief Generates a pole figure from a full in-memory GBCD cache.
 *
 * The dispatcher normally selects this class for the in-memory scenario. It
 * copies the full GBCD and crystal-structure arrays into local caches. It
 * allocates a zero-filled full output buffer before parallel pixel calculation,
 * then writes that buffer after the calculation. Workers access only local
 * caches and disjoint output pixels.
 * This design gives no generic DataArray or DataStore concurrency guarantee.
 * The full cache is unsuitable when OOC data has many phases.
 *
 * The executor retains the cancellation flag for getCancel(). operator() does
 * not inspect the flag, so the current computation runs to completion.
 *
 * @see ComputeGBCDPoleFigureScanline for the OOC-optimized variant.
 */
class ORIENTATIONANALYSIS_EXPORT ComputeGBCDPoleFigureDirect
{
public:
  /**
   * @brief Initializes the direct GBCD pole-figure executor.
   * @param dataStructure Provides the selected arrays.
   * @param mesgHandler Supplies the filter message handler.
   * @param shouldCancel Provides the retained cancellation flag.
   * @param inputValues Identifies pole-figure settings.
   * @pre dataStructure, mesgHandler, shouldCancel, and inputValues outlive this
   *      executor.
   */
  ComputeGBCDPoleFigureDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeGBCDPoleFigureInputValues* inputValues);

  /**
   * @brief Destroys the direct GBCD pole-figure executor.
   */
  ~ComputeGBCDPoleFigureDirect() noexcept;

  ComputeGBCDPoleFigureDirect(const ComputeGBCDPoleFigureDirect&) = delete;
  ComputeGBCDPoleFigureDirect(ComputeGBCDPoleFigureDirect&&) noexcept = delete;
  ComputeGBCDPoleFigureDirect& operator=(const ComputeGBCDPoleFigureDirect&) = delete;
  ComputeGBCDPoleFigureDirect& operator=(ComputeGBCDPoleFigureDirect&&) noexcept = delete;

  /**
   * @brief Generates the pole figure.
   * @return Success. The direct path does not inspect bulk-I/O results.
   *
   * The executor does not inspect cancellation and runs the current computation
   * to completion.
   */
  Result<> operator()();

  /**
   * @brief Returns the retained cancellation flag.
   * @return Reference to the cancellation flag supplied at construction.
   *
   * operator() does not inspect this flag.
   */
  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeGBCDPoleFigureInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
