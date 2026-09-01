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
 * @brief Generates a pole figure from one cached OOC phase slice.
 *
 * The dispatcher normally selects this class when the GBCD is OOC. It reads
 * the selected contiguous phase slice instead of all phase data. Full-width
 * output pages bound output staging memory. Workers access read-only local
 * caches and disjoint page elements. They do not access DataArray or DataStore
 * objects. Cancellation returns success with completed output pages preserved.
 *
 * @see ComputeGBCDPoleFigureDirect for the in-core variant that caches the full GBCD.
 */
class ORIENTATIONANALYSIS_EXPORT ComputeGBCDPoleFigureScanline
{
public:
  /**
   * @brief Initializes the scanline GBCD pole-figure executor.
   * @param dataStructure Provides the selected arrays.
   * @param mesgHandler Supplies the filter message handler.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Identifies pole-figure settings.
   * @pre dataStructure, mesgHandler, shouldCancel, and inputValues outlive this
   *      executor.
   */
  ComputeGBCDPoleFigureScanline(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeGBCDPoleFigureInputValues* inputValues);

  /**
   * @brief Destroys the scanline GBCD pole-figure executor.
   */
  ~ComputeGBCDPoleFigureScanline() noexcept;

  ComputeGBCDPoleFigureScanline(const ComputeGBCDPoleFigureScanline&) = delete;
  ComputeGBCDPoleFigureScanline(ComputeGBCDPoleFigureScanline&&) noexcept = delete;
  ComputeGBCDPoleFigureScanline& operator=(const ComputeGBCDPoleFigureScanline&) = delete;
  ComputeGBCDPoleFigureScanline& operator=(ComputeGBCDPoleFigureScanline&&) noexcept = delete;

  /**
   * @brief Generates the pole figure.
   * @return Success, or an error from crystal-structure, phase-slice, or output
   *         bulk I/O.
   *
   * Cancellation returns success and preserves completed output pages.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeGBCDPoleFigureInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
