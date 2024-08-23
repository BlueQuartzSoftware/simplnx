

#include "LaplacianSmoothing.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/INodeGeometry2D.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"

using namespace nx::core;

LaplacianSmoothing::LaplacianSmoothing(DataStructure& dataStructure, LaplacianSmoothingInputValues* inputValues, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

LaplacianSmoothing::~LaplacianSmoothing() noexcept = default;

Result<> LaplacianSmoothing::operator()()
{
  return edgeBasedSmoothing();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
Result<> LaplacianSmoothing::edgeBasedSmoothing()
{
  auto& surfaceMesh = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->pTriangleGeometryDataPath);

  Float32AbstractDataStore& verts = surfaceMesh.getVertices()->getDataStoreRef();

  IGeometry::MeshIndexType nvert = surfaceMesh.getNumberOfVertices();

  // Generate the Lambda Array
  std::vector<float> lambdas = generateLambdaArray();

  //  Generate the Unique Edges
  if(surfaceMesh.findEdges(false) < 0)
  {
    return MakeErrorResult(-560, "Error retrieving the shared edge list");
  }

  AbstractDataStore<IGeometry::SharedEdgeList::value_type>& edges = surfaceMesh.getEdges()->getDataStoreRef();
  IGeometry::MeshIndexType numEdges = edges.getNumberOfTuples();

  std::vector<int32> numConnections(nvert, 0);

  std::vector<double> deltaArray(nvert * 3);
  double dlta = 0.0;

  for(int32_t q = 0; q < m_InputValues->pIterationSteps; q++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Iteration {} of {}", q, m_InputValues->pIterationSteps));
    // Compute the Deltas for each point
    for(IGeometry::MeshIndexType i = 0; i < numEdges; i++)
    {
      IGeometry::MeshIndexType in1 = edges[2 * i];     // row of the first vertex
      IGeometry::MeshIndexType in2 = edges[2 * i + 1]; // row the second vertex

      for(IGeometry::MeshIndexType j = 0; j < 3; j++)
      {
#if 0
        Q_ASSERT(static_cast<size_t>(3 * in1 + j) < static_cast<size_t>(nvert * 3));
        Q_ASSERT(static_cast<size_t>(3 * in2 + j) < static_cast<size_t>(nvert * 3));
#endif
        dlta = static_cast<double>(verts[3 * in2 + j] - verts[3 * in1 + j]);
        deltaArray[3 * in1 + j] += dlta;
        deltaArray[3 * in2 + j] += -1.0 * dlta;
      }
      numConnections[in1] += 1;
      numConnections[in2] += 1;
    }

    // Move each point
    for(IGeometry::MeshIndexType i = 0; i < nvert; i++)
    {
      for(IGeometry::MeshIndexType j = 0; j < 3; j++)
      {
        IGeometry::MeshIndexType in0 = 3 * i + j;
        dlta = deltaArray[in0] / numConnections[i];

        float ll = lambdas[i];
        verts[3 * i + j] += ll * dlta;
        deltaArray[in0] = 0.0; // reset for next iteration
      }
      numConnections[i] = 0; // reset for next iteration
    }

    // Now optionally apply a negative lambda based on the mu Factor value.
    // This is from Taubin's paper on smoothing without shrinkage. This effectively
    // runs a low pass filter on the data
    if(m_InputValues->pUseTaubinSmoothing)
    {

      if(m_ShouldCancel)
      {
        return {};
      }
      m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Iteration {} of {}", q, m_InputValues->pIterationSteps));
      // Compute the Delta's
      for(IGeometry::MeshIndexType i = 0; i < numEdges; i++)
      {
        IGeometry::MeshIndexType in1 = edges[2 * i];     // row of the first vertex
        IGeometry::MeshIndexType in2 = edges[2 * i + 1]; // row the second vertex

        for(int32_t j = 0; j < 3; j++)
        {
#if 0
          Q_ASSERT(static_cast<size_t>(3 * in1 + j) < static_cast<size_t>(nvert * 3));
          Q_ASSERT(static_cast<size_t>(3 * in2 + j) < static_cast<size_t>(nvert * 3));
#endif
          dlta = verts[3 * in2 + j] - verts[3 * in1 + j];
          deltaArray[3 * in1 + j] += dlta;
          deltaArray[3 * in2 + j] += -1.0 * dlta;
        }
        numConnections[in1] += 1;
        numConnections[in2] += 1;
      }

      // MOve the points
      for(IGeometry::MeshIndexType i = 0; i < nvert; i++)
      {
        for(IGeometry::MeshIndexType j = 0; j < 3; j++)
        {
          IGeometry::MeshIndexType in0 = 3 * i + j;
          dlta = deltaArray[in0] / numConnections[i];

          float ll = lambdas[i] * m_InputValues->pMuFactor;
          verts[3 * i + j] += ll * dlta;
          deltaArray[in0] = 0.0; // reset for next iteration
        }
        numConnections[i] = 0; // reset for next iteration
      }
    }
  }

  return {};
}

// -----------------------------------------------------------------------------
std::vector<float> LaplacianSmoothing::generateLambdaArray()
{
  auto& surfaceMeshNode = m_DataStructure.getDataAs<Int8Array>(m_InputValues->pSurfaceMeshNodeTypeArrayPath)->getDataStoreRef();

  size_t numNodes = surfaceMeshNode.getNumberOfTuples();

  std::vector<float> lambdas(numNodes, 0.0f);

  for(size_t i = 0; i < numNodes; ++i)
  {
    switch(surfaceMeshNode[i])
    {
    case nx::core::NodeType::Unused:
      break;
    case nx::core::NodeType::Default:
      lambdas[i] = m_InputValues->pLambda;
      break;
    case nx::core::NodeType::TriplePoint:
      lambdas[i] = m_InputValues->pTripleLineLambda;
      break;
    case nx::core::NodeType::QuadPoint:
      lambdas[i] = m_InputValues->pQuadPointLambda;
      break;
    case nx::core::NodeType::SurfaceDefault:
      lambdas[i] = m_InputValues->pSurfacePointLambda;
      break;
    case nx::core::NodeType::SurfaceTriplePoint:
      lambdas[i] = m_InputValues->pSurfaceTripleLineLambda;
      break;
    case nx::core::NodeType::SurfaceQuadPoint:
      lambdas[i] = m_InputValues->pSurfaceQuadPointLambda;
      break;
    default:
      break;
    }
  }

  return std::move(lambdas);
}

// -----------------------------------------------------------------------------
// This is just here for some debugging issues.
// -----------------------------------------------------------------------------
#if 0
void LaplacianSmoothing::writeVTKFile(const QString& outputVtkFile)
{

  DataContainer::Pointer sm = getDataContainerArray()->getDataContainer(getSurfaceDataContainerName());  /* Place all your code to execute your filter here. */
  VertexArray& nodes = *(sm->getVertices());
  int nNodes = nodes.getNumberOfTuples();
  bool m_WriteBinaryFile = true;

  FILE* vtkFile = nullptr;
  vtkFile = fopen(outputVtkFile.toLatin1().data(), "wb");
  if (nullptr == vtkFile)
  {
    QString ss = QObject::tr("Error creating file '%1'").arg(outputVtkFile);
    setErrorCondition(-90123, ss);
    return;
  }
  Detail::ScopedFileMonitor vtkFileMonitor(vtkFile);

  fprintf(vtkFile, "# vtk DataFile Version 2.0\n");
  fprintf(vtkFile, "Data set from DREAM.3D Surface Meshing Module\n");
  if (m_WriteBinaryFile)
  {
    fprintf(vtkFile, "BINARY\n");
  }
  else
  {
    fprintf(vtkFile, "ASCII\n");
  }
  fprintf(vtkFile, "DATASET POLYDATA\n");


  fprintf(vtkFile, "POINTS %d float\n", nNodes);
  float pos[3] = {0.0f, 0.0f, 0.0f};

  size_t totalWritten = 0;
  // Write the POINTS data (Vertex)
  for (int i = 0; i < nNodes; i++)
  {
    VertexArray::Vert_t& n = nodes[i]; // Get the current Node
    //  if (m_SurfaceMeshNodeType[i] > 0)
    {
      pos[0] = static_cast<float>(n.pos[0]);
      pos[1] = static_cast<float>(n.pos[1]);
      pos[2] = static_cast<float>(n.pos[2]);
      if (m_WriteBinaryFile == true)
      {
        SIMPLib::Endian::FromSystemToBig::convert<float>(pos[0]);
        SIMPLib::Endian::FromSystemToBig::convert<float>(pos[1]);
        SIMPLib::Endian::FromSystemToBig::convert<float>(pos[2]);
        totalWritten = fwrite(pos, sizeof(float), 3, vtkFile);
        if (totalWritten != sizeof(float) * 3)
        {

        }
      }
      else
      {
        fprintf(vtkFile, "%4.4f %4.4f %4.4f\n", pos[0], pos[1], pos[2]); // Write the positions to the output file
      }
    }
  }

  // Write the triangle indices into the vtk File
  FaceArray& triangles = *(sm->getFaces());
  int triangleCount = 0;
  int end = triangles.getNumberOfTuples();
  int featureInterest = 9;
  for(int i = 0; i < end; ++i)
  {
    //FaceArray::Face_t* tri = triangles.getPointer(i);
    if (m_SurfaceMeshFaceLabels[i * 2] == featureInterest || m_SurfaceMeshFaceLabels[i * 2 + 1] == featureInterest)
    {
      ++triangleCount;
    }
  }


  int tData[4];
  // Write the CELLS Data
  //  int start = 3094380;
  //  int end = 3094450;
  //  int triangleCount = end - start;
  qDebug() << "---------------------------------------------------------------------------" << "\n";
  qDebug() << outputVtkFile << "\n";
  fprintf(vtkFile, "\nPOLYGONS %d %d\n", triangleCount, (triangleCount * 4));
  for (int tid = 0; tid < end; ++tid)
  {
    //FaceArray::Face_t* tri = triangles.getPointer(tid);
    if (m_SurfaceMeshFaceLabels[tid * 2] == featureInterest || m_SurfaceMeshFaceLabels[tid * 2 + 1] == featureInterest)
    {
      tData[1] = triangles[tid].verts[0];
      tData[2] = triangles[tid].verts[1];
      tData[3] = triangles[tid].verts[2];
      //      qDebug() << tid << "\n  " << nodes[tData[1]].coord[0] << " " << nodes[tData[1]].coord[1]  << " " << nodes[tData[1]].coord[2]  << "\n";
      //      qDebug() << "  " << nodes[tData[2]].coord[0] << " " << nodes[tData[2]].coord[1]  << " " << nodes[tData[2]].coord[2]  << "\n";
      //      qDebug() << "  " << nodes[tData[3]].coord[0] << " " << nodes[tData[3]].coord[1]  << " " << nodes[tData[3]].coord[2]  << "\n";
      if (m_WriteBinaryFile == true)
      {
        tData[0] = 3; // Push on the total number of entries for this entry
        SIMPLib::Endian::FromSystemToBig::convert<int>(tData[0]);
        SIMPLib::Endian::FromSystemToBig::convert<int>(tData[1]); // Index of Vertex 0
        SIMPLib::Endian::FromSystemToBig::convert<int>(tData[2]); // Index of Vertex 1
        SIMPLib::Endian::FromSystemToBig::convert<int>(tData[3]); // Index of Vertex 2
        fwrite(tData, sizeof(int), 4, vtkFile);
      }
      else
      {
        fprintf(vtkFile, "3 %d %d %d\n", tData[1], tData[2], tData[3]);
      }
    }
  }

  fprintf(vtkFile, "\n");
}
#endif
