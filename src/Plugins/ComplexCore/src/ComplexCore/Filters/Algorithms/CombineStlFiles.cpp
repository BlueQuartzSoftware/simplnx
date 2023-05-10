#include "CombineStlFiles.hpp"

#include "ComplexCore/Filters/StlFileReaderFilter.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"
#include "complex/Utilities/StringUtilities.hpp"

#include <filesystem>
namespace fs = std::filesystem;

using namespace complex;

// -----------------------------------------------------------------------------
CombineStlFiles::CombineStlFiles(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CombineStlFilesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
CombineStlFiles::~CombineStlFiles() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& CombineStlFiles::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> CombineStlFiles::operator()()
{
  DataStructure tempDataStructure;
  for(const auto& dirEntry : std::filesystem::directory_iterator{m_InputValues->StlFilesPath})
  {
    const fs::path& stlFilePath = dirEntry.path();
    if(fs::is_regular_file(stlFilePath) && StringUtilities::toLower(stlFilePath.extension().string()) == ".stl")
    {
      StlFileReaderFilter stlFileReader;
      Arguments args;
      args.insertOrAssign(StlFileReaderFilter::k_StlFilePath_Key, std::make_any<FileSystemPathParameter::ValueType>(stlFilePath));
      args.insertOrAssign(StlFileReaderFilter::k_GeometryDataPath_Key, std::make_any<DataPath>(DataPath({stlFilePath.stem().string()})));
      auto executeResult = stlFileReader.execute(tempDataStructure, args);
      if(executeResult.result.invalid())
      {
        return executeResult.result;
      }
    }
  }

  usize totalTriangles = 0;
  usize totalVertices = 0;
  std::vector<TriangleGeom*> stlGeometries;
  for(auto* stlGeom : tempDataStructure.getTopLevelData())
  {
    if(auto* triangleGeom = dynamic_cast<TriangleGeom*>(stlGeom); triangleGeom != nullptr)
    {
      totalTriangles += triangleGeom->getNumberOfFaces();
      totalVertices += triangleGeom->getNumberOfVertices();
      stlGeometries.push_back(triangleGeom);
    }
  }

  auto& combinedGeom = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TriangleDataContainerName);
  auto* combinedFaceAM = combinedGeom.getFaceAttributeMatrix();
  auto& combinedFaceNormals = m_DataStructure.getDataRefAs<Float64Array>(m_InputValues->FaceNormalsArrayName);
  combinedGeom.resizeFaceList(totalTriangles);
  combinedGeom.resizeVertexList(totalVertices);
  combinedFaceAM->resizeTuples(std::vector<usize>{totalTriangles});

  usize triOffset = 0;
  usize vertexOffset = 0;
  usize triCounter = 0;
  usize faceNormalsOffset = 0;
  INodeGeometry2D::SharedFaceList& triangles = combinedGeom.getFacesRef();
  INodeGeometry0D::SharedVertexList& vertices = combinedGeom.getVerticesRef();
  for(auto* geom : stlGeometries)
  {
    INodeGeometry2D::SharedFaceList& curTriangles = geom->getFacesRef();
    for(usize t = 0; t < geom->getNumberOfFaces(); t++)
    {
      curTriangles[3 * t + 0] += triCounter;
      curTriangles[3 * t + 1] += triCounter;
      curTriangles[3 * t + 2] += triCounter;
    }
    triCounter += geom->getNumberOfVertices();
    INodeGeometry0D::SharedVertexList& curVertices = geom->getVerticesRef();

    std::copy(curTriangles.begin(), curTriangles.end(), triangles.begin() + triOffset);
    std::copy(curVertices.begin(), curVertices.end(), vertices.begin() + vertexOffset);

    triOffset += geom->getNumberOfFaces() * 3;
    vertexOffset += geom->getNumberOfVertices() * 3;

    auto& curFaceNormals = tempDataStructure.getDataRefAs<Float64Array>(geom->getFaceAttributeMatrixDataPath().createChildPath(StlFileReaderFilter::k_FaceNormals));
    std::copy(curFaceNormals.begin(), curFaceNormals.end(), combinedFaceNormals.begin() + faceNormalsOffset);
    faceNormalsOffset += curFaceNormals.getSize();
  }

  return {};
}
