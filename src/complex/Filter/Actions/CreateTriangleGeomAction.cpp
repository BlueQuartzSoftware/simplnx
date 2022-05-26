#include "CreateTriangleGeomAction.hpp"

#include <fmt/core.h>

#include "complex/DataStructure/EmptyDataStore.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"

using namespace complex;

namespace complex
{
CreateTriangleGeomAction::CreateTriangleGeomAction(const DataPath& path, AdditionalDataTypes additional)
: m_Path(path)
, m_AdditionalData(additional)
{
}

CreateTriangleGeomAction::CreateTriangleGeomAction(const DataPath& path, const ShapeType& tupleSize, AdditionalDataTypes additional)
: m_Path(path)
, m_TupleSize(tupleSize)
, m_AdditionalData(additional)
{
}

CreateTriangleGeomAction::~CreateTriangleGeomAction() noexcept = default;

Result<> CreateTriangleGeomAction::apply(DataStructure& dataStructure, Mode mode) const
{
  auto geomResult = createGeometry(dataStructure, mode);
  if(geomResult.invalid())
  {
    return geomResult;
  }

  TriangleGeom& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(m_Path);

  if(m_AdditionalData.find(AdditionalData::CreateVertexGroup) != m_AdditionalData.end())
  {
    dataStructure.makePath(m_Path.createChildPath(k_DefaultVertexGroup));
  }
  if(m_AdditionalData.find(AdditionalData::CreateTriangleGroup) != m_AdditionalData.end())
  {
    dataStructure.makePath(m_Path.createChildPath(k_DefaultFacesGroup));
  }

  if(shouldCreateVertexArray())
  {
    auto vertexResult = createVertexArray(dataStructure, mode);
    if(vertexResult.invalid())
    {
      return vertexResult;
    }
    DataPath vertPath = path().createChildPath(k_DefaultVerticesName);
    Float32Array& verArray = dataStructure.getDataRefAs<Float32Array>(vertPath);
    triangleGeom.setVertices(verArray);
  }
  if(shouldCreateTriangleArray())
  {
    auto triangleResult = createTriangleArray(dataStructure, mode);
    if(triangleResult.invalid())
    {
      return triangleResult;
    }
    DataPath facePath = path().createChildPath(k_DefaultFacesName);
    UInt64Array& faceArray = dataStructure.getDataRefAs<UInt64Array>(facePath);
    triangleGeom.setFaces(faceArray);
  }

  return {};
}

const DataPath& CreateTriangleGeomAction::path() const
{
  return m_Path;
}

CreateTriangleGeomAction::ShapeType CreateTriangleGeomAction::tupleSize() const
{
  return m_TupleSize;
}

bool CreateTriangleGeomAction::shouldCreateVertexArray() const
{

  return (m_AdditionalData.find(AdditionalData::Vertices) != m_AdditionalData.end() || m_AdditionalData.find(AdditionalData::VerticesTriangles) != m_AdditionalData.end());
}

bool CreateTriangleGeomAction::shouldCreateTriangleArray() const
{
  return (m_AdditionalData.find(AdditionalData::Triangles) != m_AdditionalData.end() || m_AdditionalData.find(AdditionalData::VerticesTriangles) != m_AdditionalData.end());
}

Result<> CreateTriangleGeomAction::createGeometry(DataStructure& dataStructure, Mode mode) const
{
  auto parentPath = path().getParent();

  std::optional<DataObject::IdType> id;

  if(parentPath.getLength() != 0)
  {
    auto* parentObject = dataStructure.getData(parentPath);
    if(parentObject == nullptr)
    {
      return MakeErrorResult(-262, fmt::format("Parent object '{}' does not exist", parentPath.toString()));
    }

    id = parentObject->getId();
  }
  auto* geom = TriangleGeom::Create(dataStructure, path().getTargetName(), id);

  if(geom == nullptr)
  {
    return MakeErrorResult(-263, fmt::format("Failed to create the specified geometry", parentPath.toString()));
  }

  return {};
}

Result<> CreateTriangleGeomAction::createVertexArray(DataStructure& dataStructure, Mode mode) const
{
  DataPath arrayPath = path().createChildPath(k_DefaultVerticesName);
  IDataStore::ShapeType compShape{3};
  return CreateArray<float32>(dataStructure, m_TupleSize, compShape, arrayPath, mode);
}

Result<> CreateTriangleGeomAction::createTriangleArray(DataStructure& dataStructure, Mode mode) const
{
  DataPath arrayPath = path().createChildPath(k_DefaultFacesName);
  IDataStore::ShapeType compShape{3};
  return CreateArray<uint64>(dataStructure, m_TupleSize, compShape, arrayPath, mode);
}

} // namespace complex
