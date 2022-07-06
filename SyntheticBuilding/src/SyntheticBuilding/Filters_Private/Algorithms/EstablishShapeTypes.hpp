#pragma once

#include "SyntheticBuilding/SyntheticBuilding_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  EstablishShapeTypesInputValues inputValues;

  inputValues.InputPhaseTypesArrayPath = filterArgs.value<DataPath>(k_InputPhaseTypesArrayPath_Key);
  inputValues.ShapeTypesArrayName = filterArgs.value<DataPath>(k_ShapeTypesArrayName_Key);
  inputValues.ShapeTypeData = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_ShapeTypeData_Key);

  return EstablishShapeTypes(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct SYNTHETICBUILDING_EXPORT EstablishShapeTypesInputValues
{
  DataPath InputPhaseTypesArrayPath;
  DataPath ShapeTypesArrayName;
  <<<NOT_IMPLEMENTED>>> ShapeTypeData;
  /*[x]*/
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SYNTHETICBUILDING_EXPORT EstablishShapeTypes
{
public:
  EstablishShapeTypes(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, EstablishShapeTypesInputValues* inputValues);
  ~EstablishShapeTypes() noexcept;

  EstablishShapeTypes(const EstablishShapeTypes&) = delete;
  EstablishShapeTypes(EstablishShapeTypes&&) noexcept = delete;
  EstablishShapeTypes& operator=(const EstablishShapeTypes&) = delete;
  EstablishShapeTypes& operator=(EstablishShapeTypes&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const EstablishShapeTypesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
