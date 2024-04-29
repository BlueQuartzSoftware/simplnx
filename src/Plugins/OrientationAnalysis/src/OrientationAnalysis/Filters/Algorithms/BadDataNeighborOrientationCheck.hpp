#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  BadDataNeighborOrientationCheckInputValues inputValues;

  inputValues.MisorientationTolerance = filterArgs.value<float32>(k_MisorientationTolerance_Key);
  inputValues.NumberOfNeighbors = filterArgs.value<int32>(k_NumberOfNeighbors_Key);
  inputValues.QuatsArrayPath = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  inputValues.MaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputValues.CellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);

  return BadDataNeighborOrientationCheck(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT BadDataNeighborOrientationCheckInputValues
{
  float32 MisorientationTolerance;
  int32 NumberOfNeighbors;
  DataPath ImageGeomPath;
  DataPath QuatsArrayPath;
  DataPath MaskArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath CrystalStructuresArrayPath;
};

/**
 * @class
 */
class ORIENTATIONANALYSIS_EXPORT BadDataNeighborOrientationCheck
{
public:
  BadDataNeighborOrientationCheck(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                  BadDataNeighborOrientationCheckInputValues* inputValues);
  ~BadDataNeighborOrientationCheck() noexcept;

  BadDataNeighborOrientationCheck(const BadDataNeighborOrientationCheck&) = delete;
  BadDataNeighborOrientationCheck(BadDataNeighborOrientationCheck&&) noexcept = delete;
  BadDataNeighborOrientationCheck& operator=(const BadDataNeighborOrientationCheck&) = delete;
  BadDataNeighborOrientationCheck& operator=(BadDataNeighborOrientationCheck&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const BadDataNeighborOrientationCheckInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
