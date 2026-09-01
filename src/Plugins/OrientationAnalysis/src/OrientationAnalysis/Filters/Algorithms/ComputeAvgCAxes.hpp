#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

/**
 * @struct ComputeAvgCAxesInputValues
 * @brief Identifies average c-axis inputs.
 */
struct ORIENTATIONANALYSIS_EXPORT ComputeAvgCAxesInputValues
{
  DataPath QuatsArrayPath;
  DataPath FeatureIdsArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath CellFeatureDataPath;
  DataPath AvgCAxesArrayPath;
  DataPath CrystalStructuresArrayPath;
};

/**
 * @class ComputeAvgCAxes
 * @brief Computes one average crystallographic c axis for each feature.
 *
 * The executor rotates [001] from the crystal frame into the sample frame.
 * It flips antiparallel vectors before adding each feature contribution.
 *
 * Cell arrays are read in 4,096-tuple chunks. The feature output stays local
 * because feature IDs access it in random order. This avoids OOC chunk
 * thrashing. The executor writes the completed cache once.
 */
class ORIENTATIONANALYSIS_EXPORT ComputeAvgCAxes
{
public:
  /**
   * @brief Initializes average c-axis computation.
   * @param dataStructure Provides the selected arrays.
   * @param mesgHandler Supplies progress messages.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Identifies selected arrays.
   * @pre dataStructure, mesgHandler, shouldCancel, and inputValues outlive this
   *      executor.
   */
  ComputeAvgCAxes(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeAvgCAxesInputValues* inputValues);

  /**
   * @brief Destroys the average c-axis executor.
   */
  ~ComputeAvgCAxes() noexcept;

  ComputeAvgCAxes(const ComputeAvgCAxes&) = delete;
  ComputeAvgCAxes(ComputeAvgCAxes&&) noexcept = delete;
  ComputeAvgCAxes& operator=(const ComputeAvgCAxes&) = delete;
  ComputeAvgCAxes& operator=(ComputeAvgCAxes&&) noexcept = delete;

  /**
   * @brief Computes feature-average c axes.
   * @return An error if no hexagonal phase exists, or a warning for skipped
   *         non-hexagonal phases.
   *
   * Cancellation returns the current result without writing the local output
   * cache. Current bulk-I/O Result values are not inspected.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeAvgCAxesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
