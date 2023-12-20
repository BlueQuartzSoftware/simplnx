#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FindSchmidsInputValues inputValues;

  inputValues.LoadingDirection = filterArgs.value<VectorFloat32Parameter::ValueType>(k_LoadingDirection_Key);
  inputValues.StoreAngleComponents = filterArgs.value<bool>(k_StoreAngleComponents_Key);
  inputValues.OverrideSystem = filterArgs.value<bool>(k_OverrideSystem_Key);
  inputValues.SlipPlane = filterArgs.value<VectorFloat32Parameter::ValueType>(k_SlipPlane_Key);
  inputValues.SlipDirection = filterArgs.value<VectorFloat32Parameter::ValueType>(k_SlipDirection_Key);
  inputValues.FeaturePhasesArrayPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.AvgQuatsArrayPath = filterArgs.value<DataPath>(k_AvgQuatsArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.SchmidsArrayName = filterArgs.value<DataPath>(k_SchmidsArrayName_Key);
  inputValues.SlipSystemsArrayName = filterArgs.value<DataPath>(k_SlipSystemsArrayName_Key);
  inputValues.PolesArrayName = filterArgs.value<DataPath>(k_PolesArrayName_Key);
  inputValues.PhisArrayName = filterArgs.value<DataPath>(k_PhisArrayName_Key);
  inputValues.LambdasArrayName = filterArgs.value<DataPath>(k_LambdasArrayName_Key);

  return FindSchmids(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT FindSchmidsInputValues
{
  VectorFloat32Parameter::ValueType LoadingDirection;
  bool StoreAngleComponents;
  bool OverrideSystem;
  VectorFloat32Parameter::ValueType SlipPlane;
  VectorFloat32Parameter::ValueType SlipDirection;
  DataPath FeaturePhasesArrayPath;
  DataPath AvgQuatsArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath SchmidsArrayName;
  DataPath SlipSystemsArrayName;
  DataPath PolesArrayName;
  DataPath PhisArrayName;
  DataPath LambdasArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT FindSchmids
{
public:
  FindSchmids(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindSchmidsInputValues* inputValues);
  ~FindSchmids() noexcept;

  FindSchmids(const FindSchmids&) = delete;
  FindSchmids(FindSchmids&&) noexcept = delete;
  FindSchmids& operator=(const FindSchmids&) = delete;
  FindSchmids& operator=(FindSchmids&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindSchmidsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
