#include "CreateTriangleGeometryAction.hpp"

#include <fmt/core.h>

#include <utility>

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"

using namespace complex;

namespace
{
template <class T>
DataArray<T>* ArrayFromPath(DataStructure& dataGraph, const DataPath& path)
{
  DataObject* object = dataGraph.getData(path);
  DataArray<T>* dataArray = dynamic_cast<DataArray<T>*>(object);
  if(dataArray == nullptr)
  {
    throw std::runtime_error("Can't obtain DataArray");
  }
  return dataArray;
}
} // namespace

namespace complex
{
CreateTriangleGeometryAction::CreateTriangleGeometryAction(DataPath geometryPath, AbstractGeometry::MeshIndexType numFaces, AbstractGeometry::MeshIndexType numVertices)
: m_GeometryPath(std::move(geometryPath))
, m_NumFaces(numFaces)
, m_NumVertices(numVertices)
{
}

CreateTriangleGeometryAction::~CreateTriangleGeometryAction() noexcept = default;

Result<> CreateTriangleGeometryAction::apply(DataStructure& dataStructure, Mode mode) const
{
  const std::string k_TriangleDataName("SharedTriList");
  const std::string k_VertexDataName("SharedVertexList");

  if(m_GeometryPath.empty())
  {
    return MakeErrorResult(-220, "CreateTriangleGeometryAction: Geometry Path cannot be empty");
  }

  usize size = m_GeometryPath.getLength();
  std::string name = (size == 1 ? m_GeometryPath[0] : m_GeometryPath[size - 1]);

  std::optional<DataObject::IdType> parentId;
  if(size > 1)
  {
    DataPath parentPath = m_GeometryPath.getParent();
    std::cout << "Triangle Geometry Parent Path: " << parentPath.toString() << std::endl;
    parentId = dataStructure.getId(parentPath);
    if(!parentId.has_value())
    {
      return MakeErrorResult(-221, "CreateTriangleGeometryAction: Invalid path");
    }
  }

  BaseGroup* parentObject = dataStructure.getDataAs<BaseGroup>(parentId);
  if(parentObject->contains(name))
  {
    return MakeErrorResult(-222, fmt::format("CreateTriangleGeometryAction: DataObject already exists at path '{}'", m_GeometryPath.toString()));
  }

  // Create the TriangleGeometry
  auto triangleGeom = TriangleGeom::Create(dataStructure, name, parentId);

  using MeshIndexType = AbstractGeometry::MeshIndexType;
  using SharedTriList = AbstractGeometry::SharedTriList;

  DataPath trianglesPath = m_GeometryPath.createChildPath(k_TriangleDataName);
  // Create the default DataArray that will hold the FaceList and Vertices. We
  // size these to 1 because the Csv parser will resize them to the appropriate number of tuples
  std::vector<size_t> tupleShape = {m_NumFaces};
  complex::Result result = complex::CreateArray<MeshIndexType>(dataStructure, tupleShape, 3, trianglesPath, mode);
  if(result.invalid())
  {
    return MakeErrorResult(-223, fmt::format("CreateTriangleGeometryAction: Could not allocate SharedTriList '{}'", trianglesPath.toString()));
  }
  SharedTriList* triangles = ::ArrayFromPath<MeshIndexType>(dataStructure, trianglesPath);
  triangleGeom->setTriangles(triangles);

  // Create the Vertex Array with a component size of 3
  DataPath vertexPath = m_GeometryPath.createChildPath(k_VertexDataName);
  tupleShape = {m_NumVertices}; // We don't probably know how many Vertices there are but take what ever the developer sends us

  result = complex::CreateArray<float>(dataStructure, tupleShape, 3, vertexPath, mode);
  if(result.invalid())
  {
    return MakeErrorResult(-224, fmt::format("CreateTriangleGeometryAction: Could not allocate SharedVertList '{}'", trianglesPath.toString()));
  }
  Float32Array* vertexArray = ::ArrayFromPath<float>(dataStructure, vertexPath);
  triangleGeom->setVertices(vertexArray);

  return {};
}

const DataPath& CreateTriangleGeometryAction::geometryPath() const
{
  return m_GeometryPath;
}
AbstractGeometry::MeshIndexType CreateTriangleGeometryAction::numFaces() const
{
  return m_NumFaces;
}
AbstractGeometry::MeshIndexType CreateTriangleGeometryAction::numVertices() const
{
  return m_NumVertices;
}

} // namespace complex
