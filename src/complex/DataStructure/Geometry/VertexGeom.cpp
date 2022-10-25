#include "VertexGeom.hpp"

#include <stdexcept>

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Utilities/GeometryHelpers.hpp"
#include "complex/Utilities/Parsing/HDF5/H5Constants.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupWriter.hpp"

using namespace complex;

VertexGeom::VertexGeom(DataStructure& ds, std::string name)
: INodeGeometry0D(ds, std::move(name))
{
}

VertexGeom::VertexGeom(DataStructure& ds, std::string name, IdType importId)
: INodeGeometry0D(ds, std::move(name), importId)
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

VertexGeom* VertexGeom::Create(DataStructure& ds, std::string name, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<VertexGeom>(new VertexGeom(ds, std::move(name)));
  if(!AttemptToAddObject(ds, data, parentId))
  {
    return nullptr;
  }
  return data.get();
}

VertexGeom* VertexGeom::Import(DataStructure& ds, std::string name, IdType importId, const std::optional<IdType>& parentId)
{
  auto data = std::shared_ptr<VertexGeom>(new VertexGeom(ds, std::move(name), importId));
  if(!AttemptToAddObject(ds, data, parentId))
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
  return "VertexGeom";
}

DataObject* VertexGeom::shallowCopy()
{
  return new VertexGeom(*this);
}

std::shared_ptr<DataObject> VertexGeom::deepCopy(const DataPath& copyPath)
{
  auto& dataStruct = getDataStructureRef();
  // Don't construct with id since it will get created when inserting into data structure
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

IGeometry::StatusCode VertexGeom::findElementSizes()
{
  // Vertices are 0-dimensional (they have no getSize),
  // so simply splat 0 over the sizes array
  auto dataStore = std::make_unique<DataStore<float32>>(std::vector<usize>{getNumberOfCells()}, std::vector<usize>{1}, 0.0f);

  Float32Array* vertexSizes = DataArray<float32>::Create(getDataStructureRef(), k_VoxelSizes, std::move(dataStore), getId());
  if(vertexSizes == nullptr)
  {
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
