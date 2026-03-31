#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"

#include <random>
#include <string>

namespace nx::core
{

struct SIMPLNXCORE_EXPORT QuickSurfaceMeshInputValues
{
  bool FixProblemVoxels;
  bool RepairTriangleWinding;
  bool GenerateTripleLines;

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

class SIMPLNXCORE_EXPORT QuickSurfaceMeshDirect
{
public:
  using VertexStore = AbstractDataStore<IGeometry::SharedVertexList::value_type>;
  using TriStore = AbstractDataStore<IGeometry::SharedTriList::value_type>;
  using MeshIndexType = IGeometry::MeshIndexType;

  QuickSurfaceMeshDirect(DataStructure& dataStructure, QuickSurfaceMeshInputValues* inputValues, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler);
  ~QuickSurfaceMeshDirect() noexcept;

  QuickSurfaceMeshDirect(const QuickSurfaceMeshDirect&) = delete;
  QuickSurfaceMeshDirect(QuickSurfaceMeshDirect&&) noexcept = delete;
  QuickSurfaceMeshDirect& operator=(const QuickSurfaceMeshDirect&) = delete;
  QuickSurfaceMeshDirect& operator=(QuickSurfaceMeshDirect&&) noexcept = delete;

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
   */
  void determineActiveNodes(std::vector<MeshIndexType>& m_NodeIds, MeshIndexType& nodeCount, MeshIndexType& triangleCount);

  /**
   * @brief
   * @param m_NodeIds
   * @param nodeCount
   * @param triangleCount
   */
  void createNodesAndTriangles(std::vector<MeshIndexType>& m_NodeIds, MeshIndexType nodeCount, MeshIndexType triangleCount);

  /**
   * @brief generateTripleLines
   */
  void generateTripleLines();

private:
  DataStructure& m_DataStructure;
  const QuickSurfaceMeshInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
  bool m_GenerateTripleLines = false;
};
} // namespace nx::core
