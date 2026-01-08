#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  SetImageGeomOriginScalingInputValues inputValues;
  inputValues.CenterOrigin = filterArgs.value<BoolParameter::ValueType>(center_origin);
  inputValues.ChangeOrigin = filterArgs.value<BoolParameter::ValueType>(change_origin);
  inputValues.ChangeSpacing = filterArgs.value<BoolParameter::ValueType>(change_spacing);
  inputValues.InputImageGeometryPath = filterArgs.value<GeometrySelectionParameter::ValueType>(input_image_geometry_path);
  inputValues.Origin = filterArgs.value<VectorFloat32Parameter::ValueType>(origin);
  inputValues.Spacing = filterArgs.value<VectorFloat32Parameter::ValueType>(spacing);
  return SetImageGeomOriginScaling(dataStructure, messageHandler, shouldCancel, &inputValues)();

*/

namespace nx::core
{

struct SIMPLNXCORE_EXPORT SetImageGeomOriginScalingInputValues
{
  BoolParameter::ValueType CenterOrigin;
  BoolParameter::ValueType ChangeOrigin;
  BoolParameter::ValueType ChangeSpacing;
  GeometrySelectionParameter::ValueType InputImageGeometryPath;
  VectorFloat32Parameter::ValueType Origin;
  VectorFloat32Parameter::ValueType Spacing;
};

/**
 * @class SetImageGeomOriginScaling
 * @brief This algorithm implements support code for the SetImageGeomOriginScalingFilter
 */

class SIMPLNXCORE_EXPORT SetImageGeomOriginScaling
{
public:
  SetImageGeomOriginScaling(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, SetImageGeomOriginScalingInputValues* inputValues);
  ~SetImageGeomOriginScaling() noexcept;

  SetImageGeomOriginScaling(const SetImageGeomOriginScaling&) = delete;
  SetImageGeomOriginScaling(SetImageGeomOriginScaling&&) noexcept = delete;
  SetImageGeomOriginScaling& operator=(const SetImageGeomOriginScaling&) = delete;
  SetImageGeomOriginScaling& operator=(SetImageGeomOriginScaling&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const SetImageGeomOriginScalingInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
