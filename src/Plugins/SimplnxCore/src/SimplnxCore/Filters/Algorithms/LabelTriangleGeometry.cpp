#include "LabelTriangleGeometry.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
LabelTriangleGeometry::LabelTriangleGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                             LabelTriangleGeometryInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
LabelTriangleGeometry::~LabelTriangleGeometry() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& LabelTriangleGeometry::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> LabelTriangleGeometry::operator()()
{
  // Scope other values since underlying arrays will be changed by the time they are need again
  std::vector<uint32> triangleCounts = {0, 0};
  auto& triangle = m_DataStructure.getDataRefAs<TriangleGeom>(m_InputValues->TriangleGeomPath);

  {
    usize numTris = triangle.getNumberOfFaces();

    auto check = triangle.findElementNeighbors(); // use auto since return time is a class declared typename
    if(check < 0)
    {
      return MakeErrorResult(check, fmt::format("Error finding element neighbors for {} geometry", triangle.getName()));
    }

    const TriangleGeom::ElementDynamicList* triangleNeighbors = triangle.getElementNeighbors();

    auto& regionIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->RegionIdsPath);

    usize chunkSize = 1000;
    std::vector<int32> triList(chunkSize, -1);
    // first identify connected triangle sets as features
    int32 regionCount = 1;
    for(usize i = 0; i < numTris; i++)
    {
      if(regionIds[i] == 0)
      {
        regionIds[i] = regionCount;
        triangleCounts[regionCount]++;

        usize size = 0;
        triList[size] = i;
        size++;
        while(size > 0)
        {
          TriangleGeom::MeshIndexType tri = triList[size - 1];
          size -= 1;
          uint16_t tCount = triangleNeighbors->getNumberOfElements(tri);
          TriangleGeom::MeshIndexType* data = triangleNeighbors->getElementListPointer(tri);
          for(int j = 0; j < tCount; j++)
          {
            TriangleGeom::MeshIndexType neighTri = data[j];
            if(regionIds[neighTri] == 0)
            {
              regionIds[neighTri] = regionCount;
              triangleCounts[regionCount]++;
              triList[size] = neighTri;
              size++;
              if(size >= triList.size())
              {
                size = triList.size();
                triList.resize(size + chunkSize);
                for(usize k = size; k < triList.size(); ++k)
                {
                  triList[k] = -1;
                }
              }
            }
          }
        }
        regionCount++;
        triangleCounts.push_back(0);
      }
    }

    // Resize the Triangle Region AttributeMatrix
    m_DataStructure.getDataAs<AttributeMatrix>(m_InputValues->TriangleAMPath)->resizeTuples(std::vector<usize>{triangleCounts.size()});
  }

  // Clear ElementDynamicLists so write out is possible
  triangle.deleteElementNeighbors();
  // Remove elements containing vertices, because Element neighbors created it quietly under the covers
  triangle.deleteElementsContainingVert();

  // copy triangleCounts into the proper DataArray "NumTriangles" in the Feature Attribute Matrix
  auto& numTriangles = m_DataStructure.getDataRefAs<UInt64Array>(m_InputValues->NumTrianglesPath);
  for(usize index = 0; index < numTriangles.getSize(); index++)
  {
    numTriangles[index] = static_cast<uint64>(triangleCounts[index]);
  }

  return {};
}
