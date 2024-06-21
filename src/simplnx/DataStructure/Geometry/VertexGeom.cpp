#include "VertexGeom.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Utilities/GeometryHelpers.hpp"

#include <stdexcept>

using namespace nx::core;

VertexGeom::VertexGeom(DataStructure& dataStructure, std::string name)
: INodeGeometry0D(dataStructure, std::move(name))
{
}

VertexGeom::VertexGeom(DataStructure& dataStructure, std::string name, IdType importId)
: INodeGeometry0D(dataStructure, std::move(name), importId)
{
}

VertexGeom::VertexGeom(const VertexGeom&) = default;

VertexGeom::VertexGeom(VertexGeom&&) = default;

VertexGeom::~VertexGeom() noexcept = default;

DataObject::Type VertexGeom::getDataObjectType() const
{
  return DataObject::Type::VertexGeom;
}

BaseGroup::GroupType VertexGeom::getGroupType() const
{
  return GroupType::VertexGeom;
}

VertexGeom* VertexGeom::Create(DataStructure& dataStructure, std::string name, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<VertexGeom>(new VertexGeom(dataStructure, std::move(name)));
  if(!AttemptToAddObject(dataStructure, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

VertexGeom* VertexGeom::Import(DataStructure& dataStructure, std::string name, IdType importId, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<VertexGeom>(new VertexGeom(dataStructure, std::move(name), importId));
  if(!AttemptToAddObject(dataStructure, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

IGeometry::Type VertexGeom::getGeomType() const
{
  return IGeometry::Type::Vertex;
}

std::string VertexGeom::getTypeName() const
{
  return k_TypeName;
}

DataObject* VertexGeom::shallowCopy()
{
  return new VertexGeom(*this);
}

std::shared_ptr<DataObject> VertexGeom::deepCopy(const DataPath& copyPath)
{
  auto& dataStruct = getDataStructureRef();
  // Don't construct with identifier since it will get created when inserting into data structure
  auto copy = std::shared_ptr<VertexGeom>(new VertexGeom(dataStruct, copyPath.getTargetName()));
  if(!dataStruct.containsData(copyPath) && dataStruct.insert(copy, copyPath.getParent()))
  {
    auto dataMapCopy = getDataMap().deepCopy(copyPath);

    if(m_VertexAttributeMatrixId.has_value())
    {
      const DataPath copiedDataPath = copyPath.createChildPath(getVertexAttributeMatrix()->getName());
      // if this is not a parent of the cell data object, make a deep copy and insert it here
      if(!isParentOf(getVertexAttributeMatrix()))
      {
        const auto dataObjCopy = getVertexAttributeMatrix()->deepCopy(copiedDataPath);
      }
      copy->m_VertexAttributeMatrixId = dataStruct.getId(copiedDataPath);
    }

    if(m_VertexDataArrayId.has_value())
    {
      const DataPath copiedDataPath = copyPath.createChildPath(getVertices()->getName());
      // if this is not a parent of the data object, make a deep copy and insert it here
      if(!isParentOf(getVertices()))
      {
        const auto dataObjCopy = getVertices()->deepCopy(copiedDataPath);
      }
      copy->m_VertexDataArrayId = dataStruct.getId(copiedDataPath);
    }

    if(const auto voxelSizesCopy = dataStruct.getDataAs<Float32Array>(copyPath.createChildPath(k_VoxelSizes)); voxelSizesCopy != nullptr)
    {
      copy->m_ElementSizesId = voxelSizesCopy->getId();
    }
    return copy;
  }
  return nullptr;
}

IGeometry::StatusCode VertexGeom::findElementSizes(bool recalculate)
{
  auto* vertexSizes = getDataStructureRef().getDataAs<Float32Array>(m_ElementSizesId);
  if(vertexSizes != nullptr && !recalculate)
  {
    return 0;
  }
  // Vertices are 0-dimensional (they have no getSize),
  // so simply splat 0 over the sizes array
  if(vertexSizes == nullptr)
  {
    auto dataStore = std::make_unique<DataStore<float32>>(std::vector<usize>{getNumberOfCells()}, std::vector<usize>{1}, 0.0f);
    vertexSizes = DataArray<float32>::Create(getDataStructureRef(), k_VoxelSizes, std::move(dataStore), getId());
  }
  if(vertexSizes == nullptr)
  {
    m_ElementSizesId.reset();
    return -1;
  }
  m_ElementSizesId = vertexSizes->getId();
  return 1;
}

Point3D<float64> VertexGeom::getParametricCenter() const
{
  return {0.0, 0.0, 0.0};
}

void VertexGeom::getShapeFunctions([[maybe_unused]] const Point3D<float64>& pCoords, float64* shape) const
{
  shape[0] = 0.0;
  shape[1] = 0.0;
  shape[2] = 0.0;
}
