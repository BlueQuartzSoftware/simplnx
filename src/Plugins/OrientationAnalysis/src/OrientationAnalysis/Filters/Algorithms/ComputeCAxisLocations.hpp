#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

/**
 * @brief Input values for the ComputeCAxisLocations algorithm.
 */
struct ORIENTATIONANALYSIS_EXPORT ComputeCAxisLocationsInputValues
{
  DataPath QuatsArrayPath;             ///< Cell-level Float32 quaternions (4 components)
  DataPath CellPhasesArrayPath;        ///< Cell-level Int32 phase index per voxel
  DataPath CrystalStructuresArrayPath; ///< Ensemble-level UInt32 crystal structure Laue classes
  DataPath CAxisLocationsArrayName;    ///< Output: Cell-level Float32 c-axis direction (3 components)
};

/**
 * @class ComputeCAxisLocations
 * @brief Converts each voxel's quaternion to a c-axis direction vector in the
 *        sample reference frame.
 *
 * For each Element, the quaternion is converted to an orientation matrix,
 * transposed (passive to active), and multiplied by the <001> c-axis direction.
 * The result is normalized and oriented so the Z component is positive.
 *
 * Only Hexagonal-High (6/mmm) and Hexagonal-Low (6/m) Laue classes are
 * supported; non-hexagonal phases produce NaN output values.
 *
 * ## OOC Optimization
 *
 * Cell-level arrays (quaternions, phases) are read in chunks of 65536 tuples
 * via `copyIntoBuffer()`, and the output (c-axis locations) is written back
 * in matching chunks via `copyFromBuffer()`. Ensemble-level crystal structures
 * are cached in a local vector. This replaces per-element `operator[]` access
 * that would trigger chunk load/evict cycles with OOC storage.
 */
class ORIENTATIONANALYSIS_EXPORT ComputeCAxisLocations
{
public:
  ComputeCAxisLocations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeCAxisLocationsInputValues* inputValues);
  ~ComputeCAxisLocations() noexcept;

  ComputeCAxisLocations(const ComputeCAxisLocations&) = delete;
  ComputeCAxisLocations(ComputeCAxisLocations&&) noexcept = delete;
  ComputeCAxisLocations& operator=(const ComputeCAxisLocations&) = delete;
  ComputeCAxisLocations& operator=(ComputeCAxisLocations&&) noexcept = delete;

  /**
   * @brief Executes the c-axis location computation using chunked bulk I/O.
   * @return Result<> with any errors or warnings encountered.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeCAxisLocationsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
