#include "LabelTriangleGeometry.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"

#include <fmt/format.h>

#include <array>
#include <optional>
#include <stdexcept>

using namespace nx::core;

namespace
{
// Index 0 is neighbors, index 1 is incidence: remove dependent temporary data first.
void CleanupCreatedSupport(DataStructure& dataStructure, TriangleGeom& geometry, const DataPath& geometryPath, const std::array<std::optional<DataObject::IdType>, 2>& entryIds,
                           const std::array<bool, 2>& childExisted)
{
  const std::array supportPaths = {geometryPath.createChildPath(TriangleGeom::k_EltNeighbors), geometryPath.createChildPath(TriangleGeom::k_EltsContainingVert)};
  for(usize index = 0; index < supportPaths.size(); ++index)
  {
    if(!childExisted[index])
    {
      // Look up the exact new ID even when a failed producer cleared its association.
      const auto* child = dataStructure.getData(supportPaths[index]);
      if(child != nullptr)
      {
        const auto createdId = child->getId();
        if(!dataStructure.removeData(createdId))
        {
          throw std::runtime_error(fmt::format("Cannot remove temporary support object {} from geometry '{}'.", createdId, geometryPath.toString()));
        }
      }
    }
    if(index == 0)
    {
      geometry.setElementNeighborsId(entryIds[index]);
    }
    else
    {
      geometry.setElementContainingVertId(entryIds[index]);
    }
  }
}
} // namespace

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
  const std::array entryIds = {triangle.getElementNeighborsId(), triangle.getElementContainingVertId()};
  const std::array childExisted = {m_DataStructure.getData(m_InputValues->TriangleGeomPath.createChildPath(TriangleGeom::k_EltNeighbors)) != nullptr,
                                   m_DataStructure.getData(m_InputValues->TriangleGeomPath.createChildPath(TriangleGeom::k_EltsContainingVert)) != nullptr};

  {
    usize numTris = triangle.getNumberOfFaces();

    Result<> result = triangle.findElementNeighbors(false); // use auto since return time is a class declared typename
    if(result.invalid())
    {
      CleanupCreatedSupport(m_DataStructure, triangle, m_InputValues->TriangleGeomPath, entryIds, childExisted);
      return result;
    }

    const TriangleGeom::ElementDynamicList* triangleNeighborsPtr = triangle.getElementNeighbors();

    auto& regionIdsStore = m_DataStructure.getDataAs<Int32Array>(m_InputValues->RegionIdsPath)->getDataStoreRef();

    usize chunkSize = 1000;
    std::vector<int32> triList(chunkSize, -1);
    // first identify connected triangle sets as features
    int32 regionCount = 1;
    for(usize i = 0; i < numTris; i++)
    {
      if(regionIdsStore[i] == 0)
      {
        regionIdsStore[i] = regionCount;
        triangleCounts[regionCount]++;

        usize size = 0;
        triList[size] = static_cast<int32>(i);
        size++;
        while(size > 0)
        {
          TriangleGeom::MeshIndexType tri = triList[size - 1];
          size -= 1;
          uint16_t tCount = triangleNeighborsPtr->getNumberOfElements(tri);
          TriangleGeom::MeshIndexType* dataPtr = triangleNeighborsPtr->getElementListPointer(tri);
          for(int j = 0; j < tCount; j++)
          {
            TriangleGeom::MeshIndexType neighTri = dataPtr[j];
            if(regionIdsStore[neighTri] == 0)
            {
              regionIdsStore[neighTri] = regionCount;
              triangleCounts[regionCount]++;
              triList[size] = static_cast<int32>(neighTri);
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
    Result<> resizeResult = m_DataStructure.getDataAs<AttributeMatrix>(m_InputValues->TriangleAMPath)->resizeTuples(std::vector<usize>{triangleCounts.size()});
    if(resizeResult.invalid())
    {
      CleanupCreatedSupport(m_DataStructure, triangle, m_InputValues->TriangleGeomPath, entryIds, childExisted);
      return resizeResult;
    }
  }

  CleanupCreatedSupport(m_DataStructure, triangle, m_InputValues->TriangleGeomPath, entryIds, childExisted);

  // copy triangleCounts into the proper DataArray "NumTriangles" in the Feature Attribute Matrix
  auto& numTrianglesStore = m_DataStructure.getDataAs<UInt64Array>(m_InputValues->NumTrianglesPath)->getDataStoreRef();
  for(usize index = 0; index < numTrianglesStore.getSize(); index++)
  {
    numTrianglesStore[index] = static_cast<uint64>(triangleCounts[index]);
  }

  return {};
}
