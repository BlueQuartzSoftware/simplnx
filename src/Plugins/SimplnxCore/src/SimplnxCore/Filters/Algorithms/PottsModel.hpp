#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <vector>

namespace nx::core
{
/**
 * @struct PottsModelInputValues
 * @brief Contains the values that configure a Potts-model execution.
 */
struct SIMPLNXCORE_EXPORT PottsModelInputValues
{
  int32 Iterations = 100;
  float64 Temperature = 273.0;
  bool PeriodicBoundaries = false;
  bool UseMask = false;
  DataPath MaskArrayPath;
  std::vector<DataPath> IgnoredDataArrayPaths;
  DataPath FeatureIdsArrayPath;
  uint64 SeedValue = 0;
};

/**
 * @class PottsModel
 * @brief Applies Monte Carlo spin flips to an image geometry feature-ID array.
 */
class SIMPLNXCORE_EXPORT PottsModel
{
public:
  /**
   * @brief Constructs the algorithm.
   * @param dataStructure Data structure that contains the input arrays.
   * @param messageHandler Receives progress messages.
   * @param shouldCancel Indicates that the algorithm must stop.
   * @param inputValues Values that configure the algorithm. The caller retains ownership.
   */
  PottsModel(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel, const PottsModelInputValues* inputValues);

  /**
   * @brief Destroys the algorithm.
   */
  ~PottsModel() noexcept;

  PottsModel(const PottsModel&) = delete;
  PottsModel(PottsModel&&) noexcept = delete;
  PottsModel& operator=(const PottsModel&) = delete;
  PottsModel& operator=(PottsModel&&) noexcept = delete;

  /**
   * @brief Runs the Potts-model iterations.
   * @return Success, cancellation, or an execution error.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const PottsModelInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
