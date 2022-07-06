#pragma once

#include "SyntheticBuilding/SyntheticBuilding_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  AddBadDataInputValues inputValues;

  inputValues.PoissonNoise = filterArgs.value<bool>(k_PoissonNoise_Key);
  inputValues.PoissonVolFraction = filterArgs.value<float32>(k_PoissonVolFraction_Key);
  inputValues.BoundaryNoise = filterArgs.value<bool>(k_BoundaryNoise_Key);
  inputValues.BoundaryVolFraction = filterArgs.value<float32>(k_BoundaryVolFraction_Key);
  inputValues.GBEuclideanDistancesArrayPath = filterArgs.value<DataPath>(k_GBEuclideanDistancesArrayPath_Key);

  return AddBadData(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct SYNTHETICBUILDING_EXPORT AddBadDataInputValues
{
  bool PoissonNoise;
  float32 PoissonVolFraction;
  bool BoundaryNoise;
  float32 BoundaryVolFraction;
  DataPath GBEuclideanDistancesArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SYNTHETICBUILDING_EXPORT AddBadData
{
public:
  AddBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, AddBadDataInputValues* inputValues);
  ~AddBadData() noexcept;

  AddBadData(const AddBadData&) = delete;
  AddBadData(AddBadData&&) noexcept = delete;
  AddBadData& operator=(const AddBadData&) = delete;
  AddBadData& operator=(AddBadData&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const AddBadDataInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
