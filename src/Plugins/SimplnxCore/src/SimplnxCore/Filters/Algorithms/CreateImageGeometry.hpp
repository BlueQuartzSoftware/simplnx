#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  CreateImageGeometryInputValues inputValues;
  inputValues.CellDataName = filterArgs.value<DataObjectNameParameter::ValueType>(cell_data_name);
  inputValues.Dimensions = filterArgs.value<VectorUInt64Parameter::ValueType>(dimensions);
  inputValues.Origin = filterArgs.value<VectorFloat32Parameter::ValueType>(origin);
  inputValues.OutputImageGeometryPath = filterArgs.value<DataGroupCreationParameter::ValueType>(output_image_geometry_path);
  inputValues.Spacing = filterArgs.value<VectorFloat32Parameter::ValueType>(spacing);
  return CreateImageGeometry(dataStructure, messageHandler, shouldCancel, &inputValues)();

*/

namespace nx::core
{

struct SIMPLNXCORE_EXPORT CreateImageGeometryInputValues
{
  DataObjectNameParameter::ValueType CellDataName;
  VectorUInt64Parameter::ValueType Dimensions;
  VectorFloat32Parameter::ValueType Origin;
  DataGroupCreationParameter::ValueType OutputImageGeometryPath;
  VectorFloat32Parameter::ValueType Spacing;
};

/**
 * @class CreateImageGeometry
 * @brief This algorithm implements support code for the CreateImageGeometryFilter
 */

class SIMPLNXCORE_EXPORT CreateImageGeometry
{
public:
  CreateImageGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CreateImageGeometryInputValues* inputValues);
  ~CreateImageGeometry() noexcept;

  CreateImageGeometry(const CreateImageGeometry&) = delete;
  CreateImageGeometry(CreateImageGeometry&&) noexcept = delete;
  CreateImageGeometry& operator=(const CreateImageGeometry&) = delete;
  CreateImageGeometry& operator=(CreateImageGeometry&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const CreateImageGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
