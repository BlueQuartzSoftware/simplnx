#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

#include <chrono>

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT FindKernelAvgMisorientationsInputValues
{
  VectorInt32Parameter::ValueType KernelSize;
  DataPath FeatureIdsArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath QuatsArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath KernelAverageMisorientationsArrayName;
  DataPath InputImageGeometry;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT FindKernelAvgMisorientations
{
public:
  FindKernelAvgMisorientations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindKernelAvgMisorientationsInputValues* inputValues);
  ~FindKernelAvgMisorientations() noexcept;

  FindKernelAvgMisorientations(const FindKernelAvgMisorientations&) = delete;
  FindKernelAvgMisorientations(FindKernelAvgMisorientations&&) noexcept = delete;
  FindKernelAvgMisorientations& operator=(const FindKernelAvgMisorientations&) = delete;
  FindKernelAvgMisorientations& operator=(FindKernelAvgMisorientations&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

  void sendThreadSafeProgressMessage(usize counter);

private:
  DataStructure& m_DataStructure;
  const FindKernelAvgMisorientationsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  // Thread safe Progress Message
  std::chrono::steady_clock::time_point m_InitialPoint = std::chrono::steady_clock::now();
  mutable std::mutex m_ProgressMessage_Mutex;
  size_t m_TotalElements = 0;
  size_t m_ProgressCounter = 0;
  size_t m_LastProgressInt = 0;
};

} // namespace nx::core
