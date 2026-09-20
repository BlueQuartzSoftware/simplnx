#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <vector>

namespace nx::core
{

/**
 * @struct ComputeAvgOrientationsInputValues
 * @brief Identifies average-orientation inputs.
 *
 * RandomSeed fixes EM initialization. NumEMIterations and NumIterations set
 * the vMF and Watson estimate limits.
 */
struct ORIENTATIONANALYSIS_EXPORT ComputeAvgOrientationsInputValues
{
  DataPath cellFeatureIdsArrayPath;
  DataPath cellPhasesArrayPath;
  DataPath cellQuatsArrayPath;
  DataPath crystalStructuresArrayPath;

  bool useRodriguesAverage;
  bool useVonMisesAverage;
  bool useWatsonAverage;
  DataPath avgQuatsArrayPath;
  DataPath avgEulerAnglesArrayPath;

  DataPath VMFQuatsArrayPath;
  DataPath VMFEulerAnglesArrayPath;
  DataPath VMFKappaArrayPath;

  DataPath WatsonQuatsArrayPath;
  DataPath WatsonEulerAnglesArrayPath;
  DataPath WatsonKappaArrayPath;

  uint32 RandomSeed = 43514;
  int32 NumEMIterations = 5;
  int32 NumIterations = 10;
};

/**
 * @class ComputeAvgOrientations
 * @brief Computes feature-average orientations from cell quaternions.
 *
 * Rodrigues uses a running quaternion average. vMF and Watson use EbsdLib EM
 * estimators. The selected methods write independent feature outputs.
 *
 * The scanline path reads cells in 65,536-tuple chunks and holds feature
 * accumulators locally. It uses deterministic external grouping for vMF and
 * Watson. EbsdLib still requires one full feature vector, so their peak cell
 * memory is the largest feature.
 */
class ORIENTATIONANALYSIS_EXPORT ComputeAvgOrientations
{
public:
  /**
   * @brief Initializes average-orientation computation.
   * @param dataStructure Provides selected arrays.
   * @param msgHandler Supplies progress messages.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Identifies selected arrays and averaging methods.
   * @pre dataStructure, msgHandler, shouldCancel, and inputValues outlive this
   *      executor.
   */
  ComputeAvgOrientations(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, ComputeAvgOrientationsInputValues* inputValues);
  /**
   * @brief Destroys the average-orientation executor.
   */
  ~ComputeAvgOrientations() noexcept;

  ComputeAvgOrientations(const ComputeAvgOrientations&) = delete;
  ComputeAvgOrientations(ComputeAvgOrientations&&) = delete;
  ComputeAvgOrientations& operator=(const ComputeAvgOrientations&) = delete;
  ComputeAvgOrientations& operator=(ComputeAvgOrientations&&) = delete;

  /**
   * @brief Executes selected averaging methods.
   * @return Result from the selected direct or scanline path.
   */
  Result<> operator()();

  /**
   * @brief Sends a throttled progress message.
   * @param counter Specifies completed feature estimates.
   *
   * The mutex serializes counter and time updates from vMF and Watson workers.
   */
  void sendThreadSafeProgressMessage(usize counter);

  /**
   * @brief Returns the current cancellation state.
   * @return True if cancellation has been requested.
   */
  bool getCancel() const
  {
    return m_ShouldCancel;
  }

private:
  class DirectAlgorithm;
  class ScanlineAlgorithm;

  DataStructure& m_DataStructure;
  const IFilter::MessageHandler& m_MessageHandler;
  const std::atomic_bool& m_ShouldCancel;
  const ComputeAvgOrientationsInputValues* m_InputValues = nullptr;

  /**
   * @brief Executes the direct averaging path.
   * @return Result from selected averaging methods.
   */
  Result<> executeDirect();
  /**
   * @brief Executes the checked scanline averaging path.
   * @return Result from validation, bulk I/O, or selected averaging methods.
   */
  Result<> executeScanline();

  /**
   * @brief Computes direct Rodrigues averages.
   * @return Result from Rodrigues averaging.
   */
  Result<> computeRodriguesAverage();

  /**
   * @brief Computes direct vMF and Watson averages.
   * @return Result from directional estimation.
   */
  Result<> computeVmfWatsonAverage();

  /**
   * @brief Computes scanline Rodrigues averages.
   * @return Result from validation, bulk I/O, or phase checks.
   */
  Result<> computeRodriguesAverageScanline();
  /**
   * @brief Computes scanline vMF and Watson averages.
   * @return Result from external grouping, bulk I/O, or directional estimation.
   */
  Result<> computeVmfWatsonAverageScanline();

  // Thread safe Progress Message
  std::chrono::steady_clock::time_point m_InitialPoint = std::chrono::steady_clock::now();
  mutable std::mutex m_ProgressMessage_Mutex;
  size_t m_NumberOfFeatures = 0;
  size_t m_ProgressCounter = 0;
};

} // namespace nx::core
