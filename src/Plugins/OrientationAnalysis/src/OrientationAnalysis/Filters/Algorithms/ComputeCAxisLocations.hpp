#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

/**
 * @struct ComputeCAxisLocationsInputValues
 * @brief Identifies c-axis location inputs.
 */
struct ORIENTATIONANALYSIS_EXPORT ComputeCAxisLocationsInputValues
{
  DataPath QuatsArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath CAxisLocationsArrayName;
};

/**
 * @class ComputeCAxisLocations
 * @brief Converts each cell quaternion to a sample-frame c axis.
 *
 * The executor maps crystal [001] through the transposed orientation matrix.
 * It normalizes the result and makes its Z component positive.
 *
 * Hexagonal phases produce c axes. Other phases produce NaN values. The
 * executor reads and writes 65,536-tuple pages to bound OOC memory.
 */
class ORIENTATIONANALYSIS_EXPORT ComputeCAxisLocations
{
public:
  /**
   * @brief Initializes c-axis location computation.
   * @param dataStructure Provides selected arrays.
   * @param mesgHandler Supplies the filter message handler.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Identifies selected arrays.
   * @pre dataStructure, mesgHandler, shouldCancel, and inputValues outlive this
   *      executor.
   */
  ComputeCAxisLocations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeCAxisLocationsInputValues* inputValues);

  /**
   * @brief Destroys the c-axis location executor.
   */
  ~ComputeCAxisLocations() noexcept;

  ComputeCAxisLocations(const ComputeCAxisLocations&) = delete;
  ComputeCAxisLocations(ComputeCAxisLocations&&) noexcept = delete;
  ComputeCAxisLocations& operator=(const ComputeCAxisLocations&) = delete;
  ComputeCAxisLocations& operator=(ComputeCAxisLocations&&) noexcept = delete;

  /**
   * @brief Computes c-axis locations.
   * @return An error if no hexagonal phase exists, or a warning for skipped
   *         non-hexagonal phases.
   *
   * Cancellation returns success with completed pages preserved. Current bulk-
   * I/O Result values are not inspected.
   */
  Result<> operator()();

  /**
   * @brief Returns the retained cancellation flag.
   * @return Reference to the cancellation flag supplied at construction.
   */
  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeCAxisLocationsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
