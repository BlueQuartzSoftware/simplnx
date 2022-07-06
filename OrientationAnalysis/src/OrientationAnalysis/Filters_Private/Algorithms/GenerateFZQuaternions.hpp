#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  GenerateFZQuaternionsInputValues inputValues;

  inputValues.UseGoodVoxels = filterArgs.value<bool>(k_UseGoodVoxels_Key);
  inputValues.QuatsArrayPath = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  inputValues.CellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.GoodVoxelsArrayPath = filterArgs.value<DataPath>(k_GoodVoxelsArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.FZQuatsArrayPath = filterArgs.value<DataPath>(k_FZQuatsArrayPath_Key);

  return GenerateFZQuaternions(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct ORIENTATIONANALYSIS_EXPORT GenerateFZQuaternionsInputValues
{
  bool UseGoodVoxels;
  DataPath QuatsArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath GoodVoxelsArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath FZQuatsArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT GenerateFZQuaternions
{
public:
  GenerateFZQuaternions(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, GenerateFZQuaternionsInputValues* inputValues);
  ~GenerateFZQuaternions() noexcept;

  GenerateFZQuaternions(const GenerateFZQuaternions&) = delete;
  GenerateFZQuaternions(GenerateFZQuaternions&&) noexcept = delete;
  GenerateFZQuaternions& operator=(const GenerateFZQuaternions&) = delete;
  GenerateFZQuaternions& operator=(GenerateFZQuaternions&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const GenerateFZQuaternionsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
