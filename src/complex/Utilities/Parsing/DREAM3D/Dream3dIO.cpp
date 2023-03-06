#include "Dream3dIO.hpp"

#include "complex/DataStructure/AttributeMatrix.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/EmptyDataStore.hpp"
#include "complex/DataStructure/Geometry/EdgeGeom.hpp"
#include "complex/DataStructure/Geometry/HexahedralGeom.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/Geometry/QuadGeom.hpp"
#include "complex/DataStructure/Geometry/RectGridGeom.hpp"
#include "complex/DataStructure/Geometry/TetrahedralGeom.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/DataStructure/Geometry/VertexGeom.hpp"
#include "complex/DataStructure/IO/HDF5/DataStructureReader.hpp"
#include "complex/DataStructure/IO/HDF5/DataStructureWriter.hpp"
#include "complex/DataStructure/IO/HDF5/NeighborListIO.hpp"
#include "complex/DataStructure/NeighborList.hpp"
#include "complex/DataStructure/StringArray.hpp"
#include "complex/Pipeline/Pipeline.hpp"
#include "complex/Utilities/Parsing/HDF5/Readers/FileReader.hpp"
#include "complex/Utilities/Parsing/HDF5/Writers/AttributeWriter.hpp"
#include "complex/Utilities/Parsing/HDF5/Writers/FileWriter.hpp"

#include <nlohmann/json.hpp>

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>

using namespace complex;

namespace
{
constexpr StringLiteral k_DataStructureGroupTag = "DataStructure";
constexpr StringLiteral k_LegacyDataStructureGroupTag = "DataContainers";
constexpr StringLiteral k_FileVersionTag = "FileVersion";
constexpr StringLiteral k_PipelineJsonTag = "Pipeline";
constexpr StringLiteral k_PipelineNameTag = "Current Pipeline";
constexpr StringLiteral k_PipelineVersionTag = "Pipeline Version";
constexpr StringLiteral k_CurrentFileVersion = "8.0";

constexpr int32_t k_CurrentPipelineVersion = 3;

namespace Legacy
{
constexpr StringLiteral DCATag = "DataContainers";
constexpr StringLiteral GeometryTag = "_SIMPL_GEOMETRY";
constexpr StringLiteral GeometryNameTag = "GeometryName";
constexpr StringLiteral GeometryTypeNameTag = "GeometryTypeName";
constexpr StringLiteral PipelineName = "Pipeline";
constexpr StringLiteral CompDims = "ComponentDimensions";
constexpr StringLiteral TupleDims = "TupleDimensions";

constexpr StringLiteral FileVersion = "7.0";

constexpr StringLiteral VertexListName = "SharedVertexList";
constexpr StringLiteral EdgeListName = "SharedEdgeList";
constexpr StringLiteral TriListName = "SharedTriList";
constexpr StringLiteral QuadListName = "SharedQuadList";
constexpr StringLiteral TetraListName = "SharedTetList";
constexpr StringLiteral HexListName = "SharedHexList";
constexpr StringLiteral VerticesName = "Verts";
constexpr StringLiteral XBoundsName = "xBounds";
constexpr StringLiteral YBoundsName = "yBounds";
constexpr StringLiteral ZBoundsName = "zBounds";

namespace Type
{
constexpr StringLiteral ImageGeom = "ImageGeometry";
constexpr StringLiteral EdgeGeom = "EdgeGeometry";
constexpr StringLiteral HexGeom = "HexahedralGeometry";
constexpr StringLiteral QuadGeom = "QuadrilateralGeometry";
constexpr StringLiteral RectGridGeom = "RectGridGeometry";
constexpr StringLiteral TetrahedralGeom = "TetrahedralGeometry";
constexpr StringLiteral TriangleGeom = "TriangleGeometry";
constexpr StringLiteral VertexGeom = "VertexGeometry";
} // namespace Type
} // namespace Legacy

std::pair<std::string, usize> GetXdmfTypeAndSize(DataType type)
{
  switch(type)
  {
  case DataType::int8: {
    return {"Char", 1};
  }
  case DataType::int16: {
    return {"Int", 2};
  }
  case DataType::int32: {
    return {"Int", 4};
  }
  case DataType::int64: {
    return {"Int", 8};
  }
  case DataType::uint8: {
    return {"UChar", 1};
  }
  case DataType::uint16: {
    return {"UInt", 2};
  }
  case DataType::uint32: {
    return {"UInt", 4};
  }
  case DataType::uint64: {
    return {"UInt", 8};
  }
  case DataType::float32: {
    return {"Float", 4};
  }
  case DataType::float64: {
    return {"Float", 8};
  }
  case DataType::boolean: {
    return {"UChar", 1};
  }
  }
  throw std::runtime_error("GetXdmfTypeAndSize: invalid DataType");
}

void WriteGeomXdmf(std::ostream& out, const ImageGeom& imageGeom, std::string_view hdf5FilePath)
{
  std::string name = imageGeom.getName();

  SizeVec3 dims = imageGeom.getDimensions();
  FloatVec3 spacing = imageGeom.getSpacing();
  FloatVec3 origin = imageGeom.getOrigin();

  std::array<int64, 3> volDims = {static_cast<int64>(dims.getX()), static_cast<int64>(dims.getY()), static_cast<int64>(dims.getZ())};

  out << "  <!-- *************** START OF " << name << " *************** -->"
      << "\n";
  out << "  <Grid Name=\"" << name << R"(" GridType="Uniform">)"
      << "\n";
  out << "    <Topology TopologyType=\"3DCoRectMesh\" Dimensions=\"" << volDims[2] + 1 << " " << volDims[1] + 1 << " " << volDims[0] + 1 << " \"></Topology>"
      << "\n";
  out << "    <Geometry Type=\"ORIGIN_DXDYDZ\">"
      << "\n";
  out << "      <!-- Origin  Z, Y, X -->"
      << "\n";
  out << R"(      <DataItem Format="XML" Dimensions="3">)" << origin[2] << " " << origin[1] << " " << origin[0] << "</DataItem>"
      << "\n";
  out << "      <!-- DxDyDz (Spacing/Spacing) Z, Y, X -->"
      << "\n";
  out << R"(      <DataItem Format="XML" Dimensions="3">)" << spacing[2] << " " << spacing[1] << " " << spacing[0] << "</DataItem>"
      << "\n";
  out << "    </Geometry>"
      << "\n";
}

void WriteGeomXdmf(std::ostream& out, const RectGridGeom& rectGridGeom, std::string_view hdf5FilePath)
{
  std::string name = rectGridGeom.getName();

  SizeVec3 dims = rectGridGeom.getDimensions();
  const Float32Array* xBounds = rectGridGeom.getXBounds();
  const Float32Array* yBounds = rectGridGeom.getYBounds();
  const Float32Array* zBounds = rectGridGeom.getZBounds();
  DataPath xBoundsPath = xBounds->getDataPaths().at(0);
  DataPath yBoundsPath = yBounds->getDataPaths().at(0);
  DataPath zBoundsPath = zBounds->getDataPaths().at(0);

  std::array<int64, 3> volDims = {static_cast<int64>(dims.getX()), static_cast<int64>(dims.getY()), static_cast<int64>(dims.getZ())};

  out << "  <!-- *************** START OF " << name << " *************** -->"
      << "\n";
  out << "  <Grid Name=\"" << name << R"(" GridType="Uniform">)"
      << "\n";
  out << "    <Topology TopologyType=\"3DRectMesh\" Dimensions=\"" << volDims[2] + 1 << " " << volDims[1] + 1 << " " << volDims[0] + 1 << " \"></Topology>"
      << "\n";
  out << "    <Geometry Type=\"VxVyVz\">"
      << "\n";
  out << "    <DataItem Format=\"HDF\" Dimensions=\"" << xBounds->getNumberOfTuples() << "\" NumberType=\"Float\" Precision=\"4\">"
      << "\n";
  out << "      " << hdf5FilePath << ":/DataStructure/" << xBoundsPath.toString() << "\n";
  out << "    </DataItem>"
      << "\n";
  out << "    <DataItem Format=\"HDF\" Dimensions=\"" << yBounds->getNumberOfTuples() << "\" NumberType=\"Float\" Precision=\"4\">"
      << "\n";
  out << "      " << hdf5FilePath << ":/DataStructure/" << yBoundsPath.toString() << "\n";
  out << "    </DataItem>"
      << "\n";
  out << "    <DataItem Format=\"HDF\" Dimensions=\"" << zBounds->getNumberOfTuples() << "\" NumberType=\"Float\" Precision=\"4\">"
      << "\n";
  out << "      " << hdf5FilePath << ":/DataStructure/" << zBoundsPath.toString() << "\n";
  out << "    </DataItem>"
      << "\n";
  out << "    </Geometry>"
      << "\n";
}

void WriteGeomXdmf(std::ostream& out, const VertexGeom& vertexGeom, std::string_view hdf5FilePath)
{
  std::string name = vertexGeom.getName();
  usize numVerts = vertexGeom.getNumberOfVertices();

  DataPath verticesPath = vertexGeom.getVerticesRef().getDataPaths().at(0);

  DataPath geomPath = vertexGeom.getDataPaths().at(0);

  out << "  <!-- *************** START OF " << name << " *************** -->"
      << "\n";
  out << "  <Grid Name=\"" << name << R"(" GridType="Uniform">)"
      << "\n";

  out << R"(    <Topology TopologyType="Polyvertex" NumberOfElements=")" << numVerts << "\">"
      << "\n";
  out << R"(      <DataItem Format="HDF" NumberType="Int" Dimensions=")" << numVerts << "\">"
      << "\n";
  out << "        " << hdf5FilePath << ":/DataStructure/" << geomPath.toString() << "/_VertexIndices"
      << "\n";
  out << "      </DataItem>"
      << "\n";
  out << "    </Topology>"
      << "\n";

  out << "    <Geometry Type=\"XYZ\">"
      << "\n";
  out << R"(      <DataItem Format="HDF"  Dimensions=")" << numVerts << R"( 3" NumberType="Float" Precision="4">)"
      << "\n";
  out << "        " << hdf5FilePath << ":/DataStructure/" << verticesPath.toString() << "\n";
  out << "      </DataItem>"
      << "\n";
  out << "    </Geometry>"
      << "\n";
  out << ""
      << "\n";
}

void WriteGeomXdmf(std::ostream& out, const EdgeGeom& edgeGeom, std::string_view hdf5FilePath)
{
  std::string name = edgeGeom.getName();
  usize numEdges = edgeGeom.getNumberOfCells();
  usize numVerts = edgeGeom.getNumberOfVertices();

  DataPath edgesPath = edgeGeom.getEdgesRef().getDataPaths().at(0);
  DataPath verticesPath = edgeGeom.getVerticesRef().getDataPaths().at(0);

  out << "  <!-- *************** START OF " << name << " *************** -->"
      << "\n";
  out << "  <Grid Name=\"" << name << "\" GridType=\"Uniform\">"
      << "\n";
  out << "    <Topology TopologyType=\"Polyline\" NodesPerElement=\"2\" NumberOfElements=\"" << numEdges << "\">"
      << "\n";
  out << "      <DataItem Format=\"HDF\" NumberType=\"Int\" Dimensions=\"" << numEdges << " 2\">"
      << "\n";
  out << "        " << hdf5FilePath << ":/DataStructure/" << edgesPath.toString() << "\n";
  out << "      </DataItem>"
      << "\n";
  out << "    </Topology>"
      << "\n";
  out << "    <Geometry Type=\"XYZ\">"
      << "\n";
  out << "      <DataItem Format=\"HDF\"  Dimensions=\"" << numVerts << " 3\" NumberType=\"Float\" Precision=\"4\">"
      << "\n";
  out << "        " << hdf5FilePath << ":/DataStructure/" << verticesPath.toString() << "\n";
  out << "      </DataItem>"
      << "\n";
  out << "    </Geometry>"
      << "\n";
  out << ""
      << "\n";
}

void WriteGeomXdmf(std::ostream& out, const TriangleGeom& triangleGeom, std::string_view hdf5FilePath)
{
  std::string name = triangleGeom.getName();
  usize numFaces = triangleGeom.getNumberOfFaces();
  usize numVerts = triangleGeom.getNumberOfVertices();

  DataPath facesPath = triangleGeom.getFacesRef().getDataPaths().at(0);
  DataPath verticesPath = triangleGeom.getVerticesRef().getDataPaths().at(0);

  out << "  <!-- *************** START OF " << name << " *************** -->"
      << "\n";
  out << "  <Grid Name=\"" << name << "\" GridType=\"Uniform\">"
      << "\n";
  out << "    <Topology TopologyType=\"Triangle\" NumberOfElements=\"" << numFaces << "\">"
      << "\n";
  out << "      <DataItem Format=\"HDF\" NumberType=\"Int\" Dimensions=\"" << numFaces << " 3\">"
      << "\n";
  out << "        " << hdf5FilePath << ":/DataStructure/" << facesPath.toString() << "\n";
  out << "      </DataItem>"
      << "\n";
  out << "    </Topology>"
      << "\n";
  out << "    <Geometry Type=\"XYZ\">"
      << "\n";
  out << "      <DataItem Format=\"HDF\"  Dimensions=\"" << numVerts << " 3\" NumberType=\"Float\" Precision=\"4\">"
      << "\n";
  out << "        " << hdf5FilePath << ":/DataStructure/" << verticesPath.toString() << "\n";
  out << "      </DataItem>"
      << "\n";
  out << "    </Geometry>"
      << "\n";
  out << ""
      << "\n";
}

void WriteGeomXdmf(std::ostream& out, const QuadGeom& quadGeom, std::string_view hdf5FilePath)
{
  std::string name = quadGeom.getName();
  usize numFaces = quadGeom.getNumberOfFaces();
  usize numVerts = quadGeom.getNumberOfVertices();

  DataPath facesPath = quadGeom.getFacesRef().getDataPaths().at(0);
  DataPath verticesPath = quadGeom.getVerticesRef().getDataPaths().at(0);

  out << "  <!-- *************** START OF " << name << " *************** -->"
      << "\n";
  out << "  <Grid Name=\"" << name << "\" GridType=\"Uniform\">"
      << "\n";
  out << "    <Topology TopologyType=\"Quadrilateral\" NumberOfElements=\"" << numFaces << "\">"
      << "\n";
  out << "      <DataItem Format=\"HDF\" NumberType=\"Int\" Dimensions=\"" << numFaces << " 4\">"
      << "\n";
  out << "        " << hdf5FilePath << ":/DataStructure/" << facesPath.toString() << "\n";
  out << "      </DataItem>"
      << "\n";
  out << "    </Topology>"
      << "\n";
  out << "    <Geometry Type=\"XYZ\">"
      << "\n";
  out << "      <DataItem Format=\"HDF\"  Dimensions=\"" << numVerts << " 3\" NumberType=\"Float\" Precision=\"4\">"
      << "\n";
  out << "        " << hdf5FilePath << ":/DataStructure/" << verticesPath.toString() << "\n";
  out << "      </DataItem>"
      << "\n";
  out << "    </Geometry>"
      << "\n";
  out << ""
      << "\n";
}

void WriteGeomXdmf(std::ostream& out, const TetrahedralGeom& tetrahedralGeom, std::string_view hdf5FilePath)
{
  std::string name = tetrahedralGeom.getName();
  usize numPolyhedra = tetrahedralGeom.getNumberOfPolyhedra();
  usize numVerts = tetrahedralGeom.getNumberOfVertices();

  DataPath polyhedraPath = tetrahedralGeom.getPolyhedraRef().getDataPaths().at(0);
  DataPath verticesPath = tetrahedralGeom.getVerticesRef().getDataPaths().at(0);

  out << "  <!-- *************** START OF " << name << " *************** -->"
      << "\n";
  out << "  <Grid Name=\"" << name << "\" GridType=\"Uniform\">"
      << "\n";
  out << "    <Topology TopologyType=\"Tetrahedron\" NumberOfElements=\"" << numPolyhedra << "\">"
      << "\n";
  out << "      <DataItem Format=\"HDF\" NumberType=\"Int\" Dimensions=\"" << numPolyhedra << " 4\">"
      << "\n";
  out << "        " << hdf5FilePath << ":/DataStructure/" << polyhedraPath.toString() << "\n";
  out << "      </DataItem>"
      << "\n";
  out << "    </Topology>"
      << "\n";
  out << "    <Geometry Type=\"XYZ\">"
      << "\n";
  out << "      <DataItem Format=\"HDF\"  Dimensions=\"" << numVerts << " 3\" NumberType=\"Float\" Precision=\"4\">"
      << "\n";
  out << "        " << hdf5FilePath << ":/DataStructure/" << verticesPath.toString() << "\n";
  out << "      </DataItem>"
      << "\n";
  out << "    </Geometry>"
      << "\n";
  out << ""
      << "\n";
}

void WriteGeomXdmf(std::ostream& out, const HexahedralGeom& hexhedralGeom, std::string_view hdf5FilePath)
{
  std::string name = hexhedralGeom.getName();
  usize numPolyhedra = hexhedralGeom.getNumberOfPolyhedra();
  usize numVerts = hexhedralGeom.getNumberOfVertices();

  DataPath polyhedraPath = hexhedralGeom.getPolyhedraRef().getDataPaths().at(0);
  DataPath verticesPath = hexhedralGeom.getVerticesRef().getDataPaths().at(0);

  out << "  <!-- *************** START OF " << name << " *************** -->"
      << "\n";
  out << "  <Grid Name=\"" << name << "\" GridType=\"Uniform\">"
      << "\n";
  out << "    <Topology TopologyType=\"Hexahedron\" NumberOfElements=\"" << numPolyhedra << "\">"
      << "\n";
  out << "      <DataItem Format=\"HDF\" NumberType=\"Int\" Dimensions=\"" << numPolyhedra << " 8\">"
      << "\n";
  out << "        " << hdf5FilePath << ":/DataStructure/" << polyhedraPath.toString() << "\n";
  out << "      </DataItem>"
      << "\n";
  out << "    </Topology>"
      << "\n";
  out << "    <Geometry Type=\"XYZ\">"
      << "\n";
  out << "      <DataItem Format=\"HDF\"  Dimensions=\"" << numVerts << " 3\" NumberType=\"Float\" Precision=\"4\">"
      << "\n";
  out << "        " << hdf5FilePath << ":/DataStructure/" << verticesPath.toString() << "\n";
  out << "      </DataItem>"
      << "\n";
  out << "    </Geometry>"
      << "\n";
  out << ""
      << "\n";
}

void WriteXdmfHeader(std::ostream& out)
{
  out << "<?xml version=\"1.0\"?>"
      << "\n";
  out << "<!DOCTYPE Xdmf SYSTEM \"Xdmf.dtd\"[]>"
      << "\n";
  out << "<Xdmf xmlns:xi=\"http://www.w3.org/2003/XInclude\" Version=\"2.2\">"
      << "\n";
  out << " <Domain>"
      << "\n";
}

void WriteXdmfFooter(std::ostream& xdmf)
{
  xdmf << " </Domain>"
       << "\n";
  xdmf << "</Xdmf>"
       << "\n";
}

std::string GetXdmfArrayType(usize numComp)
{
  switch(numComp)
  {
  case 1: {
    return "Scalar";
  }
    // we are assuming a component of 2 is for scalars on either side of a single object (ie faceIds)
  case 2: {
    return "Scalar";
  }
  case 3: {
    return "Vector";
  }
  case 6: {
    return "Vector";
  }
  case 9: {
    return "Tensor";
  }
  }

  return "";
}

void WriteXdmfAttributeDataHelper(std::ostream& out, usize numComp, std::string_view attrType, std::string_view dataContainerName, const IDataArray& array, std::string_view centering, usize precision,
                                  std::string_view xdmfTypeName, std::string_view hdf5FilePath)
{
  IDataStore::ShapeType tupleDims = array.getTupleShape();

  std::string tupleStr = fmt::format("{}", fmt::join(tupleDims.crbegin(), tupleDims.crend(), " "));

  std::string dimStr = fmt::format("{} {}", tupleStr, numComp);
  std::string dimStrHalf = fmt::format("{} {}", tupleStr, numComp / 2);

  std::string arrayName = array.getName();

  DataPath arrayPath = array.getDataPaths().at(0);

  std::string hdf5DatasetPath = fmt::format("{}:/DataStructure/{}", hdf5FilePath, arrayPath.toString());

  if(numComp == 1 || numComp == 3 || numComp == 9)
  {
    out << "    <Attribute Name=\"" << arrayName << "\" ";
    out << "AttributeType=\"" << attrType << "\" ";
    out << "Center=\"" << centering << "\">"
        << "\n";
    // Open the <DataItem> Tag
    out << R"(      <DataItem Format="HDF" Dimensions=")" << dimStr << "\" ";
    out << "NumberType=\"" << xdmfTypeName << "\" "
        << "Precision=\"" << precision << "\" >"
        << "\n";
    out << "        " << hdf5DatasetPath << "\n";
    out << "      </DataItem>"
        << "\n";
    out << "    </Attribute>"
        << "\n";
  }
  else if(numComp == 2 || numComp == 6)
  {
    // First Slab
    out << "    <Attribute Name=\"" << arrayName << " (Feature 0)\" ";
    out << "AttributeType=\"" << attrType << "\" ";

    out << "Center=\"" << centering << "\">"
        << "\n";
    // Open the <DataItem> Tag
    out << R"(      <DataItem ItemType="HyperSlab" Dimensions=")" << dimStrHalf << "\" ";
    out << "Type=\"HyperSlab\" "
        << "Name=\"" << arrayName << " (Feature 0)\" >"
        << "\n";
    out << "        <DataItem Dimensions=\"3 2\" "
        << "Format=\"XML\" >"
        << "\n";
    out << "          0        0"
        << "\n";
    out << "          1        1"
        << "\n";
    out << "          " << dimStrHalf << " </DataItem>"
        << "\n";
    out << "\n";
    out << R"(        <DataItem Format="HDF" Dimensions=")" << dimStr << "\" "
        << "NumberType=\"" << xdmfTypeName << "\" "
        << "Precision=\"" << precision << "\" >"
        << "\n";

    out << "        " << hdf5DatasetPath << "\n";
    out << "        </DataItem>"
        << "\n";
    out << "      </DataItem>"
        << "\n";
    out << "    </Attribute>"
        << "\n"
        << "\n";

    // Second Slab
    out << "    <Attribute Name=\"" << arrayName << " (Feature 1)\" ";
    out << "AttributeType=\"" << attrType << "\" ";

    out << "Center=\"" << centering << "\">"
        << "\n";
    // Open the <DataItem> Tag
    out << R"(      <DataItem ItemType="HyperSlab" Dimensions=")" << dimStrHalf << "\" ";
    out << "Type=\"HyperSlab\" "
        << "Name=\"" << arrayName << " (Feature 1)\" >"
        << "\n";
    out << "        <DataItem Dimensions=\"3 2\" "
        << "Format=\"XML\" >"
        << "\n";
    out << "          0        " << (numComp / 2) << "\n";
    out << "          1        1"
        << "\n";
    out << "          " << dimStrHalf << " </DataItem>"
        << "\n";
    out << "\n";
    out << R"(        <DataItem Format="HDF" Dimensions=")" << dimStr << "\" "
        << "NumberType=\"" << xdmfTypeName << "\" "
        << "Precision=\"" << precision << "\" >"
        << "\n";
    out << "        " << hdf5DatasetPath << "\n";
    out << "        </DataItem>"
        << "\n";
    out << "      </DataItem>"
        << "\n";
    out << "    </Attribute>"
        << "\n";
  }
}

void WriteXdmfGeomFooter(std::ostream& xdmf, std::string_view geomName)
{
  xdmf << "  </Grid>"
       << "\n";
  xdmf << "  <!-- *************** END OF " << geomName << " *************** -->"
       << "\n";
}

void WriteXdmfAttributeMatrix(std::ostream& out, const AttributeMatrix& attributeMatrix, std::string_view geomName, std::string_view hdf5FilePath, std::string_view centering)
{
  for(const auto& [arrayId, arrayObject] : attributeMatrix)
  {
    const auto* dataArray = dynamic_cast<const IDataArray*>(arrayObject.get());
    if(dataArray == nullptr)
    {
      continue;
    }
    usize numComp = dataArray->getNumberOfComponents();
    DataType dataType = dataArray->getDataType();
    auto [xdmfTypeName, precision] = GetXdmfTypeAndSize(dataType);
    std::string attrType = GetXdmfArrayType(numComp);
    WriteXdmfAttributeDataHelper(out, numComp, attrType, geomName, *dataArray, centering, precision, xdmfTypeName, hdf5FilePath);
  }
}

void WriteXdmfGridGeometry(std::ostream& out, const IGridGeometry& gridGeometry, std::string_view geomName, std::string_view hdf5FilePath)
{
  const AttributeMatrix* cellData = gridGeometry.getCellData();
  if(cellData == nullptr)
  {
    return;
  }
  WriteXdmfAttributeMatrix(out, *cellData, geomName, hdf5FilePath, "Cell");
}

void WriteXdmfNodeGeometry0D(std::ostream& out, const INodeGeometry0D& nodeGeom0D, std::string_view geomName, std::string_view hdf5FilePath)
{
  const AttributeMatrix* vertexData = nodeGeom0D.getVertexAttributeMatrix();
  if(vertexData == nullptr)
  {
    return;
  }
  WriteXdmfAttributeMatrix(out, *vertexData, geomName, hdf5FilePath, "Node");
}

void WriteXdmfNodeGeometry1D(std::ostream& out, const INodeGeometry1D& nodeGeom1D, std::string_view geomName, std::string_view hdf5FilePath)
{
  WriteXdmfNodeGeometry0D(out, nodeGeom1D, hdf5FilePath, geomName);

  const AttributeMatrix* edgeData = nodeGeom1D.getEdgeAttributeMatrix();
  if(edgeData == nullptr)
  {
    return;
  }
  WriteXdmfAttributeMatrix(out, *edgeData, geomName, hdf5FilePath, "Cell");
}

void WriteXdmfNodeGeometry2D(std::ostream& out, const INodeGeometry2D& nodeGeom2D, std::string_view geomName, std::string_view hdf5FilePath)
{
  WriteXdmfNodeGeometry1D(out, nodeGeom2D, hdf5FilePath, geomName);

  const AttributeMatrix* faceData = nodeGeom2D.getFaceAttributeMatrix();
  if(faceData == nullptr)
  {
    return;
  }
  WriteXdmfAttributeMatrix(out, *faceData, geomName, hdf5FilePath, "Cell");
}

void WriteXdmfNodeGeometry3D(std::ostream& out, const INodeGeometry3D& nodeGeom3D, std::string_view geomName, std::string_view hdf5FilePath)
{
  WriteXdmfNodeGeometry2D(out, nodeGeom3D, hdf5FilePath, geomName);

  const AttributeMatrix* polyhedraData = nodeGeom3D.getPolyhedraAttributeMatrix();
  if(polyhedraData == nullptr)
  {
    return;
  }
  WriteXdmfAttributeMatrix(out, *polyhedraData, geomName, hdf5FilePath, "Cell");
}

void WriteXdmf(std::ostream& out, const DataStructure& dataStructure, std::string_view hdf5FilePath)
{
  std::stringstream ss;

  WriteXdmfHeader(ss);

  for(const auto& [identifier, object] : dataStructure)
  {
    const auto* geometry = dynamic_cast<const IGeometry*>(object.get());
    if(geometry == nullptr)
    {
      continue;
    }

    std::string geomName = geometry->getName();

    IGeometry::Type geomType = geometry->getGeomType();

    switch(geomType)
    {
    case IGeometry::Type::Image: {
      const auto& imageGeom = dynamic_cast<const ImageGeom&>(*object);
      WriteGeomXdmf(ss, imageGeom, hdf5FilePath);
      WriteXdmfGridGeometry(ss, imageGeom, geomName, hdf5FilePath);
      break;
    }
    case IGeometry::Type::RectGrid: {
      const auto& rectGridGeom = dynamic_cast<const RectGridGeom&>(*object);
      WriteGeomXdmf(ss, rectGridGeom, hdf5FilePath);
      WriteXdmfGridGeometry(ss, rectGridGeom, geomName, hdf5FilePath);
      break;
    }
    case IGeometry::Type::Vertex: {
      const auto& vertexGeom = dynamic_cast<const VertexGeom&>(*object);
      WriteGeomXdmf(ss, vertexGeom, hdf5FilePath);
      WriteXdmfNodeGeometry0D(ss, vertexGeom, geomName, hdf5FilePath);
      break;
    }
    case IGeometry::Type::Edge: {
      const auto& edgeGeom = dynamic_cast<const EdgeGeom&>(*object);
      WriteGeomXdmf(ss, edgeGeom, hdf5FilePath);
      WriteXdmfNodeGeometry1D(ss, edgeGeom, geomName, hdf5FilePath);
      break;
    }
    case IGeometry::Type::Triangle: {
      const auto& triangleGeom = dynamic_cast<const TriangleGeom&>(*object);
      WriteGeomXdmf(ss, triangleGeom, hdf5FilePath);
      WriteXdmfNodeGeometry2D(ss, triangleGeom, geomName, hdf5FilePath);
      break;
    }
    case IGeometry::Type::Quad: {
      const auto& quadGeom = dynamic_cast<const QuadGeom&>(*object);
      WriteGeomXdmf(ss, quadGeom, hdf5FilePath);
      WriteXdmfNodeGeometry2D(ss, quadGeom, geomName, hdf5FilePath);
      break;
    }
    case IGeometry::Type::Tetrahedral: {
      const auto& tetrahedralGeom = dynamic_cast<const TetrahedralGeom&>(*object);
      WriteGeomXdmf(ss, tetrahedralGeom, hdf5FilePath);
      WriteXdmfNodeGeometry3D(ss, tetrahedralGeom, geomName, hdf5FilePath);
      break;
    }
    case IGeometry::Type::Hexahedral: {
      const auto& hexahedralGeom = dynamic_cast<const HexahedralGeom&>(*object);
      WriteGeomXdmf(ss, hexahedralGeom, hdf5FilePath);
      WriteXdmfNodeGeometry3D(ss, hexahedralGeom, geomName, hdf5FilePath);
      break;
    }
    }

    WriteXdmfGeomFooter(ss, geomName);
  }

  WriteXdmfFooter(ss);

  out << ss.str();
}
} // namespace

void DREAM3D::WriteXdmf(const std::filesystem::path& filePath, const DataStructure& dataStructure, std::string_view hdf5FilePath)
{
  std::ofstream file(filePath);

  ::WriteXdmf(file, dataStructure, hdf5FilePath);
}

DREAM3D::FileVersionType DREAM3D::GetFileVersion(const complex::HDF5::FileReader& fileReader)
{
  auto fileVersionAttribute = fileReader.getAttribute(k_FileVersionTag);
  if(!fileVersionAttribute.isValid())
  {
    return "";
  }
  return fileVersionAttribute.readAsString();
}

DREAM3D::PipelineVersionType DREAM3D::GetPipelineVersion(const complex::HDF5::FileReader& fileReader)
{
  auto pipelineGroup = fileReader.openGroup(k_PipelineJsonTag);
  auto pipelineVersionAttribute = pipelineGroup.getAttribute(k_PipelineVersionTag);
  if(!pipelineVersionAttribute.isValid())
  {
    return 0;
  }
  return pipelineVersionAttribute.readAsValue<PipelineVersionType>();
}

Result<DataStructure> ImportDataStructureV8(const complex::HDF5::FileReader& fileReader, bool preflight)
{
  return HDF5::DataStructureReader::ReadFile(fileReader, preflight);
}

// Begin legacy DCA importing

/**
 * @brief
 * @tparam T
 * @param dataStructure
 * @param name
 * @param parentId
 * @param daId
 * @param tDims
 * @param cDims
 */
template <typename T>
DataArray<T>* createLegacyDataArray(DataStructure& dataStructure, DataObject::IdType parentId, const HDF5::DatasetReader& dataArrayReader, const std::vector<usize>& tDims,
                                    const std::vector<usize>& cDims, bool preflight = false)
{
  using DataArrayType = DataArray<T>;
  using EmptyDataStoreType = EmptyDataStore<T>;

  const std::string daName = dataArrayReader.getName();

  if(preflight)
  {
    return DataArrayType::template CreateWithStore<EmptyDataStoreType>(dataStructure, daName, tDims, cDims, parentId);
  }
  auto dataStore = std::make_unique<DataStore<T>>(tDims, cDims, static_cast<T>(0));

  if(!dataArrayReader.readIntoSpan(dataStore->createSpan()))
  {
    throw std::runtime_error(fmt::format("Error reading HDF5 Data set {}", complex::HDF5::Support::GetObjectPath(dataArrayReader.getId())));
  }
  // Insert the DataArray into the DataStructure
  return DataArray<T>::Create(dataStructure, daName, std::move(dataStore), parentId);
}

/**
 * @brief
 * @param daId
 * @param tDims
 * @param cDims
 */
void readLegacyDataArrayDims(const complex::HDF5::DatasetReader& dataArrayReader, std::vector<usize>& tDims, std::vector<usize>& cDims)
{
  {
    auto compAttrib = dataArrayReader.getAttribute(Legacy::CompDims);
    if(!compAttrib.isValid())
    {
      throw std::runtime_error("Error reading legacy DataArray dimensions");
    }
    // From SIMPL, the component dimensions are actually all ordered correctly in the HDF5 file. Somehow.
    cDims = compAttrib.readAsVector<usize>();
  }

  {
    auto tupleAttrib = dataArrayReader.getAttribute(Legacy::TupleDims);
    if(!tupleAttrib.isValid())
    {
      throw std::runtime_error("Error reading legacy DataArray dimensions");
    }
    tDims = tupleAttrib.readAsVector<usize>();
    std::reverse(tDims.begin(), tDims.end()); // SIMPL writes the Tuple Dimensions in reverse order to this attribute
  }
}

void readLegacyStringArray(DataStructure& dataStructure, const complex::HDF5::DatasetReader& dataArrayReader, DataObject::IdType parentId, bool preflight = false)
{
  const std::string daName = dataArrayReader.getName();

  if(preflight)
  {
    std::vector<usize> tDims;
    std::vector<usize> cDims;
    readLegacyDataArrayDims(dataArrayReader, tDims, cDims);
    auto numElements =
        std::accumulate(tDims.cbegin(), tDims.cend(), static_cast<usize>(1), std::multiplies<>()) * std::accumulate(cDims.cbegin(), cDims.cend(), static_cast<usize>(1), std::multiplies<>());
    const std::vector<std::string> strings(numElements);
    StringArray::CreateWithValues(dataStructure, daName, strings, parentId);
  }
  else
  {
    const std::vector<std::string> strings = dataArrayReader.readAsVectorOfStrings();
    StringArray::CreateWithValues(dataStructure, daName, strings, parentId);
  }
}

IDataArray* readLegacyDataArray(DataStructure& dataStructure, const complex::HDF5::DatasetReader& dataArrayReader, DataObject::IdType parentId, bool preflight = false)
{
  auto size = H5Dget_storage_size(dataArrayReader.getId());
  auto typeId = dataArrayReader.getTypeId();

  std::vector<usize> tDims;
  std::vector<usize> cDims;
  readLegacyDataArrayDims(dataArrayReader, tDims, cDims);

  IDataArray* dataArray = nullptr;

  if(H5Tequal(typeId, H5T_NATIVE_FLOAT) > 0)
  {
    dataArray = createLegacyDataArray<float32>(dataStructure, parentId, dataArrayReader, tDims, cDims, preflight);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_DOUBLE) > 0)
  {
    dataArray = createLegacyDataArray<float64>(dataStructure, parentId, dataArrayReader, tDims, cDims, preflight);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_INT8) > 0)
  {
    dataArray = createLegacyDataArray<int8>(dataStructure, parentId, dataArrayReader, tDims, cDims, preflight);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_INT16) > 0)
  {
    dataArray = createLegacyDataArray<int16>(dataStructure, parentId, dataArrayReader, tDims, cDims, preflight);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_INT32) > 0)
  {
    dataArray = createLegacyDataArray<int32>(dataStructure, parentId, dataArrayReader, tDims, cDims, preflight);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_INT64) > 0)
  {
    dataArray = createLegacyDataArray<int64>(dataStructure, parentId, dataArrayReader, tDims, cDims, preflight);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_UINT8) > 0)
  {
    const auto typeAttrib = dataArrayReader.getAttribute(Constants::k_ObjectTypeTag);
    if(typeAttrib.isValid() && typeAttrib.readAsString() == "DataArray<bool>")
    {
      dataArray = createLegacyDataArray<bool>(dataStructure, parentId, dataArrayReader, tDims, cDims, preflight);
    }
    else
    {
      dataArray = createLegacyDataArray<uint8>(dataStructure, parentId, dataArrayReader, tDims, cDims, preflight);
    }
  }
  else if(H5Tequal(typeId, H5T_NATIVE_UINT16) > 0)
  {
    dataArray = createLegacyDataArray<uint16>(dataStructure, parentId, dataArrayReader, tDims, cDims, preflight);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_UINT32) > 0)
  {
    dataArray = createLegacyDataArray<uint32>(dataStructure, parentId, dataArrayReader, tDims, cDims, preflight);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_UINT64) > 0)
  {
    dataArray = createLegacyDataArray<uint64>(dataStructure, parentId, dataArrayReader, tDims, cDims, preflight);
  }

  H5Tclose(typeId);

  return dataArray;
}

UInt64Array* readLegacyNodeConnectivityList(DataStructure& dataStructure, IGeometry* geometry, const HDF5::GroupReader& geomGroup, const std::string& arrayName, bool preflight = false)
{
  HDF5::DatasetReader dataArrayReader = geomGroup.openDataset(arrayName);
  DataObject::IdType parentId = geometry->getId();

  auto size = H5Dget_storage_size(dataArrayReader.getId());

  std::vector<usize> tDims;
  std::vector<usize> cDims;
  readLegacyDataArrayDims(dataArrayReader, tDims, cDims);

  return createLegacyDataArray<uint64>(dataStructure, parentId, dataArrayReader, tDims, cDims, preflight);
}

template <typename T>
void createLegacyNeighborList(DataStructure& dataStructure, DataObject ::IdType parentId, const complex::HDF5::GroupReader& parentReader, const complex::HDF5::DatasetReader& datasetReader,
                              const std::vector<usize>& tupleDims)
{
  auto numTuples = std::accumulate(tupleDims.cbegin(), tupleDims.cend(), static_cast<usize>(1), std::multiplies<>());

  auto data = HDF5::NeighborListIO<T>::ReadHdf5Data(parentReader, datasetReader);
  auto* neighborList = NeighborList<T>::Create(dataStructure, datasetReader.getName(), numTuples, parentId);
  for(usize i = 0; i < data.size(); ++i)
  {
    neighborList->setList(i, data[i]);
  }
}

void readLegacyNeighborList(DataStructure& dataStructure, const complex::HDF5::GroupReader& parentReader, const complex::HDF5::DatasetReader& datasetReader, DataObject::IdType parentId)
{
  auto typeId = datasetReader.getTypeId();

  auto tupleAttrib = datasetReader.getAttribute(Legacy::TupleDims);
  if(!tupleAttrib.isValid())
  {
    throw std::runtime_error("Error reading legacy DataArray dimensions");
  }

  auto tDims = tupleAttrib.readAsVector<usize>();

  if(H5Tequal(typeId, H5T_NATIVE_FLOAT) > 0)
  {
    createLegacyNeighborList<float32>(dataStructure, parentId, parentReader, datasetReader, tDims);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_DOUBLE) > 0)
  {
    createLegacyNeighborList<float64>(dataStructure, parentId, parentReader, datasetReader, tDims);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_INT8) > 0)
  {
    createLegacyNeighborList<int8>(dataStructure, parentId, parentReader, datasetReader, tDims);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_INT16) > 0)
  {
    createLegacyNeighborList<int16>(dataStructure, parentId, parentReader, datasetReader, tDims);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_INT32) > 0)
  {
    createLegacyNeighborList<int32>(dataStructure, parentId, parentReader, datasetReader, tDims);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_INT64) > 0)
  {
    createLegacyNeighborList<int64>(dataStructure, parentId, parentReader, datasetReader, tDims);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_UINT8) > 0)
  {
    createLegacyNeighborList<uint8>(dataStructure, parentId, parentReader, datasetReader, tDims);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_UINT16) > 0)
  {
    createLegacyNeighborList<uint16>(dataStructure, parentId, parentReader, datasetReader, tDims);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_UINT32) > 0)
  {
    createLegacyNeighborList<uint32>(dataStructure, parentId, parentReader, datasetReader, tDims);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_UINT64) > 0)
  {
    createLegacyNeighborList<uint64>(dataStructure, parentId, parentReader, datasetReader, tDims);
  }

  H5Tclose(typeId);
}

bool isLegacyNeighborList(const complex::HDF5::DatasetReader& arrayReader)
{
  const auto objectTypeAttribute = arrayReader.getAttribute("ObjectType");
  const std::string objectType = objectTypeAttribute.readAsString();
  return objectType == "NeighborList<T>";
}

bool isLegacyStringArray(const complex::HDF5::DatasetReader& arrayReader)
{
  const auto objectTypeAttribute = arrayReader.getAttribute("ObjectType");
  const std::string objectType = objectTypeAttribute.readAsString();
  return objectType == "StringDataArray";
}

void readLegacyAttributeMatrix(DataStructure& dataStructure, const complex::HDF5::GroupReader& amGroupReader, DataObject& parent, bool preflight = false)
{
  DataObject::IdType parentId = parent.getId();
  const std::string amName = amGroupReader.getName();
  auto* attributeMatrix = AttributeMatrix::Create(dataStructure, amName, parentId);

  auto tDimsReader = amGroupReader.getAttribute("TupleDimensions");
  auto tDims = tDimsReader.readAsVector<uint64>();
  auto reversedTDims = AttributeMatrix::ShapeType(tDims.crbegin(), tDims.crend());

  attributeMatrix->setShape(reversedTDims);

  auto dataArrayNames = amGroupReader.getChildNames();
  for(const auto& daName : dataArrayNames)
  {
    auto dataArraySet = amGroupReader.openDataset(daName);

    if(isLegacyNeighborList(dataArraySet))
    {
      readLegacyNeighborList(dataStructure, amGroupReader, dataArraySet, attributeMatrix->getId());
    }
    else if(isLegacyStringArray(dataArraySet))
    {
      readLegacyStringArray(dataStructure, dataArraySet, attributeMatrix->getId(), preflight);
    }
    else
    {
      readLegacyDataArray(dataStructure, dataArraySet, attributeMatrix->getId(), preflight);
    }
  }

  auto amTypeReader = amGroupReader.getAttribute("AttributeMatrixType");

  auto amType = amTypeReader.readAsValue<uint32>();
  switch(amType)
  {
  case 0: {
    auto* nodeGeom0D = dynamic_cast<INodeGeometry0D*>(&parent);
    if(nodeGeom0D != nullptr)
    {
      nodeGeom0D->setVertexAttributeMatrix(*attributeMatrix);
    }
    break;
  }
  case 1: {
    auto* nodeGeom1D = dynamic_cast<INodeGeometry1D*>(&parent);
    if(nodeGeom1D != nullptr)
    {
      nodeGeom1D->setEdgeAttributeMatrix(*attributeMatrix);
    }
    break;
  }
  case 2: {
    auto* nodeGeom2D = dynamic_cast<INodeGeometry2D*>(&parent);
    if(nodeGeom2D != nullptr)
    {
      nodeGeom2D->setFaceAttributeMatrix(*attributeMatrix);
    }
    break;
  }
  case 3: {
    auto* gridGeom = dynamic_cast<IGridGeometry*>(&parent);
    if(gridGeom != nullptr)
    {
      gridGeom->setCellData(*attributeMatrix);
    }
    break;
  }
  }
}

// Begin legacy geometry import methods
void readGenericGeomDims(IGeometry* geom, const complex::HDF5::GroupReader& geomGroup)
{
  auto sDimAttribute = geomGroup.getAttribute("SpatialDimensionality");
  auto sDims = sDimAttribute.readAsValue<int32>();

  auto uDimAttribute = geomGroup.getAttribute("UnitDimensionality");
  auto uDims = uDimAttribute.readAsValue<int32>();

  geom->setSpatialDimensionality(sDims);
  geom->setUnitDimensionality(uDims);
}

IDataArray* readLegacyGeomArray(DataStructure& dataStructure, IGeometry* geometry, const complex::HDF5::GroupReader& geomGroup, const std::string& arrayName)
{
  auto dataArraySet = geomGroup.openDataset(arrayName);
  return readLegacyDataArray(dataStructure, dataArraySet, geometry->getId());
}

template <typename T>
T* readLegacyGeomArrayAs(DataStructure& dataStructure, IGeometry* geometry, const complex::HDF5::GroupReader& geomGroup, const std::string& arrayName)
{
  return dynamic_cast<T*>(readLegacyGeomArray(dataStructure, geometry, geomGroup, arrayName));
}

DataObject* readLegacyVertexGeom(DataStructure& dataStructure, const complex::HDF5::GroupReader& geomGroup, const std::string& name)
{
  auto* geom = VertexGeom::Create(dataStructure, name);
  readGenericGeomDims(geom, geomGroup);
  auto* sharedVertexList = readLegacyGeomArrayAs<Float32Array>(dataStructure, geom, geomGroup, Legacy::VertexListName);

  geom->setVertices(*sharedVertexList);
  return geom;
}

DataObject* readLegacyTriangleGeom(DataStructure& dataStructure, const complex::HDF5::GroupReader& geomGroup, const std::string& name)
{
  auto geom = TriangleGeom::Create(dataStructure, name);
  readGenericGeomDims(geom, geomGroup);
  auto* sharedVertexList = readLegacyGeomArrayAs<Float32Array>(dataStructure, geom, geomGroup, Legacy::VertexListName);
  UInt64Array* sharedTriList = readLegacyNodeConnectivityList(dataStructure, geom, geomGroup, Legacy::TriListName);

  geom->setVertices(*sharedVertexList);
  geom->setFaceList(*sharedTriList);

  return geom;
}

DataObject* readLegacyTetrahedralGeom(DataStructure& dataStructure, const complex::HDF5::GroupReader& geomGroup, const std::string& name)
{
  auto geom = TetrahedralGeom::Create(dataStructure, name);
  readGenericGeomDims(geom, geomGroup);
  auto* sharedVertexList = readLegacyGeomArrayAs<Float32Array>(dataStructure, geom, geomGroup, Legacy::VertexListName);
  UInt64Array* sharedTetList = readLegacyNodeConnectivityList(dataStructure, geom, geomGroup, Legacy::TetraListName);

  geom->setVertices(*sharedVertexList);
  geom->setPolyhedraList(*sharedTetList);

  return geom;
}

DataObject* readLegacyRectGridGeom(DataStructure& dataStructure, const complex::HDF5::GroupReader& geomGroup, const std::string& name)
{
  auto geom = RectGridGeom::Create(dataStructure, name);
  readGenericGeomDims(geom, geomGroup);

  // DIMENSIONS array
  {
    auto dimsDataset = geomGroup.openDataset("DIMENSIONS");
    auto dims = dimsDataset.readAsVector<int64>();
    geom->setDimensions(SizeVec3(dims[0], dims[1], dims[2]));
  }

  auto* xBoundsArray = readLegacyGeomArrayAs<Float32Array>(dataStructure, geom, geomGroup, Legacy::XBoundsName);
  auto* yBoundsArray = readLegacyGeomArrayAs<Float32Array>(dataStructure, geom, geomGroup, Legacy::YBoundsName);
  auto* zBoundsArray = readLegacyGeomArrayAs<Float32Array>(dataStructure, geom, geomGroup, Legacy::ZBoundsName);

  geom->setBounds(xBoundsArray, yBoundsArray, zBoundsArray);

  return geom;
}

DataObject* readLegacyQuadGeom(DataStructure& dataStructure, const complex::HDF5::GroupReader& geomGroup, const std::string& name)
{
  auto geom = QuadGeom::Create(dataStructure, name);
  readGenericGeomDims(geom, geomGroup);
  auto* sharedVertexList = readLegacyGeomArrayAs<Float32Array>(dataStructure, geom, geomGroup, Legacy::VertexListName);
  UInt64Array* sharedQuadList = readLegacyNodeConnectivityList(dataStructure, geom, geomGroup, Legacy::QuadListName);

  geom->setVertices(*sharedVertexList);
  geom->setFaceList(*sharedQuadList);

  return geom;
}

DataObject* readLegacyHexGeom(DataStructure& dataStructure, const complex::HDF5::GroupReader& geomGroup, const std::string& name)
{
  auto geom = HexahedralGeom::Create(dataStructure, name);
  readGenericGeomDims(geom, geomGroup);
  auto* sharedVertexList = readLegacyGeomArrayAs<Float32Array>(dataStructure, geom, geomGroup, Legacy::VertexListName);
  UInt64Array* sharedHexList = readLegacyNodeConnectivityList(dataStructure, geom, geomGroup, Legacy::HexListName);

  geom->setVertices(*sharedVertexList);
  geom->setPolyhedraList(*sharedHexList);

  return geom;
}

DataObject* readLegacyEdgeGeom(DataStructure& dataStructure, const complex::HDF5::GroupReader& geomGroup, const std::string& name)
{
  auto geom = EdgeGeom::Create(dataStructure, name);
  auto edge = dynamic_cast<EdgeGeom*>(geom);
  readGenericGeomDims(geom, geomGroup);
  auto* sharedVertexList = readLegacyGeomArrayAs<Float32Array>(dataStructure, geom, geomGroup, Legacy::VertexListName);
  UInt64Array* sharedEdgeList = readLegacyNodeConnectivityList(dataStructure, geom, geomGroup, Legacy::EdgeListName);

  geom->setVertices(*sharedVertexList);
  geom->setEdgeList(*sharedEdgeList);

  return geom;
}

DataObject* readLegacyImageGeom(DataStructure& dataStructure, const complex::HDF5::GroupReader& geomGroup, const std::string& name)
{
  auto geom = ImageGeom::Create(dataStructure, name);
  auto image = dynamic_cast<ImageGeom*>(geom);

  readGenericGeomDims(geom, geomGroup);

  // DIMENSIONS array
  {
    auto dimsDataset = geomGroup.openDataset("DIMENSIONS");
    auto dims = dimsDataset.readAsVector<int64>();
    image->setDimensions(SizeVec3(dims[0], dims[1], dims[2]));
  }

  // ORIGIN array
  {
    auto originDataset = geomGroup.openDataset("ORIGIN");
    auto origin = originDataset.readAsVector<float32>();
    image->setOrigin(FloatVec3(origin[0], origin[1], origin[2]));
  }

  // SPACING array
  {
    auto spacingDataset = geomGroup.openDataset("SPACING");
    auto spacing = spacingDataset.readAsVector<float32>();
    image->setSpacing(FloatVec3(spacing[0], spacing[1], spacing[2]));
  }

  return image;
}
// End legacy Geometry importing

void readLegacyDataContainer(DataStructure& dataStructure, const complex::HDF5::GroupReader& dcGroup, bool preflight = false)
{
  DataObject* container = nullptr;
  const std::string dcName = dcGroup.getName();

  // Check for geometry
  auto geomGroup = dcGroup.openGroup(Legacy::GeometryTag.c_str());
  if(geomGroup.isValid())
  {
    auto geomNameAttribute = geomGroup.getAttribute(Legacy::GeometryTypeNameTag);
    const std::string geomName = geomNameAttribute.readAsString();
    if(geomName == Legacy::Type::ImageGeom)
    {
      container = readLegacyImageGeom(dataStructure, geomGroup, dcName);
    }
    else if(geomName == Legacy::Type::EdgeGeom)
    {
      container = readLegacyEdgeGeom(dataStructure, geomGroup, dcName);
    }
    else if(geomName == Legacy::Type::HexGeom)
    {
      container = readLegacyHexGeom(dataStructure, geomGroup, dcName);
    }
    else if(geomName == Legacy::Type::QuadGeom)
    {
      container = readLegacyQuadGeom(dataStructure, geomGroup, dcName);
    }
    else if(geomName == Legacy::Type::RectGridGeom)
    {
      container = readLegacyRectGridGeom(dataStructure, geomGroup, dcName);
    }
    else if(geomName == Legacy::Type::TetrahedralGeom)
    {
      container = readLegacyTetrahedralGeom(dataStructure, geomGroup, dcName);
    }
    else if(geomName == Legacy::Type::TriangleGeom)
    {
      container = readLegacyTriangleGeom(dataStructure, geomGroup, dcName);
    }
    else if(geomName == Legacy::Type::VertexGeom)
    {
      container = readLegacyVertexGeom(dataStructure, geomGroup, dcName);
    }
  }

  // No geometry found. Create a DataGroup instead
  if(!container)
  {
    container = DataGroup::Create(dataStructure, dcName);
  }

  auto attribMatrixNames = dcGroup.getChildNames();
  for(const auto& amName : attribMatrixNames)
  {
    if(amName == Legacy::GeometryTag)
    {
      continue;
    }

    auto attributeMatrixGroup = dcGroup.openGroup(amName);
    readLegacyAttributeMatrix(dataStructure, attributeMatrixGroup, *container, preflight);
  }
}

Result<DataStructure> ImportLegacyDataStructure(const complex::HDF5::FileReader& fileReader, bool preflight)
{
  DataStructure dataStructure;

  auto dcaGroup = fileReader.openGroup(k_LegacyDataStructureGroupTag);

  // Iterate over DataContainers
  const auto dcNames = dcaGroup.getChildNames();
  for(const auto& dcName : dcNames)
  {
    auto dcGroup = dcaGroup.openGroup(dcName);
    readLegacyDataContainer(dataStructure, dcGroup, preflight);
  }

  return {std::move(dataStructure)};
}

Result<DataStructure> DREAM3D::ImportDataStructureFromFile(const complex::HDF5::FileReader& fileReader, bool preflight)
{
  const auto fileVersion = GetFileVersion(fileReader);
  if(fileVersion == k_CurrentFileVersion)
  {
    return ImportDataStructureV8(fileReader, preflight);
  }
  else if(fileVersion == Legacy::FileVersion)
  {
    return ImportLegacyDataStructure(fileReader, preflight);
  }
  // Unsupported file version
  return MakeErrorResult<DataStructure>(k_InvalidDataStructureVersion,
                                        fmt::format("Could not parse DataStructure version {}. Expected versions: {} or {}", fileVersion, k_CurrentFileVersion, Legacy::FileVersion));
}

Result<DataStructure> DREAM3D::ImportDataStructureFromFile(const std::filesystem::path& filePath)
{
  complex::HDF5::FileReader fileReader(filePath);
  if(!fileReader.isValid())
  {
    return MakeErrorResult<DataStructure>(-1, fmt::format("DREAM3D::ImportDataStructureFromFile: Unable to open '{}' for reading", filePath.string()));
  }

  complex::HDF5::ErrorType error = 0;
  return ImportDataStructureFromFile(fileReader, error);
}

Result<Pipeline> DREAM3D::ImportPipelineFromFile(const complex::HDF5::FileReader& fileReader)
{
  if(GetPipelineVersion(fileReader) != k_CurrentPipelineVersion)
  {
    return MakeErrorResult<Pipeline>(k_InvalidPipelineVersion, fmt::format("Could not parse Pipeline version '{}'. Expected version: '{}'", GetPipelineVersion(fileReader), k_CurrentFileVersion));
  }

  auto pipelineGroupReader = fileReader.openGroup(k_PipelineJsonTag);
  auto pipelineDatasetReader = pipelineGroupReader.openDataset(k_PipelineJsonTag);
  if(!pipelineDatasetReader.isValid())
  {
    return MakeErrorResult<Pipeline>(k_PipelineGroupUnavailable, "Could not open '/Pipeline' HDF5 Group.");
  }

  auto pipelineJsonString = pipelineDatasetReader.readAsString();
  auto pipelineJson = nlohmann::json::parse(pipelineJsonString);
  return Pipeline::FromJson(pipelineJson);
}

Result<Pipeline> DREAM3D::ImportPipelineFromFile(const std::filesystem::path& filePath)
{
  if(!std::filesystem::exists(filePath))
  {
    return MakeErrorResult<Pipeline>(-1, fmt::format("DREAM3D::ImportPipelineFromFile: File does not exist. '{}'", filePath.string()));
  }
  complex::HDF5::FileReader fileReader(filePath);
  if(!fileReader.isValid())
  {
    return MakeErrorResult<Pipeline>(-1, fmt::format("DREAM3D::ImportPipelineFromFile: Unable to open '{}' for reading", filePath.string()));
  }

  return ImportPipelineFromFile(fileReader);
}

Result<DREAM3D::FileData> DREAM3D::ReadFile(const complex::HDF5::FileReader& fileReader, bool preflight)
{
  // Pipeline pipeline;
  auto pipeline = ImportPipelineFromFile(fileReader);
  if(pipeline.invalid())
  {
    return {{nonstd::make_unexpected(std::move(pipeline.errors()))}, std::move(pipeline.warnings())};
  }

  auto dataStructure = ImportDataStructureFromFile(fileReader, preflight);
  if(pipeline.invalid())
  {
    return {{nonstd::make_unexpected(std::move(dataStructure.errors()))}, std::move(dataStructure.warnings())};
  }

  return {DREAM3D::FileData{std::move(pipeline.value()), std::move(dataStructure.value())}};
}

Result<DREAM3D::FileData> DREAM3D::ReadFile(const std::filesystem::path& path)
{
  complex::HDF5::FileReader reader(path);
  complex::HDF5::ErrorType error = 0;

  Result<FileData> fileData = ReadFile(reader, error);
  if(error < 0)
  {
    return MakeErrorResult<FileData>(-1, fmt::format("DREAM3D::ReadFile: Unable to read '{}'", path.string()));
  }
  return {std::move(fileData)};
}

complex::HDF5::ErrorType WritePipeline(complex::HDF5::FileWriter& fileWriter, const Pipeline& pipeline)
{
  if(!fileWriter.isValid())
  {
    return -1;
  }

  auto pipelineGroupWriter = fileWriter.createGroupWriter(k_PipelineJsonTag);
  auto versionAttribute = pipelineGroupWriter.createAttribute(k_PipelineVersionTag);
  auto errorCode = versionAttribute.writeValue<DREAM3D::PipelineVersionType>(k_CurrentPipelineVersion);
  if(errorCode < 0)
  {
    return errorCode;
  }

  auto nameAttribute = pipelineGroupWriter.createAttribute(k_PipelineNameTag);
  errorCode = nameAttribute.writeString(pipeline.getName());
  if(errorCode < 0)
  {
    return errorCode;
  }

  auto pipelineDatasetWriter = pipelineGroupWriter.createDatasetWriter(k_PipelineJsonTag);
  std::string pipelineString = pipeline.toJson().dump();
  return pipelineDatasetWriter.writeString(pipelineString);
}

complex::HDF5::ErrorType WriteDataStructure(complex::HDF5::FileWriter& fileWriter, const DataStructure& dataStructure)
{
  Result<> result = HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
  if(result.invalid())
  {
    return result.errors()[0].code;
  }
  return 0;
}

complex::HDF5::ErrorType WriteFileVersion(complex::HDF5::FileWriter& fileWriter)
{
  auto fileVersionAttribute = fileWriter.createAttribute(k_FileVersionTag);
  return fileVersionAttribute.writeString(k_CurrentFileVersion);
}

complex::HDF5::ErrorType DREAM3D::WriteFile(complex::HDF5::FileWriter& fileWriter, const FileData& fileData)
{
  return WriteFile(fileWriter, fileData.first, fileData.second);
}

complex::HDF5::ErrorType DREAM3D::WriteFile(complex::HDF5::FileWriter& fileWriter, const Pipeline& pipeline, const DataStructure& dataStructure)
{
  auto errorCode = WriteFileVersion(fileWriter);
  if(errorCode < 0)
  {
    return errorCode;
  }

  errorCode = WritePipeline(fileWriter, pipeline);
  if(errorCode < 0)
  {
    return errorCode;
  }
  errorCode = WriteDataStructure(fileWriter, dataStructure);
  return errorCode;
}

Result<> DREAM3D::WriteFile(const std::filesystem::path& path, const DataStructure& dataStructure, const Pipeline& pipeline, bool writeXdmf)
{
  auto fileWriterResult = complex::HDF5::FileWriter::CreateFile(path);
  if(fileWriterResult.invalid())
  {
    return ConvertResult(std::move(fileWriterResult));
  }

  complex::HDF5::FileWriter fileWriter = std::move(fileWriterResult.value());

  complex::HDF5::ErrorType error = WriteFile(fileWriter, pipeline, dataStructure);
  if(error < 0)
  {
    return MakeErrorResult(error, fmt::format("DREAM3D::WriteFile: Unable to write DREAM3D file with HDF5 error"));
  }

  if(writeXdmf)
  {
    std::filesystem::path xdmfFilePath = std::filesystem::path(path).replace_extension(".xdmf");
    WriteXdmf(xdmfFilePath, dataStructure, path.filename().string());
  }

  return {};
}
