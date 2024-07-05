#include "Dream3dIO.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/EmptyDataStore.hpp"
#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/HexahedralGeom.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/QuadGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/DataStructure/Geometry/TetrahedralGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureReader.hpp"
#include "simplnx/DataStructure/IO/HDF5/DataStructureWriter.hpp"
#include "simplnx/DataStructure/IO/HDF5/NeighborListIO.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Utilities/Parsing/HDF5/Readers/FileReader.hpp"
#include "simplnx/Utilities/Parsing/HDF5/Writers/AttributeWriter.hpp"
#include "simplnx/Utilities/Parsing/HDF5/Writers/FileWriter.hpp"

#include <nlohmann/json.hpp>

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>

using namespace nx::core;

namespace
{
constexpr StringLiteral k_DataStructureGroupTag = "DataStructure";
constexpr StringLiteral k_LegacyDataStructureGroupTag = "DataContainers";
constexpr StringLiteral k_FileVersionTag = "FileVersion";
constexpr StringLiteral k_PipelineJsonTag = "Pipeline";
constexpr StringLiteral k_PipelineNameTag = "Current Pipeline";
constexpr StringLiteral k_PipelineVersionTag = "Pipeline Version";

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

constexpr int32 k_LegacyDataArrayH5_Code = -7890;
constexpr int32 k_FailedReadingCompDims_Code = -7891;
constexpr int32 k_FailedReadingTupleDims_Code = -7892;
constexpr int32 k_FailedReadingDataArrayData_Code = -7893;
constexpr int32 k_FailedCreatingArray_Code = -7894;
constexpr int32 k_FailedCreatingNeighborList_Code = -7895;

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

DREAM3D::FileVersionType DREAM3D::GetFileVersion(const std::filesystem::path& path)
{
  HDF5::FileReader fileReader(path);
  return GetFileVersion(fileReader);
}

DREAM3D::FileVersionType DREAM3D::GetFileVersion(const nx::core::HDF5::FileReader& fileReader)
{
  auto fileVersionAttribute = fileReader.getAttribute(k_FileVersionTag);
  if(!fileVersionAttribute.isValid())
  {
    return "";
  }
  return fileVersionAttribute.readAsString();
}

DREAM3D::PipelineVersionType DREAM3D::GetPipelineVersion(const nx::core::HDF5::FileReader& fileReader)
{
  auto pipelineGroup = fileReader.openGroup(k_PipelineJsonTag);
  auto pipelineVersionAttribute = pipelineGroup.getAttribute(k_PipelineVersionTag);
  if(!pipelineVersionAttribute.isValid())
  {
    return 0;
  }
  return pipelineVersionAttribute.readAsValue<PipelineVersionType>();
}

Result<DataStructure> ImportDataStructureV8(const nx::core::HDF5::FileReader& fileReader, bool preflight)
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
Result<IDataArray*> createLegacyDataArray(DataStructure& dataStructure, DataObject::IdType parentId, const HDF5::DatasetReader& dataArrayReader, const std::vector<usize>& tDims,
                                          const std::vector<usize>& cDims, bool preflight = false)
{
  using DataArrayType = DataArray<T>;
  using EmptyDataStoreType = EmptyDataStore<T>;

  const std::string daName = dataArrayReader.getName();
  DataArrayType* dataArray = nullptr;

  if(preflight)
  {
    dataArray = DataArrayType::template CreateWithStore<EmptyDataStoreType>(dataStructure, daName, tDims, cDims, parentId);
  }
  else
  {
    auto dataStore = std::make_unique<DataStore<T>>(tDims, cDims, static_cast<T>(0));

    Result<> result = dataArrayReader.readIntoSpan(dataStore->createSpan());
    if(result.invalid())
    {
      std::string ss = fmt::format("Error reading HDF5 Data set {}:\n\n{}", nx::core::HDF5::Support::GetObjectPath(dataArrayReader.getId()), result.errors()[0].message);
      return nx::core::MakeErrorResult<IDataArray*>(Legacy::k_FailedReadingDataArrayData_Code, ss);
    }
    // Insert the DataArray into the DataStructure
    dataArray = DataArray<T>::Create(dataStructure, daName, std::move(dataStore), parentId);
  }

  if(nullptr == dataArray)
  {
    std::string ss = fmt::format("Failed to create DataArray: '{}'", daName);
    return nx::core::MakeErrorResult<IDataArray*>(Legacy::k_FailedCreatingArray_Code, ss);
  }

  return {dataArray};
}

/**
 * @brief
 * @param daId
 * @param tDims
 * @param cDims
 */
Result<> readLegacyDataArrayDims(const nx::core::HDF5::DatasetReader& dataArrayReader, std::vector<usize>& tDims, std::vector<usize>& cDims)
{
  {
    auto compAttrib = dataArrayReader.getAttribute(Legacy::CompDims);
    if(!compAttrib.isValid())
    {
      std::string ss = fmt::format("Failed to read component dimension for DataArray: '{}'", dataArrayReader.getName());
      return nx::core::MakeWarningVoidResult(Legacy::k_FailedReadingCompDims_Code, ss);
    }
    // From SIMPL, the component dimensions are actually all ordered correctly in the HDF5 file. Somehow.
    cDims = compAttrib.readAsVector<usize>();
  }

  {
    auto tupleAttrib = dataArrayReader.getAttribute(Legacy::TupleDims);
    if(!tupleAttrib.isValid())
    {
      std::string ss = fmt::format("Failed to read tuple dimension for DataArray: '{}'", dataArrayReader.getName());
      return nx::core::MakeWarningVoidResult(Legacy::k_FailedReadingTupleDims_Code, ss);
    }
    tDims = tupleAttrib.readAsVector<usize>();
    // std::reverse(tDims.begin(), tDims.end()); // SIMPL writes the Tuple Dimensions in reverse order to this attribute
  }

  return {};
}

Result<> readLegacyStringArray(DataStructure& dataStructure, const nx::core::HDF5::DatasetReader& dataArrayReader, DataObject::IdType parentId, bool preflight = false)
{
  const std::string daName = dataArrayReader.getName();

  if(preflight)
  {
    std::vector<usize> tDims;
    std::vector<usize> cDims;
    auto result = readLegacyDataArrayDims(dataArrayReader, tDims, cDims);
    if(result.invalid())
    {
      return result;
    }

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
  return {};
}

/**
 * @brief
 * @param dataStructure
 * @param dataArrayReader
 * @param parentId
 * @param amTDims
 * @param preflight
 * @return
 */
Result<IDataArray*> readLegacyDataArray(DataStructure& dataStructure, const nx::core::HDF5::DatasetReader& dataArrayReader, DataObject::IdType parentId, std::vector<uint64>& amTDims,
                                        bool preflight = false)
{
  auto size = H5Dget_storage_size(dataArrayReader.getId());
  auto typeId = dataArrayReader.getTypeId();

  std::vector<usize> tDims;
  std::vector<usize> cDims;
  Result<> dimsResult = readLegacyDataArrayDims(dataArrayReader, tDims, cDims);
  if(dimsResult.invalid())
  {
    auto& error = dimsResult.errors()[0];
    return MakeErrorResult<IDataArray*>(error.code, error.message);
  }

  // Let's do some sanity checking of the tuple dims and the comp dims
  // This is here because some 3rd party apps are trying to write .dream3d
  // files but are getting confused about the Tuple Dims and the component
  // dims. This bit of code will try to fix the specific issue where the
  // .dream3d file essentially had _all_ the dimensions shoved in the
  // tuple dimensions.
  if(!amTDims.empty()) // If the amTDims are empty we are just reading a Data Array without an Attribute Matrix so skip.
  {
    //
    if(tDims.size() > amTDims.size() && cDims.size() == 1 && cDims[0] == 1)
    {
      cDims.clear();
      // Find the index of the first non-matching dimension value.
      for(size_t idx = 0; idx < tDims.size(); idx++)
      {
        // If we are past the end of the AM Dimensions, then copy the value to the component dimensions
        if(idx >= amTDims.size())
        {
          cDims.push_back(tDims[idx]);
        }
        else if(amTDims[idx] != tDims[idx]) // something went wrong here. Each vector should have the same exact values at the start of the vector
        {
          // This should not happen at all. If so, bail out now.
          cDims.clear(); // just put things back the way they were and fail.
          cDims.push_back(1ULL);
          break; // The first parts of the dimensions should MATCH for this algorithm to work.
        }
      }
      tDims.resize(amTDims.size());
    }
  }

  Result<IDataArray*> daResult;
  if(H5Tequal(typeId, H5T_NATIVE_FLOAT) > 0)
  {
    daResult = createLegacyDataArray<float32>(dataStructure, parentId, dataArrayReader, tDims, cDims, preflight);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_DOUBLE) > 0)
  {
    daResult = createLegacyDataArray<float64>(dataStructure, parentId, dataArrayReader, tDims, cDims, preflight);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_INT8) > 0)
  {
    daResult = createLegacyDataArray<int8>(dataStructure, parentId, dataArrayReader, tDims, cDims, preflight);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_INT16) > 0)
  {
    daResult = createLegacyDataArray<int16>(dataStructure, parentId, dataArrayReader, tDims, cDims, preflight);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_INT32) > 0)
  {
    daResult = createLegacyDataArray<int32>(dataStructure, parentId, dataArrayReader, tDims, cDims, preflight);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_INT64) > 0)
  {
    daResult = createLegacyDataArray<int64>(dataStructure, parentId, dataArrayReader, tDims, cDims, preflight);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_UINT8) > 0)
  {
    const auto typeAttrib = dataArrayReader.getAttribute(Constants::k_ObjectTypeTag);
    if(typeAttrib.isValid() && typeAttrib.readAsString() == "DataArray<bool>")
    {
      daResult = createLegacyDataArray<bool>(dataStructure, parentId, dataArrayReader, tDims, cDims, preflight);
    }
    else
    {
      daResult = createLegacyDataArray<uint8>(dataStructure, parentId, dataArrayReader, tDims, cDims, preflight);
    }
  }
  else if(H5Tequal(typeId, H5T_NATIVE_UINT16) > 0)
  {
    daResult = createLegacyDataArray<uint16>(dataStructure, parentId, dataArrayReader, tDims, cDims, preflight);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_UINT32) > 0)
  {
    daResult = createLegacyDataArray<uint32>(dataStructure, parentId, dataArrayReader, tDims, cDims, preflight);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_UINT64) > 0)
  {
    daResult = createLegacyDataArray<uint64>(dataStructure, parentId, dataArrayReader, tDims, cDims, preflight);
  }

  H5Tclose(typeId);

  return daResult;
}

Result<UInt64Array*> readLegacyNodeConnectivityList(DataStructure& dataStructure, IGeometry* geometry, const HDF5::GroupReader& geomGroup, const std::string& arrayName, bool preflight = false)
{
  HDF5::DatasetReader dataArrayReader = geomGroup.openDataset(arrayName);
  DataObject::IdType parentId = geometry->getId();

  auto size = H5Dget_storage_size(dataArrayReader.getId());

  std::vector<usize> tDims;
  std::vector<usize> cDims;
  Result<> result = readLegacyDataArrayDims(dataArrayReader, tDims, cDims);
  if(result.invalid())
  {
    auto& error = result.errors()[0];
    return MakeErrorResult<UInt64Array*>(error.code, error.message);
  }

  auto daResult = createLegacyDataArray<uint64>(dataStructure, parentId, dataArrayReader, tDims, cDims, preflight);
  if(daResult.invalid())
  {
    auto& error = daResult.errors()[0];
    return MakeErrorResult<UInt64Array*>(error.code, error.message);
  }
  auto* value = dynamic_cast<UInt64Array*>(daResult.value());
  auto voidResult = ConvertResult(std::move(daResult));
  return ConvertResultTo<UInt64Array*>(std::move(voidResult), std::move(value));
}

template <typename T>
Result<> createLegacyNeighborList(DataStructure& dataStructure, DataObject ::IdType parentId, const nx::core::HDF5::GroupReader& parentReader, const nx::core::HDF5::DatasetReader& datasetReader,
                                  const std::vector<usize>& tupleDims)
{
  auto numTuples = std::accumulate(tupleDims.cbegin(), tupleDims.cend(), static_cast<usize>(1), std::multiplies<>());

  auto data = HDF5::NeighborListIO<T>::ReadHdf5Data(parentReader, datasetReader);
  auto* neighborList = NeighborList<T>::Create(dataStructure, datasetReader.getName(), numTuples, parentId);
  if(neighborList == nullptr)
  {
    std::string ss = fmt::format("Failed to create NeighborList: '{}'", datasetReader.getName());
    return MakeErrorResult(Legacy::k_FailedCreatingNeighborList_Code, ss);
  }
  for(usize i = 0; i < data.size(); ++i)
  {
    neighborList->setList(i, data[i]);
  }
  return {};
}

Result<> readLegacyNeighborList(DataStructure& dataStructure, const nx::core::HDF5::GroupReader& parentReader, const nx::core::HDF5::DatasetReader& datasetReader, DataObject::IdType parentId)
{
  auto typeId = datasetReader.getTypeId();

  auto tupleAttrib = datasetReader.getAttribute(Legacy::TupleDims);
  if(!tupleAttrib.isValid())
  {
    std::string ss = fmt::format("Failed to read tuple dimensions for NeighbhorList: '{}'", datasetReader.getName());
    return nx::core::MakeWarningVoidResult(Legacy::k_FailedReadingTupleDims_Code, ss);
  }

  auto tDims = tupleAttrib.readAsVector<usize>();
  Result<> result;

  if(H5Tequal(typeId, H5T_NATIVE_FLOAT) > 0)
  {
    result = createLegacyNeighborList<float32>(dataStructure, parentId, parentReader, datasetReader, tDims);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_DOUBLE) > 0)
  {
    result = createLegacyNeighborList<float64>(dataStructure, parentId, parentReader, datasetReader, tDims);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_INT8) > 0)
  {
    result = createLegacyNeighborList<int8>(dataStructure, parentId, parentReader, datasetReader, tDims);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_INT16) > 0)
  {
    result = createLegacyNeighborList<int16>(dataStructure, parentId, parentReader, datasetReader, tDims);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_INT32) > 0)
  {
    result = createLegacyNeighborList<int32>(dataStructure, parentId, parentReader, datasetReader, tDims);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_INT64) > 0)
  {
    result = createLegacyNeighborList<int64>(dataStructure, parentId, parentReader, datasetReader, tDims);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_UINT8) > 0)
  {
    result = createLegacyNeighborList<uint8>(dataStructure, parentId, parentReader, datasetReader, tDims);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_UINT16) > 0)
  {
    result = createLegacyNeighborList<uint16>(dataStructure, parentId, parentReader, datasetReader, tDims);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_UINT32) > 0)
  {
    result = createLegacyNeighborList<uint32>(dataStructure, parentId, parentReader, datasetReader, tDims);
  }
  else if(H5Tequal(typeId, H5T_NATIVE_UINT64) > 0)
  {
    result = createLegacyNeighborList<uint64>(dataStructure, parentId, parentReader, datasetReader, tDims);
  }

  H5Tclose(typeId);

  return result;
}

bool isLegacyNeighborList(const nx::core::HDF5::DatasetReader& arrayReader)
{
  const auto objectTypeAttribute = arrayReader.getAttribute("ObjectType");
  const std::string objectType = objectTypeAttribute.readAsString();
  return objectType == "NeighborList<T>";
}

bool isLegacyStringArray(const nx::core::HDF5::DatasetReader& arrayReader)
{
  const auto objectTypeAttribute = arrayReader.getAttribute("ObjectType");
  const std::string objectType = objectTypeAttribute.readAsString();
  return objectType == "StringDataArray";
}

Result<> readLegacyAttributeMatrix(DataStructure& dataStructure, const nx::core::HDF5::GroupReader& amGroupReader, DataObject& parent, bool preflight = false)
{
  DataObject::IdType parentId = parent.getId();
  const std::string amName = amGroupReader.getName();

  auto tDimsReader = amGroupReader.getAttribute("TupleDimensions");
  auto tDims = tDimsReader.readAsVector<uint64>();
  auto reversedTDims = AttributeMatrix::ShapeType(tDims.crbegin(), tDims.crend());

  auto* attributeMatrix = AttributeMatrix::Create(dataStructure, amName, reversedTDims, parentId);

  std::vector<Result<>> daResults;
  auto dataArrayNames = amGroupReader.getChildNames();
  for(const auto& daName : dataArrayNames)
  {
    auto dataArraySet = amGroupReader.openDataset(daName);
    if(!dataArraySet.isValid())
    {
      // Could not open HDF5 DataSet. Could be stats array
      std::string ss = fmt::format("Could not open array '{}'", daName);
      daResults.push_back(nx::core::MakeWarningVoidResult(Legacy::k_LegacyDataArrayH5_Code, ss));
      continue;
    }

    if(isLegacyNeighborList(dataArraySet))
    {
      daResults.push_back(readLegacyNeighborList(dataStructure, amGroupReader, dataArraySet, attributeMatrix->getId()));
    }
    else if(isLegacyStringArray(dataArraySet))
    {
      daResults.push_back(readLegacyStringArray(dataStructure, dataArraySet, attributeMatrix->getId(), preflight));
    }
    else
    {
      Result<> result = ConvertResult(std::move(readLegacyDataArray(dataStructure, dataArraySet, attributeMatrix->getId(), tDims, preflight)));
      daResults.push_back(result);
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
  return MergeResults(daResults);
}

// Begin legacy geometry import methods
void readGenericGeomDims(IGeometry* geom, const nx::core::HDF5::GroupReader& geomGroup)
{
  auto sDimAttribute = geomGroup.getAttribute("SpatialDimensionality");
  auto sDims = sDimAttribute.readAsValue<int32>();

  auto uDimAttribute = geomGroup.getAttribute("UnitDimensionality");
  auto uDims = uDimAttribute.readAsValue<int32>();

  geom->setSpatialDimensionality(sDims);
  geom->setUnitDimensionality(uDims);
}

Result<IDataArray*> readLegacyGeomArray(DataStructure& dataStructure, IGeometry* geometry, const nx::core::HDF5::GroupReader& geomGroup, const std::string& arrayName, bool preflight)
{
  auto dataArraySet = geomGroup.openDataset(arrayName);
  std::vector<uint64> tDims;
  return readLegacyDataArray(dataStructure, dataArraySet, geometry->getId(), tDims, preflight);
}

template <typename T>
Result<T*> readLegacyGeomArrayAs(DataStructure& dataStructure, IGeometry* geometry, const nx::core::HDF5::GroupReader& geomGroup, const std::string& arrayName, bool preflight)
{
  Result<IDataArray*> result = readLegacyGeomArray(dataStructure, geometry, geomGroup, arrayName, preflight);
  if(result.invalid())
  {
    auto& error = result.errors()[0];
    return nx::core::MakeErrorResult<T*>(error.code, error.message);
  }

  IDataArray* iArray = result.value();
  T* dataArray = dynamic_cast<T*>(iArray);
  Result<> voidResult = ConvertResult(std::move(result));
  return ConvertResultTo<T*>(std::move(voidResult), std::move(dataArray));
}

DataObject* readLegacyVertexGeom(DataStructure& dataStructure, const nx::core::HDF5::GroupReader& geomGroup, const std::string& name, bool preflight)
{
  auto* geom = VertexGeom::Create(dataStructure, name);
  readGenericGeomDims(geom, geomGroup);
  Result<Float32Array*> sharedVertexList = readLegacyGeomArrayAs<Float32Array>(dataStructure, geom, geomGroup, Legacy::VertexListName, preflight);

  geom->setVertices(*sharedVertexList.value());
  return geom;
}

DataObject* readLegacyTriangleGeom(DataStructure& dataStructure, const nx::core::HDF5::GroupReader& geomGroup, const std::string& name, bool preflight)
{
  auto geom = TriangleGeom::Create(dataStructure, name);
  readGenericGeomDims(geom, geomGroup);
  auto sharedVertexList = readLegacyGeomArrayAs<Float32Array>(dataStructure, geom, geomGroup, Legacy::VertexListName, preflight);
  auto sharedTriList = readLegacyNodeConnectivityList(dataStructure, geom, geomGroup, Legacy::TriListName, preflight);

  geom->setVertices(*sharedVertexList.value());
  geom->setFaceList(*sharedTriList.value());

  return geom;
}

DataObject* readLegacyTetrahedralGeom(DataStructure& dataStructure, const nx::core::HDF5::GroupReader& geomGroup, const std::string& name, bool preflight)
{
  auto geom = TetrahedralGeom::Create(dataStructure, name);
  readGenericGeomDims(geom, geomGroup);
  auto sharedVertexList = readLegacyGeomArrayAs<Float32Array>(dataStructure, geom, geomGroup, Legacy::VertexListName, preflight);
  auto sharedTetList = readLegacyNodeConnectivityList(dataStructure, geom, geomGroup, Legacy::TetraListName, preflight);

  geom->setVertices(*sharedVertexList.value());
  geom->setPolyhedraList(*sharedTetList.value());

  return geom;
}

DataObject* readLegacyRectGridGeom(DataStructure& dataStructure, const nx::core::HDF5::GroupReader& geomGroup, const std::string& name, bool preflight)
{
  auto geom = RectGridGeom::Create(dataStructure, name);
  readGenericGeomDims(geom, geomGroup);

  // DIMENSIONS array
  {
    auto dimsDataset = geomGroup.openDataset("DIMENSIONS");
    auto dims = dimsDataset.readAsVector<int64>();
    geom->setDimensions(SizeVec3(dims[0], dims[1], dims[2]));
  }

  auto xBoundsArray = readLegacyGeomArrayAs<Float32Array>(dataStructure, geom, geomGroup, Legacy::XBoundsName, preflight);
  auto yBoundsArray = readLegacyGeomArrayAs<Float32Array>(dataStructure, geom, geomGroup, Legacy::YBoundsName, preflight);
  auto zBoundsArray = readLegacyGeomArrayAs<Float32Array>(dataStructure, geom, geomGroup, Legacy::ZBoundsName, preflight);

  geom->setBounds(xBoundsArray.value(), yBoundsArray.value(), zBoundsArray.value());

  return geom;
}

DataObject* readLegacyQuadGeom(DataStructure& dataStructure, const nx::core::HDF5::GroupReader& geomGroup, const std::string& name, bool preflight)
{
  auto geom = QuadGeom::Create(dataStructure, name);
  readGenericGeomDims(geom, geomGroup);
  auto sharedVertexList = readLegacyGeomArrayAs<Float32Array>(dataStructure, geom, geomGroup, Legacy::VertexListName, preflight);
  auto sharedQuadList = readLegacyNodeConnectivityList(dataStructure, geom, geomGroup, Legacy::QuadListName, preflight);

  geom->setVertices(*sharedVertexList.value());
  geom->setFaceList(*sharedQuadList.value());

  return geom;
}

DataObject* readLegacyHexGeom(DataStructure& dataStructure, const nx::core::HDF5::GroupReader& geomGroup, const std::string& name, bool preflight)
{
  auto geom = HexahedralGeom::Create(dataStructure, name);
  readGenericGeomDims(geom, geomGroup);
  auto sharedVertexList = readLegacyGeomArrayAs<Float32Array>(dataStructure, geom, geomGroup, Legacy::VertexListName, preflight);
  auto sharedHexList = readLegacyNodeConnectivityList(dataStructure, geom, geomGroup, Legacy::HexListName, preflight);

  geom->setVertices(*sharedVertexList.value());
  geom->setPolyhedraList(*sharedHexList.value());

  return geom;
}

DataObject* readLegacyEdgeGeom(DataStructure& dataStructure, const nx::core::HDF5::GroupReader& geomGroup, const std::string& name, bool preflight)
{
  auto geom = EdgeGeom::Create(dataStructure, name);
  auto edge = dynamic_cast<EdgeGeom*>(geom);
  readGenericGeomDims(geom, geomGroup);
  auto sharedVertexList = readLegacyGeomArrayAs<Float32Array>(dataStructure, geom, geomGroup, Legacy::VertexListName, preflight);
  auto sharedEdgeList = readLegacyNodeConnectivityList(dataStructure, geom, geomGroup, Legacy::EdgeListName, preflight);

  geom->setVertices(*sharedVertexList.value());
  geom->setEdgeList(*sharedEdgeList.value());

  return geom;
}

DataObject* readLegacyImageGeom(DataStructure& dataStructure, const nx::core::HDF5::GroupReader& geomGroup, const std::string& name)
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

Result<> readLegacyDataContainer(DataStructure& dataStructure, const nx::core::HDF5::GroupReader& dcGroup, bool preflight = false)
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
      container = readLegacyEdgeGeom(dataStructure, geomGroup, dcName, preflight);
    }
    else if(geomName == Legacy::Type::HexGeom)
    {
      container = readLegacyHexGeom(dataStructure, geomGroup, dcName, preflight);
    }
    else if(geomName == Legacy::Type::QuadGeom)
    {
      container = readLegacyQuadGeom(dataStructure, geomGroup, dcName, preflight);
    }
    else if(geomName == Legacy::Type::RectGridGeom)
    {
      container = readLegacyRectGridGeom(dataStructure, geomGroup, dcName, preflight);
    }
    else if(geomName == Legacy::Type::TetrahedralGeom)
    {
      container = readLegacyTetrahedralGeom(dataStructure, geomGroup, dcName, preflight);
    }
    else if(geomName == Legacy::Type::TriangleGeom)
    {
      container = readLegacyTriangleGeom(dataStructure, geomGroup, dcName, preflight);
    }
    else if(geomName == Legacy::Type::VertexGeom)
    {
      container = readLegacyVertexGeom(dataStructure, geomGroup, dcName, preflight);
    }
  }

  // No geometry found. Create a DataGroup instead
  if(!container)
  {
    container = DataGroup::Create(dataStructure, dcName);
  }

  std::vector<Result<>> amResults;
  auto attribMatrixNames = dcGroup.getChildNames();
  for(const auto& amName : attribMatrixNames)
  {
    if(amName == Legacy::GeometryTag)
    {
      continue;
    }

    auto attributeMatrixGroup = dcGroup.openGroup(amName);
    amResults.push_back(readLegacyAttributeMatrix(dataStructure, attributeMatrixGroup, *container, preflight));
  }
  return nx::core::MergeResults(amResults);
}

Result<DataStructure> ImportLegacyDataStructure(const nx::core::HDF5::FileReader& fileReader, bool preflight)
{
  DataStructure dataStructure;

  auto dcaGroup = fileReader.openGroup(k_LegacyDataStructureGroupTag);

  // Iterate over DataContainers
  std::vector<Result<>> importResults;
  const auto dcNames = dcaGroup.getChildNames();
  for(const auto& dcName : dcNames)
  {
    auto dcGroup = dcaGroup.openGroup(dcName);
    importResults.push_back(readLegacyDataContainer(dataStructure, dcGroup, preflight));
  }

  auto result = nx::core::MergeResults(importResults);
  return nx::core::ConvertResultTo<DataStructure>(std::move(result), std::move(dataStructure));
}

Result<DataStructure> DREAM3D::ImportDataStructureFromFile(const nx::core::HDF5::FileReader& fileReader, bool preflight)
{
  const auto fileVersion = GetFileVersion(fileReader);
  if(fileVersion == k_CurrentFileVersion)
  {
    return ImportDataStructureV8(fileReader, preflight);
  }
  else if(fileVersion == k_LegacyFileVersion)
  {
    return ImportLegacyDataStructure(fileReader, preflight);
  }
  // Unsupported file version
  return MakeErrorResult<DataStructure>(k_InvalidDataStructureVersion,
                                        fmt::format("Could not parse DataStructure version {}. Expected versions: {} or {}", fileVersion, k_CurrentFileVersion, k_LegacyFileVersion));
}

Result<DataStructure> DREAM3D::ImportDataStructureFromFile(const std::filesystem::path& filePath, bool preflight)
{
  nx::core::HDF5::FileReader fileReader(filePath);
  if(!fileReader.isValid())
  {
    return MakeErrorResult<DataStructure>(-1, fmt::format("DREAM3D::ImportDataStructureFromFile: Unable to open '{}' for reading", filePath.string()));
  }

  return ImportDataStructureFromFile(fileReader, preflight);
}

Result<Pipeline> DREAM3D::ImportPipelineFromFile(const nx::core::HDF5::FileReader& fileReader)
{
  Result<nlohmann::json> pipelineJson = ImportPipelineJsonFromFile(fileReader);
  if(pipelineJson.invalid())
  {
    return ConvertInvalidResult<Pipeline, nlohmann::json>(std::move(pipelineJson));
  }
  const auto fileVersion = GetFileVersion(fileReader);
  if(fileVersion == k_CurrentFileVersion)
  {
    if(GetPipelineVersion(fileReader) != k_CurrentPipelineVersion)
    {
      return MakeErrorResult<Pipeline>(k_InvalidPipelineVersion, fmt::format("Could not parse Pipeline version '{}'. Expected version: '{}'", GetPipelineVersion(fileReader), k_CurrentFileVersion));
    }
    return Pipeline::FromJson(pipelineJson.value());
  }
  if(fileVersion == k_LegacyFileVersion)
  {
    return Pipeline::FromSIMPLJson(pipelineJson.value());
  }
  return MakeErrorResult<Pipeline>(k_InvalidPipelineVersion, fmt::format("Could not parse file version '{}'", k_CurrentFileVersion));
}

Result<nlohmann::json> DREAM3D::ImportPipelineJsonFromFile(const nx::core::HDF5::FileReader& fileReader)
{
  auto pipelineGroupReader = fileReader.openGroup(k_PipelineJsonTag);
  auto pipelineDatasetReader = pipelineGroupReader.openDataset(k_PipelineJsonTag);
  if(!pipelineDatasetReader.isValid())
  {
    return MakeErrorResult<nlohmann::json>(k_PipelineGroupUnavailable, "Could not open '/Pipeline' HDF5 Group.");
  }

  auto pipelineJsonString = pipelineDatasetReader.readAsString();
  auto pipelineJson = nlohmann::json::parse(pipelineJsonString);
  return {pipelineJson};
}

Result<Pipeline> DREAM3D::ImportPipelineFromFile(const std::filesystem::path& filePath)
{
  if(!std::filesystem::exists(filePath))
  {
    return MakeErrorResult<Pipeline>(-1, fmt::format("DREAM3D::ImportPipelineFromFile: File does not exist. '{}'", filePath.string()));
  }
  nx::core::HDF5::FileReader fileReader(filePath);
  if(!fileReader.isValid())
  {
    return MakeErrorResult<Pipeline>(-1, fmt::format("DREAM3D::ImportPipelineFromFile: Unable to open '{}' for reading", filePath.string()));
  }

  return ImportPipelineFromFile(fileReader);
}

Result<nlohmann::json> DREAM3D::ImportPipelineJsonFromFile(const std::filesystem::path& filePath)
{
  if(!std::filesystem::exists(filePath))
  {
    return MakeErrorResult<nlohmann::json>(-1, fmt::format("DREAM3D::ImportPipelineFromFile: File does not exist. '{}'", filePath.string()));
  }
  nx::core::HDF5::FileReader fileReader(filePath);
  if(!fileReader.isValid())
  {
    return MakeErrorResult<nlohmann::json>(-1, fmt::format("DREAM3D::ImportPipelineFromFile: Unable to open '{}' for reading", filePath.string()));
  }

  return ImportPipelineJsonFromFile(fileReader);
}

Result<DREAM3D::FileData> DREAM3D::ReadFile(const nx::core::HDF5::FileReader& fileReader, bool preflight)
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
  nx::core::HDF5::FileReader reader(path);
  nx::core::HDF5::ErrorType error = 0;

  Result<FileData> fileData = ReadFile(reader, error);
  if(error < 0)
  {
    return MakeErrorResult<FileData>(-1, fmt::format("DREAM3D::ReadFile: Unable to read '{}'", path.string()));
  }
  return {std::move(fileData)};
}

Result<> WritePipeline(nx::core::HDF5::FileWriter& fileWriter, const Pipeline& pipeline)
{
  if(!fileWriter.isValid())
  {
    return MakeErrorResult(-100, "Cannot Write to Invalid FileWriter");
  }

  auto pipelineGroupWriter = fileWriter.createGroupWriter(k_PipelineJsonTag);
  auto versionAttribute = pipelineGroupWriter.createAttribute(k_PipelineVersionTag);
  auto result = versionAttribute.writeValue<DREAM3D::PipelineVersionType>(k_CurrentPipelineVersion);
  if(result.invalid())
  {
    return result;
  }

  auto nameAttribute = pipelineGroupWriter.createAttribute(k_PipelineNameTag);
  result = nameAttribute.writeString(pipeline.getName());
  if(result.invalid())
  {
    return result;
  }

  auto pipelineDatasetWriter = pipelineGroupWriter.createDatasetWriter(k_PipelineJsonTag);
  std::string pipelineString = pipeline.toJson().dump();
  return pipelineDatasetWriter.writeString(pipelineString);
}

Result<> WriteDataStructure(nx::core::HDF5::FileWriter& fileWriter, const DataStructure& dataStructure)
{
  return HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter);
}

Result<> WriteFileVersion(nx::core::HDF5::FileWriter& fileWriter)
{
  auto fileVersionAttribute = fileWriter.createAttribute(k_FileVersionTag);
  return fileVersionAttribute.writeString(DREAM3D::k_CurrentFileVersion);
}

Result<> DREAM3D::WriteFile(nx::core::HDF5::FileWriter& fileWriter, const FileData& fileData)
{
  return WriteFile(fileWriter, fileData.first, fileData.second);
}

Result<> DREAM3D::WriteFile(nx::core::HDF5::FileWriter& fileWriter, const Pipeline& pipeline, const DataStructure& dataStructure)
{
  auto result = WriteFileVersion(fileWriter);
  if(result.invalid())
  {
    return result;
  }

  result = WritePipeline(fileWriter, pipeline);
  if(result.invalid())
  {
    return result;
  }
  return WriteDataStructure(fileWriter, dataStructure);
}

Result<> DREAM3D::WriteFile(const std::filesystem::path& path, const DataStructure& dataStructure, const Pipeline& pipeline, bool writeXdmf)
{
  auto fileWriterResult = nx::core::HDF5::FileWriter::CreateFile(path);
  if(fileWriterResult.invalid())
  {
    return ConvertResult(std::move(fileWriterResult));
  }

  nx::core::HDF5::FileWriter fileWriter = std::move(fileWriterResult.value());

  auto result = WriteFile(fileWriter, pipeline, dataStructure);
  if(result.invalid())
  {
    return MakeErrorResult(result.errors()[0].code, fmt::format("DREAM3D::WriteFile: Unable to write DREAM3D file with HDF5 error"));
  }

  if(writeXdmf)
  {
    std::filesystem::path xdmfFilePath = std::filesystem::path(path).replace_extension(".xdmf");
    WriteXdmf(xdmfFilePath, dataStructure, path.filename().string());
  }

  return {};
}
