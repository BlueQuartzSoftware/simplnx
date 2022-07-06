#pragma once

#include "SyntheticBuilding/SyntheticBuilding_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  StatsGeneratorFilterInputValues inputValues;

  inputValues.StatsGenerator = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_StatsGenerator_Key);
  inputValues.StatsGeneratorDataContainerName = filterArgs.value<DataPath>(k_StatsGeneratorDataContainerName_Key);
  inputValues.CellEnsembleAttributeMatrixName = filterArgs.value<DataPath>(k_CellEnsembleAttributeMatrixName_Key);
  inputValues.StatsDataArrayName = filterArgs.value<DataPath>(k_StatsDataArrayName_Key);
  inputValues.CrystalStructuresArrayName = filterArgs.value<DataPath>(k_CrystalStructuresArrayName_Key);
  inputValues.PhaseTypesArrayName = filterArgs.value<DataPath>(k_PhaseTypesArrayName_Key);
  inputValues.PhaseNamesArrayName = filterArgs.value<DataPath>(k_PhaseNamesArrayName_Key);

  return StatsGeneratorFilter(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct SYNTHETICBUILDING_EXPORT StatsGeneratorFilterInputValues
{
  <<<NOT_IMPLEMENTED>>> StatsGenerator;
  /*[x]*/ DataPath StatsGeneratorDataContainerName;
  DataPath CellEnsembleAttributeMatrixName;
  DataPath StatsDataArrayName;
  DataPath CrystalStructuresArrayName;
  DataPath PhaseTypesArrayName;
  DataPath PhaseNamesArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SYNTHETICBUILDING_EXPORT StatsGeneratorFilter
{
public:
  StatsGeneratorFilter(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, StatsGeneratorFilterInputValues* inputValues);
  ~StatsGeneratorFilter() noexcept;

  StatsGeneratorFilter(const StatsGeneratorFilter&) = delete;
  StatsGeneratorFilter(StatsGeneratorFilter&&) noexcept = delete;
  StatsGeneratorFilter& operator=(const StatsGeneratorFilter&) = delete;
  StatsGeneratorFilter& operator=(StatsGeneratorFilter&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const StatsGeneratorFilterInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
