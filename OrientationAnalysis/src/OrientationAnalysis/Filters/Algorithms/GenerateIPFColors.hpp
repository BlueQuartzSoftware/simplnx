#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/IDataArray.hpp"
#include "complex/Filter/IFilter.hpp"


#include <vector>

namespace complex
{

/**
 * @brief The GenerateIPFColorsInputValues struct
 */
struct ORIENTATIONANALYSIS_EXPORT GenerateIPFColorsInputValues
{
  std::vector<float> referenceDirection;
  bool useGoodVoxels;
  DataPath goodVoxelsArrayPath;
  DataPath cellPhasesArrayPath;
  DataPath cellEulerAnglesArrayPath;
  DataPath crystalStructuresArrayPath;
  DataPath cellIpfColorsArrayPath;

};

/**
 * @brief
 */
class ORIENTATIONANALYSIS_EXPORT GenerateIPFColors
{
public:
  GenerateIPFColors(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, GenerateIPFColorsInputValues* inputValues);
  ~GenerateIPFColors() noexcept;

  GenerateIPFColors(const GenerateIPFColors&) = delete;            // Copy Constructor Not Implemented
  GenerateIPFColors(GenerateIPFColors&&) = delete;                 // Move Constructor Not Implemented
  GenerateIPFColors& operator=(const GenerateIPFColors&) = delete; // Copy Assignment Not Implemented
  GenerateIPFColors& operator=(GenerateIPFColors&&) = delete;      // Move Assignment Not Implemented

  Result<> operator()();


  /**
   * @brief incrementPhaseWarningCount
   */
  void incrementPhaseWarningCount();

protected:


private:
  DataStructure& m_DataStructure;
  const IFilter::MessageHandler& m_MessageHandler;
  const std::atomic_bool& m_ShouldCancel;
  const GenerateIPFColorsInputValues* m_InputValues = nullptr;

  int32_t m_PhaseWarningCount = 0;

};

} // namespace complex
