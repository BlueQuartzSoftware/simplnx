#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/Geometry/AbstractGeometry.hpp"
#include "complex/DataStructure/Geometry/AbstractGeometryGrid.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"

#include <string>

namespace complex
{

struct COMPLEXCORE_EXPORT QuickSurfaceMeshInputValues
{
  bool pFixProblemVoxels;
  DataPath pGridGeomDataPath;
  DataPath pFeatureIdsArrayPath;
  MultiArraySelectionParameter::ValueType pSelectedDataArrayPaths;
  DataPath pTriangleGeometryPath;
  DataPath pVertexGroupDataPath;
  DataPath pNodeTypesDataPath;
  DataPath pFaceGroupDataPath;
  DataPath pFaceLabelsDataPath;
  MultiArraySelectionParameter::ValueType pCreatedDataArrayPaths;
  bool pGenerateTripleLines;
};

class COMPLEXCORE_EXPORT QuickSurfaceMesh
{

public:
  QuickSurfaceMesh(DataStructure& dataStructure, QuickSurfaceMeshInputValues* inputValues, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler);
  ~QuickSurfaceMesh() noexcept;

  QuickSurfaceMesh(const QuickSurfaceMesh&) = delete;
  QuickSurfaceMesh(QuickSurfaceMesh&&) noexcept = delete;
  QuickSurfaceMesh& operator=(const QuickSurfaceMesh&) = delete;
  QuickSurfaceMesh& operator=(QuickSurfaceMesh&&) noexcept = delete;

  using MeshIndexType = AbstractGeometry::MeshIndexType;

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
  void getGridCoordinates(const AbstractGeometryGrid* grid, size_t x, size_t y, size_t z, AbstractGeometry::SharedVertexList& verts, AbstractGeometry::MeshIndexType nodeIndex);

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
  void flipProblemVoxelCase1(Int32Array& featureIds, MeshIndexType v1, MeshIndexType v2, MeshIndexType v3, MeshIndexType v4, MeshIndexType v5, MeshIndexType v6);

  /**
   * @brief flipProblemVoxelCase2
   * @param v1
   * @param v2
   * @param v3
   * @param v4
   */
  void flipProblemVoxelCase2(Int32Array& featureIds, MeshIndexType v1, MeshIndexType v2, MeshIndexType v3, MeshIndexType v4);

  /**
   * @brief flipProblemVoxelCase3
   * @param v1
   * @param v2
   * @param v3
   */
  void flipProblemVoxelCase3(Int32Array& featureIds, MeshIndexType v1, MeshIndexType v2, MeshIndexType v3);

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
  const QuickSurfaceMeshInputValues* m_Inputs = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
  bool m_GenerateTripleLines = false;
};

} // namespace complex
