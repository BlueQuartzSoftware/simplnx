#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <EbsdLib/Core/EbsdLibConstants.h>

#include <atomic>
#include <vector>

namespace nx::core
{

/**
 * @brief The ComputeIPFColorsInputValues struct
 */
struct ORIENTATIONANALYSIS_EXPORT ComputeIPFColorsInputValues
{
  std::vector<float> referenceDirection;
  bool useMask;
  DataPath maskArrayPath;
  DataPath cellPhasesArrayPath;
  DataPath cellEulerAnglesArrayPath;
  DataPath crystalStructuresArrayPath;
  DataPath cellIpfColorsArrayPath;
  ebsdlib::ColorKeyKind colorKey = ebsdlib::ColorKeyKind::TSL;
};

/**
 * @brief
 */
class ORIENTATIONANALYSIS_EXPORT ComputeIPFColors
{
public:
  ComputeIPFColors(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, ComputeIPFColorsInputValues* inputValues);
  ~ComputeIPFColors() noexcept;

  ComputeIPFColors(const ComputeIPFColors&) = delete;            // Copy Constructor Not Implemented
  ComputeIPFColors(ComputeIPFColors&&) = delete;                 // Move Constructor Not Implemented
  ComputeIPFColors& operator=(const ComputeIPFColors&) = delete; // Copy Assignment Not Implemented
  ComputeIPFColors& operator=(ComputeIPFColors&&) = delete;      // Move Assignment Not Implemented

  Result<> operator()();

  /**
   * @brief incrementPhaseWarningCount
   */
  void incrementPhaseWarningCount();

  bool shouldCancel() const;

private:
  DataStructure& m_DataStructure;
  const IFilter::MessageHandler& m_MessageHandler;
  const std::atomic_bool& m_ShouldCancel;
  const ComputeIPFColorsInputValues* m_InputValues = nullptr;

  // Written concurrently from the ParallelDataAlgorithm worker, so it must be atomic.
  std::atomic_int32_t m_PhaseWarningCount = 0;
};

} // namespace nx::core
