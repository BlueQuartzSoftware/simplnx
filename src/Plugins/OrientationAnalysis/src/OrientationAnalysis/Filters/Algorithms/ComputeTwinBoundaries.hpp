#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <vector>

namespace nx::core
{
/**
 * @struct ComputeTwinBoundariesInputValues
 * @brief Identifies twin-boundary inputs.
 *
 * AngleTolerance and AxisTolerance are in degrees.
 */
struct ORIENTATIONANALYSIS_EXPORT ComputeTwinBoundariesInputValues
{
  bool FindCoherence;
  float32 AngleTolerance;
  float32 AxisTolerance;
  DataPath FaceLabelsArrayPath;
  DataPath FaceNormalsArrayPath;
  DataPath AvgQuatsArrayPath;
  DataPath FeaturePhasesArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath TwinBoundariesArrayPath;
  DataPath TwinBoundaryIncoherenceArrayPath;
};

/**
 * @class ComputeTwinBoundaries
 * @brief Identifies sigma-3 twin boundaries on a triangle mesh.
 *
 * Cubic features compare their average orientations to the 60-degree [111]
 * relationship. Local feature, face, and ensemble buffers keep parallel
 * workers outside DataStore access. Coherence optionally uses face normals.
 */
class ORIENTATIONANALYSIS_EXPORT ComputeTwinBoundaries
{
public:
  /**
   * @brief Initializes twin-boundary computation.
   * @param dataStructure Provides selected arrays.
   * @param mesgHandler Supplies the filter message handler.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Identifies selected arrays and tolerances.
   * @pre dataStructure, mesgHandler, shouldCancel, and inputValues outlive this
   *      executor.
   */
  ComputeTwinBoundaries(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeTwinBoundariesInputValues* inputValues);
  /**
   * @brief Destroys the twin-boundary executor.
   */
  ~ComputeTwinBoundaries() noexcept;

  ComputeTwinBoundaries(const ComputeTwinBoundaries&) = delete;
  ComputeTwinBoundaries(ComputeTwinBoundaries&&) noexcept = delete;
  ComputeTwinBoundaries& operator=(const ComputeTwinBoundaries&) = delete;
  ComputeTwinBoundaries& operator=(ComputeTwinBoundaries&&) noexcept = delete;

  /**
   * @brief Computes twin boundaries.
   * @return An error if no cubic phase exists, or warnings for skipped phases or
   *         invalid normals.
   *
   * Cancellation returns success with completed face results preserved. Current
   * input and incoherence bulk-I/O Result values are not inspected.
   */
  Result<> operator()();

  /**
   * @brief Returns the retained cancellation flag.
   * @return Reference to the cancellation flag supplied at construction.
   */
  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeTwinBoundariesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
