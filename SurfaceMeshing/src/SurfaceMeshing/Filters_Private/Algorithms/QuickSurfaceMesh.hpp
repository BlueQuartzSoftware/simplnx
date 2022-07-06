#pragma once

#include "SurfaceMeshing/SurfaceMeshing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  QuickSurfaceMeshInputValues inputValues;

  inputValues.FixProblemVoxels = filterArgs.value<bool>(k_FixProblemVoxels_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.SelectedDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  inputValues.SurfaceDataContainerName = filterArgs.value<DataPath>(k_SurfaceDataContainerName_Key);
  inputValues.VertexAttributeMatrixName = filterArgs.value<DataPath>(k_VertexAttributeMatrixName_Key);
  inputValues.NodeTypesArrayName = filterArgs.value<DataPath>(k_NodeTypesArrayName_Key);
  inputValues.FaceAttributeMatrixName = filterArgs.value<DataPath>(k_FaceAttributeMatrixName_Key);
  inputValues.FaceLabelsArrayName = filterArgs.value<DataPath>(k_FaceLabelsArrayName_Key);
  inputValues.FeatureAttributeMatrixName = filterArgs.value<DataPath>(k_FeatureAttributeMatrixName_Key);

  return QuickSurfaceMesh(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct SURFACEMESHING_EXPORT QuickSurfaceMeshInputValues
{
  bool FixProblemVoxels;
  DataPath FeatureIdsArrayPath;
  MultiArraySelectionParameter::ValueType SelectedDataArrayPaths;
  DataPath SurfaceDataContainerName;
  DataPath VertexAttributeMatrixName;
  DataPath NodeTypesArrayName;
  DataPath FaceAttributeMatrixName;
  DataPath FaceLabelsArrayName;
  DataPath FeatureAttributeMatrixName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SURFACEMESHING_EXPORT QuickSurfaceMesh
{
public:
  QuickSurfaceMesh(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, QuickSurfaceMeshInputValues* inputValues);
  ~QuickSurfaceMesh() noexcept;

  QuickSurfaceMesh(const QuickSurfaceMesh&) = delete;
  QuickSurfaceMesh(QuickSurfaceMesh&&) noexcept = delete;
  QuickSurfaceMesh& operator=(const QuickSurfaceMesh&) = delete;
  QuickSurfaceMesh& operator=(QuickSurfaceMesh&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const QuickSurfaceMeshInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
