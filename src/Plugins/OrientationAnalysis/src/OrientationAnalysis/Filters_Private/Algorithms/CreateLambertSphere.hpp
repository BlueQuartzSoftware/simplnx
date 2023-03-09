#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  CreateLambertSphereInputValues inputValues;

  inputValues.Hemisphere = filterArgs.value<ChoicesParameter::ValueType>(k_Hemisphere_Key);
  inputValues.CreateVertexGeometry = filterArgs.value<bool>(k_CreateVertexGeometry_Key);
  inputValues.VertexDataContainerName = filterArgs.value<DataPath>(k_VertexDataContainerName_Key);
  inputValues.CreateEdgeGeometry = filterArgs.value<bool>(k_CreateEdgeGeometry_Key);
  inputValues.EdgeDataContainerName = filterArgs.value<DataPath>(k_EdgeDataContainerName_Key);
  inputValues.CreateTriangleGeometry = filterArgs.value<bool>(k_CreateTriangleGeometry_Key);
  inputValues.TriangleDataContainerName = filterArgs.value<DataPath>(k_TriangleDataContainerName_Key);
  inputValues.CreateQuadGeometry = filterArgs.value<bool>(k_CreateQuadGeometry_Key);
  inputValues.QuadDataContainerName = filterArgs.value<DataPath>(k_QuadDataContainerName_Key);
  inputValues.ImageDataArrayPath = filterArgs.value<DataPath>(k_ImageDataArrayPath_Key);

  return CreateLambertSphere(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct ORIENTATIONANALYSIS_EXPORT CreateLambertSphereInputValues
{
  ChoicesParameter::ValueType Hemisphere;
  bool CreateVertexGeometry;
  DataPath VertexDataContainerName;
  bool CreateEdgeGeometry;
  DataPath EdgeDataContainerName;
  bool CreateTriangleGeometry;
  DataPath TriangleDataContainerName;
  bool CreateQuadGeometry;
  DataPath QuadDataContainerName;
  DataPath ImageDataArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT CreateLambertSphere
{
public:
  CreateLambertSphere(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CreateLambertSphereInputValues* inputValues);
  ~CreateLambertSphere() noexcept;

  CreateLambertSphere(const CreateLambertSphere&) = delete;
  CreateLambertSphere(CreateLambertSphere&&) noexcept = delete;
  CreateLambertSphere& operator=(const CreateLambertSphere&) = delete;
  CreateLambertSphere& operator=(CreateLambertSphere&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const CreateLambertSphereInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
