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
  bool GenerateTripleLines;

  DataPath GridGeomDataPath;
  DataPath FeatureIdsArrayPath;
  MultiArraySelectionParameter::ValueType SelectedDataArrayPaths;
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
  QuickSurfaceMesh(DataStructure& dataStructure, QuickSurfaceMeshInputValues* inputValues, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler);
  ~QuickSurfaceMesh() noexcept;

  QuickSurfaceMesh(const QuickSurfaceMesh&) = delete;
  QuickSurfaceMesh(QuickSurfaceMesh&&) noexcept = delete;
  QuickSurfaceMesh& operator=(const QuickSurfaceMesh&) = delete;
  QuickSurfaceMesh& operator=(QuickSurfaceMesh&&) noexcept = delete;

  using MeshIndexType = IGeometry::MeshIndexType;

  Result<> operator()();

  /**
   *
   * @param grid
   * @param x
   * @param y
   * @param z
   * @param verts
   * @param nodeIndex
   */
  void getGridCoordinates(const IGridGeometry* grid, size_t x, size_t y, size_t z, IGeometry::SharedVertexList& verts, IGeometry::MeshIndexType nodeIndex);

  /**
   *
   * @param featureIds
   * @param v1
   * @param v2
   * @param v3
   * @param v4
   * @param v5
   * @param v6
   */
  void flipProblemVoxelCase1(Int32AbstractDataStore& featureIds, MeshIndexType v1, MeshIndexType v2, MeshIndexType v3, MeshIndexType v4, MeshIndexType v5, MeshIndexType v6) const;

  /**
   * @brief flipProblemVoxelCase2
   * @param v1
   * @param v2
   * @param v3
   * @param v4
   */
  void flipProblemVoxelCase2(Int32AbstractDataStore& featureIds, MeshIndexType v1, MeshIndexType v2, MeshIndexType v3, MeshIndexType v4) const;

  /**
   * @brief flipProblemVoxelCase3
   * @param v1
   * @param v2
   * @param v3
   */
  void flipProblemVoxelCase3(Int32AbstractDataStore& featureIds, MeshIndexType v1, MeshIndexType v2, MeshIndexType v3) const;

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
   * @brief
   */
  void copyCellDataToTriangleFaceArray();

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
