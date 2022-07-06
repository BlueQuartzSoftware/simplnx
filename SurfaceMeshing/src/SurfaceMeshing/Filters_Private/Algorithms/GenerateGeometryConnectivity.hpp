#pragma once

#include "SurfaceMeshing/SurfaceMeshing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  GenerateGeometryConnectivityInputValues inputValues;

  inputValues.GenerateVertexTriangleLists = filterArgs.value<bool>(k_GenerateVertexTriangleLists_Key);
  inputValues.GenerateTriangleNeighbors = filterArgs.value<bool>(k_GenerateTriangleNeighbors_Key);
  inputValues.SurfaceDataContainerName = filterArgs.value<DataPath>(k_SurfaceDataContainerName_Key);

  return GenerateGeometryConnectivity(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct SURFACEMESHING_EXPORT GenerateGeometryConnectivityInputValues
{
  bool GenerateVertexTriangleLists;
  bool GenerateTriangleNeighbors;
  DataPath SurfaceDataContainerName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SURFACEMESHING_EXPORT GenerateGeometryConnectivity
{
public:
  GenerateGeometryConnectivity(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, GenerateGeometryConnectivityInputValues* inputValues);
  ~GenerateGeometryConnectivity() noexcept;

  GenerateGeometryConnectivity(const GenerateGeometryConnectivity&) = delete;
  GenerateGeometryConnectivity(GenerateGeometryConnectivity&&) noexcept = delete;
  GenerateGeometryConnectivity& operator=(const GenerateGeometryConnectivity&) = delete;
  GenerateGeometryConnectivity& operator=(GenerateGeometryConnectivity&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const GenerateGeometryConnectivityInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
