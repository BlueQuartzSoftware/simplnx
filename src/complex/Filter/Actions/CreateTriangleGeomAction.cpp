#include "CreateTriangleGeomAction.hpp"

#include <fmt/core.h>

#include "complex/DataStructure/EmptyDataStore.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"

using namespace complex;

namespace complex
{
CreateTriangleGeomAction::CreateTriangleGeomAction(const DataPath& path, AdditionalData additional)
: m_Path(path)
, m_AdditionalData(additional)
{
}

CreateTriangleGeomAction::CreateTriangleGeomAction(const DataPath& path, const ShapeType& tupleSize, AdditionalData additional)
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

  if(shouldCreateVertexArray())
  {
    auto vertexResult = createVertexArray(dataStructure, mode);
    if(vertexResult.invalid())
    {
      return vertexResult;
    }
  }
  if(shouldCreateTriangleArray())
  {
    auto triangleResult = createTriangleArray(dataStructure, mode);
    if(triangleResult.invalid())
    {
      return triangleResult;
    }
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
  auto additionalInt8 = static_cast<int8>(m_AdditionalData);
  auto targetInt8 = static_cast<int8>(AdditionalData::Vertices);
  return (additionalInt8 & targetInt8) != 0;
}

bool CreateTriangleGeomAction::shouldCreateTriangleArray() const
{
  auto additionalInt8 = static_cast<int8>(m_AdditionalData);
  auto targetInt8 = static_cast<int8>(AdditionalData::Triangles);
  return (additionalInt8 & targetInt8) != 0;
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
  DataPath arrayPath = path().createChildPath("Vertex Array");
  IDataStore::ShapeType compShape{3};
  return CreateArray<float32>(dataStructure, m_TupleSize, compShape, arrayPath, mode);
}

Result<> CreateTriangleGeomAction::createTriangleArray(DataStructure& dataStructure, Mode mode) const
{
  DataPath arrayPath = path().createChildPath("Triangle Array");
  IDataStore::ShapeType compShape{3};
  return CreateArray<usize>(dataStructure, m_TupleSize, compShape, arrayPath, mode);
}

} // namespace complex
