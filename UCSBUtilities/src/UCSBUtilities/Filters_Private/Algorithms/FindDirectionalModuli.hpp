#pragma once

#include "UCSBUtilities/UCSBUtilities_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FindDirectionalModuliInputValues inputValues;

  inputValues.LoadingDirection = filterArgs.value<VectorFloat32Parameter::ValueType>(k_LoadingDirection_Key);
  inputValues.FeaturePhasesArrayPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.AvgQuatsArrayPath = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.CrystalCompliancesArrayPath = filterArgs.value<DataPath>(k_CrystalCompliancesArrayPath_Key);
  inputValues.DirectionalModuliArrayName = filterArgs.value<DataPath>(k_DirectionalModuliArrayName_Key);

  return FindDirectionalModuli(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct UCSBUTILITIES_EXPORT FindDirectionalModuliInputValues
{
  VectorFloat32Parameter::ValueType LoadingDirection;
  DataPath FeaturePhasesArrayPath;
  DataPath AvgQuatsArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath CrystalCompliancesArrayPath;
  DataPath DirectionalModuliArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class UCSBUTILITIES_EXPORT FindDirectionalModuli
{
public:
  FindDirectionalModuli(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindDirectionalModuliInputValues* inputValues);
  ~FindDirectionalModuli() noexcept;

  FindDirectionalModuli(const FindDirectionalModuli&) = delete;
  FindDirectionalModuli(FindDirectionalModuli&&) noexcept = delete;
  FindDirectionalModuli& operator=(const FindDirectionalModuli&) = delete;
  FindDirectionalModuli& operator=(FindDirectionalModuli&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindDirectionalModuliInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
