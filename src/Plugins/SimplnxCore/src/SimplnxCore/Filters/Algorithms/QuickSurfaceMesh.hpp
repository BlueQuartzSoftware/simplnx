#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"

#include <random>
#include <string>

namespace nx::core
{

struct SIMPLNXCORE_EXPORT QuickSurfaceMeshInputValues
{
  bool FixProblemVoxels;
  bool RepairTriangleWinding;
  ChoicesParameter::ValueType BoundingBoxSkinMode;

  DataPath GridGeomDataPath;
  DataPath FeatureIdsArrayPath;
  MultiArraySelectionParameter::ValueType SelectedCellDataArrayPaths;
  MultiArraySelectionParameter::ValueType SelectedFeatureDataArrayPaths;
  DataPath TriangleGeometryPath;
  DataPath VertexGroupDataPath;
  DataPath NodeTypesDataPath;
  DataPath FaceGroupDataPath;
  DataPath FaceLabelsDataPath;
  MultiArraySelectionParameter::ValueType CreatedDataArrayPaths;
};

class SIMPLNXCORE_EXPORT QuickSurfaceMesh
{
public:
  using VertexStore = AbstractDataStore<IGeometry::SharedVertexList::value_type>;
  using TriStore = AbstractDataStore<IGeometry::SharedTriList::value_type>;
  using MeshIndexType = IGeometry::MeshIndexType;

  QuickSurfaceMesh(DataStructure& dataStructure, QuickSurfaceMeshInputValues* inputValues, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler);
  ~QuickSurfaceMesh() noexcept;

  QuickSurfaceMesh(const QuickSurfaceMesh&) = delete;
  QuickSurfaceMesh(QuickSurfaceMesh&&) noexcept = delete;
  QuickSurfaceMesh& operator=(const QuickSurfaceMesh&) = delete;
  QuickSurfaceMesh& operator=(QuickSurfaceMesh&&) noexcept = delete;

  Result<> operator()();

  /**
   * @brief
   */
  void correctProblemVoxels();

  /**
   * @brief
   * @param m_NodeIds
   * @param nodeCount
   * @param triangleCount
   * @param suppressedFaceCount Incremented once per bounding-box wall face that 'Omit Bounding Box
   * Skin' (BoundingBoxSkinMode::k_BackgroundBackedWallsOnly) suppresses. Always 0 when the mode is
   * Off. Lets the caller warn when the option is on but pruned nothing.
   */
  void determineActiveNodes(std::vector<MeshIndexType>& m_NodeIds, MeshIndexType& nodeCount, MeshIndexType& triangleCount, MeshIndexType& suppressedFaceCount);

  /**
   * @brief
   * @param m_NodeIds
   * @param nodeCount
   * @param triangleCount
   */
  void createNodesAndTriangles(std::vector<MeshIndexType>& m_NodeIds, MeshIndexType nodeCount, MeshIndexType triangleCount);

private:
  DataStructure& m_DataStructure;
  const QuickSurfaceMeshInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
