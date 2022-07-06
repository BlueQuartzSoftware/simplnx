#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  SetOriginResolutionImageGeomInputValues inputValues;

  inputValues.DataContainerName = filterArgs.value<DataPath>(k_DataContainerName_Key);
  inputValues.ChangeOrigin = filterArgs.value<bool>(k_ChangeOrigin_Key);
  inputValues.Origin = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  inputValues.ChangeResolution = filterArgs.value<bool>(k_ChangeResolution_Key);
  inputValues.Spacing = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);

  return SetOriginResolutionImageGeom(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT SetOriginResolutionImageGeomInputValues
{
  DataPath DataContainerName;
  bool ChangeOrigin;
  VectorFloat32Parameter::ValueType Origin;
  bool ChangeResolution;
  VectorFloat32Parameter::ValueType Spacing;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT SetOriginResolutionImageGeom
{
public:
  SetOriginResolutionImageGeom(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, SetOriginResolutionImageGeomInputValues* inputValues);
  ~SetOriginResolutionImageGeom() noexcept;

  SetOriginResolutionImageGeom(const SetOriginResolutionImageGeom&) = delete;
  SetOriginResolutionImageGeom(SetOriginResolutionImageGeom&&) noexcept = delete;
  SetOriginResolutionImageGeom& operator=(const SetOriginResolutionImageGeom&) = delete;
  SetOriginResolutionImageGeom& operator=(SetOriginResolutionImageGeom&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const SetOriginResolutionImageGeomInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
