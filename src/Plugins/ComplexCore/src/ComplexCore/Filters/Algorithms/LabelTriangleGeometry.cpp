#include "LabelTriangleGeometry.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
LabelTriangleGeometry::LabelTriangleGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, LabelTriangleGeometryInputValues* inputValues)
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
  TriangleGeom::Pointer triangle = getDataContainerArray()->getDataContainer(getCADDataContainerPath())->getGeometryAs<TriangleGeom>();

  size_t numTris = triangle->getNumberOfTris();

  DataContainer::Pointer dataContainerCAD = getDataContainerArray()->getDataContainer(getCADDataContainerPath());

  int check = triangle->findElementNeighbors();
  if(check < 0)
  {
    QString ss = "Error finding element neighbors";
    setErrorCondition(check, ss);
    return;
  }

  ElementDynamicList::Pointer m_TriangleNeighbors = triangle->getElementNeighbors();

  size_t chunkSize = 1000;
  std::vector<int32_t> triList(chunkSize, -1);
  std::vector<uint32_t> triangleCounts = {0, 0};
  // first identify connected triangle sets as features
  size_t size = 0;
  int32_t regionCount = 1;
  for(size_t i = 0; i < numTris; i++)
  {
    if(m_RegionId[i] == 0)
    {
      m_RegionId[i] = regionCount;
      triangleCounts[regionCount]++;

      size = 0;
      triList[size] = i;
      size++;
      while(size > 0)
      {
        MeshIndexType tri = triList[size - 1];
        size -= 1;
        uint16_t tCount = m_TriangleNeighbors->getNumberOfElements(tri);
        MeshIndexType* data = m_TriangleNeighbors->getElementListPointer(tri);
        for(int j = 0; j < tCount; j++)
        {
          MeshIndexType neighTri = data[j];
          if(m_RegionId[neighTri] == 0)
          {
            m_RegionId[neighTri] = regionCount;
            triangleCounts[regionCount]++;
            triList[size] = neighTri;
            size++;
            if(size >= triList.size())
            {
              size = triList.size();
              triList.resize(size + chunkSize);
              for(std::vector<int64_t>::size_type k = size; k < triList.size(); ++k)
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
  std::vector<size_t> tDims(1, triangleCounts.size());
  dataContainerCAD->getAttributeMatrix(getTriangleAttributeMatrixName())->resizeAttributeArrays(tDims);
  updateTriangleInstancePointers();

  // copy triangleCounts into the proper DataArray "NumTriangles" in the Feature Attribute Matrix
  auto& numTriangles = *m_NumTrianglesPtr.lock();
  for(size_t index = 0; index < m_NumTrianglesPtr.lock()->getSize(); index++)
  {
    numTriangles[index] = triangleCounts[index];
  }

  return {};
}
