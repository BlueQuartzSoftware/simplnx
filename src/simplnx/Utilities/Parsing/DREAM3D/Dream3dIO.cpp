#include "Dream3dIO.hpp"

#include "simplnx/Common/Aliases.hpp"
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
#include "simplnx/DataStructure/IO/HDF5/IDataStoreIO.hpp"
#include "simplnx/DataStructure/IO/HDF5/NeighborListIO.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/DataStructure/StringStore.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"

#include <nlohmann/json.hpp>

#include <fstream>
#include <set>
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

  out << "  <!-- *************** START OF " << name << " *************** -->" << "\n";
  out << "  <Grid Name=\"" << name << R"(" GridType="Uniform">)" << "\n";
  out << R"(    <Topology TopologyType="3DCoRectMesh" Dimensions=")" << volDims[2] + 1 << " " << volDims[1] + 1 << " " << volDims[0] + 1 << " \"></Topology>" << "\n";
  out << "    <Geometry Type=\"ORIGIN_DXDYDZ\">" << "\n";
  out << "      <!-- Origin  Z, Y, X -->" << "\n";
  out << R"(      <DataItem Format="XML" Dimensions="3">)" << origin[2] << " " << origin[1] << " " << origin[0] << "</DataItem>" << "\n";
  out << "      <!-- DxDyDz (Spacing/Spacing) Z, Y, X -->" << "\n";
  out << R"(      <DataItem Format="XML" Dimensions="3">)" << spacing[2] << " " << spacing[1] << " " << spacing[0] << "</DataItem>" << "\n";
  out << "    </Geometry>" << "\n";
}

void WriteGeomXdmf(std::ostream& out, const RectGridGeom& rectGridGeom, std::string_view hdf5FilePath)
{
  std::string name = rectGridGeom.getName();

  SizeVec3 dims = rectGridGeom.getDimensions();
  const Float32Array* xBounds = rectGridGeom.getXBounds();
  const Float32Array* yBounds = rectGridGeom.getYBounds();
  const Float32Array* zBounds = rectGridGeom.getZBounds();
  if(xBounds == nullptr || yBounds == nullptr || zBounds == nullptr)
  {
    return;
  }
  DataPath xBoundsPath = xBounds->getDataPaths().at(0);
  DataPath yBoundsPath = yBounds->getDataPaths().at(0);
  DataPath zBoundsPath = zBounds->getDataPaths().at(0);

  std::array<int64, 3> volDims = {static_cast<int64>(dims.getX()), static_cast<int64>(dims.getY()), static_cast<int64>(dims.getZ())};

  out << "  <!-- *************** START OF " << name << " *************** -->" << "\n";
  out << "  <Grid Name=\"" << name << R"(" GridType="Uniform">)" << "\n";
  out << "    <Topology TopologyType=\"3DRectMesh\" Dimensions=\"" << volDims[2] + 1 << " " << volDims[1] + 1 << " " << volDims[0] + 1 << " \"></Topology>" << "\n";
  out << "    <Geometry Type=\"VxVyVz\">" << "\n";
  out << "    <DataItem Format=\"HDF\" Dimensions=\"" << xBounds->getNumberOfTuples() << "\" NumberType=\"Float\" Precision=\"4\">" << "\n";
  out << "      " << hdf5FilePath << ":/DataStructure/" << xBoundsPath.toString() << "\n";
  out << "    </DataItem>" << "\n";
  out << "    <DataItem Format=\"HDF\" Dimensions=\"" << yBounds->getNumberOfTuples() << "\" NumberType=\"Float\" Precision=\"4\">" << "\n";
  out << "      " << hdf5FilePath << ":/DataStructure/" << yBoundsPath.toString() << "\n";
  out << "    </DataItem>" << "\n";
  out << "    <DataItem Format=\"HDF\" Dimensions=\"" << zBounds->getNumberOfTuples() << "\" NumberType=\"Float\" Precision=\"4\">" << "\n";
  out << "      " << hdf5FilePath << ":/DataStructure/" << zBoundsPath.toString() << "\n";
  out << "    </DataItem>" << "\n";
  out << "    </Geometry>" << "\n";
}

void WriteGeomXdmf(std::ostream& out, const VertexGeom& vertexGeom, std::string_view hdf5FilePath)
{
  std::string name = vertexGeom.getName();
  usize numVerts = vertexGeom.getNumberOfVertices();
  if(numVerts == 0)
  {
    return;
  }
  DataPath verticesPath = vertexGeom.getVerticesRef().getDataPaths().at(0);

  DataPath geomPath = vertexGeom.getDataPaths().at(0);

  out << "  <!-- *************** START OF " << name << " *************** -->" << "\n";
  out << "  <Grid Name=\"" << name << R"(" GridType="Uniform">)" << "\n";

  out << R"(    <Topology TopologyType="Polyvertex" NumberOfElements=")" << numVerts << "\">" << "\n";
  out << R"(      <DataItem Format="HDF" NumberType="Int" Dimensions=")" << numVerts << "\">" << "\n";
  out << "        " << hdf5FilePath << ":/DataStructure/" << geomPath.toString() << "/_VertexIndices" << "\n";
  out << "      </DataItem>" << "\n";
  out << "    </Topology>" << "\n";

  out << "    <Geometry Type=\"XYZ\">" << "\n";
  out << R"(      <DataItem Format="HDF"  Dimensions=")" << numVerts << R"( 3" NumberType="Float" Precision="4">)" << "\n";
  out << "        " << hdf5FilePath << ":/DataStructure/" << verticesPath.toString() << "\n";
  out << "      </DataItem>" << "\n";
  out << "    </Geometry>" << "\n";
  out << "" << "\n";
}

void WriteGeomXdmf(std::ostream& out, const EdgeGeom& edgeGeom, std::string_view hdf5FilePath)
{
  std::string name = edgeGeom.getName();
  usize numEdges = edgeGeom.getNumberOfCells();
  usize numVerts = edgeGeom.getNumberOfVertices();
  if(numEdges == 0 || numVerts == 0)
  {
    return;
  }

  DataPath edgesPath = edgeGeom.getEdgesRef().getDataPaths().at(0);
  DataPath verticesPath = edgeGeom.getVerticesRef().getDataPaths().at(0);

  out << "  <!-- *************** START OF " << name << " *************** -->" << "\n";
  out << "  <Grid Name=\"" << name << "\" GridType=\"Uniform\">" << "\n";
  out << "    <Topology TopologyType=\"Polyline\" NodesPerElement=\"2\" NumberOfElements=\"" << numEdges << "\">" << "\n";
  out << "      <DataItem Format=\"HDF\" NumberType=\"Int\" Dimensions=\"" << numEdges << " 2\">" << "\n";
  out << "        " << hdf5FilePath << ":/DataStructure/" << edgesPath.toString() << "\n";
  out << "      </DataItem>" << "\n";
  out << "    </Topology>" << "\n";
  out << "    <Geometry Type=\"XYZ\">" << "\n";
  out << "      <DataItem Format=\"HDF\"  Dimensions=\"" << numVerts << " 3\" NumberType=\"Float\" Precision=\"4\">" << "\n";
  out << "        " << hdf5FilePath << ":/DataStructure/" << verticesPath.toString() << "\n";
  out << "      </DataItem>" << "\n";
  out << "    </Geometry>" << "\n";
  out << "" << "\n";
}

void WriteGeomXdmf(std::ostream& out, const TriangleGeom& triangleGeom, std::string_view hdf5FilePath)
{
  std::string name = triangleGeom.getName();
  usize numFaces = triangleGeom.getNumberOfFaces();
  usize numVerts = triangleGeom.getNumberOfVertices();
  if(numFaces == 0 || numVerts == 0)
  {
    return;
  }

  DataPath facesPath = triangleGeom.getFacesRef().getDataPaths().at(0);
  DataPath verticesPath = triangleGeom.getVerticesRef().getDataPaths().at(0);

  out << "  <!-- *************** START OF " << name << " *************** -->" << "\n";
  out << "  <Grid Name=\"" << name << "\" GridType=\"Uniform\">" << "\n";
  out << "    <Topology TopologyType=\"Triangle\" NumberOfElements=\"" << numFaces << "\">" << "\n";
  out << "      <DataItem Format=\"HDF\" NumberType=\"Int\" Dimensions=\"" << numFaces << " 3\">" << "\n";
  out << "        " << hdf5FilePath << ":/DataStructure/" << facesPath.toString() << "\n";
  out << "      </DataItem>" << "\n";
  out << "    </Topology>" << "\n";
  out << "    <Geometry Type=\"XYZ\">" << "\n";
  out << "      <DataItem Format=\"HDF\"  Dimensions=\"" << numVerts << " 3\" NumberType=\"Float\" Precision=\"4\">" << "\n";
  out << "        " << hdf5FilePath << ":/DataStructure/" << verticesPath.toString() << "\n";
  out << "      </DataItem>" << "\n";
  out << "    </Geometry>" << "\n";
  out << "" << "\n";
}

void WriteGeomXdmf(std::ostream& out, const QuadGeom& quadGeom, std::string_view hdf5FilePath)
{
  std::string name = quadGeom.getName();
  usize numFaces = quadGeom.getNumberOfFaces();
  usize numVerts = quadGeom.getNumberOfVertices();
  if(numFaces == 0 || numVerts == 0)
  {
    return;
  }
  DataPath facesPath = quadGeom.getFacesRef().getDataPaths().at(0);
  DataPath verticesPath = quadGeom.getVerticesRef().getDataPaths().at(0);

  out << "  <!-- *************** START OF " << name << " *************** -->" << "\n";
  out << "  <Grid Name=\"" << name << "\" GridType=\"Uniform\">" << "\n";
  out << "    <Topology TopologyType=\"Quadrilateral\" NumberOfElements=\"" << numFaces << "\">" << "\n";
  out << "      <DataItem Format=\"HDF\" NumberType=\"Int\" Dimensions=\"" << numFaces << " 4\">" << "\n";
  out << "        " << hdf5FilePath << ":/DataStructure/" << facesPath.toString() << "\n";
  out << "      </DataItem>" << "\n";
  out << "    </Topology>" << "\n";
  out << "    <Geometry Type=\"XYZ\">" << "\n";
  out << "      <DataItem Format=\"HDF\"  Dimensions=\"" << numVerts << " 3\" NumberType=\"Float\" Precision=\"4\">" << "\n";
  out << "        " << hdf5FilePath << ":/DataStructure/" << verticesPath.toString() << "\n";
  out << "      </DataItem>" << "\n";
  out << "    </Geometry>" << "\n";
  out << "" << "\n";
}

void WriteGeomXdmf(std::ostream& out, const TetrahedralGeom& tetrahedralGeom, std::string_view hdf5FilePath)
{
  std::string name = tetrahedralGeom.getName();
  usize numPolyhedra = tetrahedralGeom.getNumberOfPolyhedra();
  usize numVerts = tetrahedralGeom.getNumberOfVertices();
  if(numPolyhedra == 0 || numVerts == 0)
  {
    return;
  }
  DataPath polyhedraPath = tetrahedralGeom.getPolyhedraRef().getDataPaths().at(0);
  DataPath verticesPath = tetrahedralGeom.getVerticesRef().getDataPaths().at(0);

  out << "  <!-- *************** START OF " << name << " *************** -->" << "\n";
  out << "  <Grid Name=\"" << name << "\" GridType=\"Uniform\">" << "\n";
  out << "    <Topology TopologyType=\"Tetrahedron\" NumberOfElements=\"" << numPolyhedra << "\">" << "\n";
  out << "      <DataItem Format=\"HDF\" NumberType=\"Int\" Dimensions=\"" << numPolyhedra << " 4\">" << "\n";
  out << "        " << hdf5FilePath << ":/DataStructure/" << polyhedraPath.toString() << "\n";
  out << "      </DataItem>" << "\n";
  out << "    </Topology>" << "\n";
  out << "    <Geometry Type=\"XYZ\">" << "\n";
  out << "      <DataItem Format=\"HDF\"  Dimensions=\"" << numVerts << " 3\" NumberType=\"Float\" Precision=\"4\">" << "\n";
  out << "        " << hdf5FilePath << ":/DataStructure/" << verticesPath.toString() << "\n";
  out << "      </DataItem>" << "\n";
  out << "    </Geometry>" << "\n";
  out << "" << "\n";
}

void WriteGeomXdmf(std::ostream& out, const HexahedralGeom& hexhedralGeom, std::string_view hdf5FilePath)
{
  std::string name = hexhedralGeom.getName();
  usize numPolyhedra = hexhedralGeom.getNumberOfPolyhedra();
  usize numVerts = hexhedralGeom.getNumberOfVertices();
  if(numPolyhedra == 0 || numVerts == 0)
  {
    return;
  }
  DataPath polyhedraPath = hexhedralGeom.getPolyhedraRef().getDataPaths().at(0);
  DataPath verticesPath = hexhedralGeom.getVerticesRef().getDataPaths().at(0);

  out << "  <!-- *************** START OF " << name << " *************** -->" << "\n";
  out << "  <Grid Name=\"" << name << "\" GridType=\"Uniform\">" << "\n";
  out << "    <Topology TopologyType=\"Hexahedron\" NumberOfElements=\"" << numPolyhedra << "\">" << "\n";
  out << "      <DataItem Format=\"HDF\" NumberType=\"Int\" Dimensions=\"" << numPolyhedra << " 8\">" << "\n";
  out << "        " << hdf5FilePath << ":/DataStructure/" << polyhedraPath.toString() << "\n";
  out << "      </DataItem>" << "\n";
  out << "    </Topology>" << "\n";
  out << "    <Geometry Type=\"XYZ\">" << "\n";
  out << "      <DataItem Format=\"HDF\"  Dimensions=\"" << numVerts << " 3\" NumberType=\"Float\" Precision=\"4\">" << "\n";
  out << "        " << hdf5FilePath << ":/DataStructure/" << verticesPath.toString() << "\n";
  out << "      </DataItem>" << "\n";
  out << "    </Geometry>" << "\n";
  out << "" << "\n";
}

void WriteXdmfHeader(std::ostream& out)
{
  out << "<?xml version=\"1.0\"?>" << "\n";
  out << "<!DOCTYPE Xdmf SYSTEM \"Xdmf.dtd\"[]>" << "\n";
  out << "<Xdmf xmlns:xi=\"http://www.w3.org/2003/XInclude\" Version=\"2.2\">" << "\n";
  out << " <Domain>" << "\n";
}

void WriteXdmfFooter(std::ostream& xdmf)
{
  xdmf << " </Domain>" << "\n";
  xdmf << "</Xdmf>" << "\n";
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
  ShapeType tupleDims = array.getTupleShape();

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
    out << "Center=\"" << centering << "\">" << "\n";
    // Open the <DataItem> Tag
    out << R"(      <DataItem Format="HDF" Dimensions=")" << dimStr << "\" ";
    out << "NumberType=\"" << xdmfTypeName << "\" " << "Precision=\"" << precision << "\" >" << "\n";
    out << "        " << hdf5DatasetPath << "\n";
    out << "      </DataItem>" << "\n";
    out << "    </Attribute>" << "\n";
  }
  else if(numComp == 2 || numComp == 6)
  {
    // First Slab
    out << "    <Attribute Name=\"" << arrayName << " (Feature 0)\" ";
    out << "AttributeType=\"" << attrType << "\" ";

    out << "Center=\"" << centering << "\">" << "\n";
    // Open the <DataItem> Tag
    out << R"(      <DataItem ItemType="HyperSlab" Dimensions=")" << dimStrHalf << "\" ";
    out << "Type=\"HyperSlab\" " << "Name=\"" << arrayName << " (Feature 0)\" >" << "\n";
    out << "        <DataItem Dimensions=\"3 2\" " << "Format=\"XML\" >" << "\n";
    out << "          0        0" << "\n";
    out << "          1        1" << "\n";
    out << "          " << dimStrHalf << " </DataItem>" << "\n";
    out << "\n";
    out << R"(        <DataItem Format="HDF" Dimensions=")" << dimStr << "\" " << "NumberType=\"" << xdmfTypeName << "\" " << "Precision=\"" << precision << "\" >" << "\n";

    out << "        " << hdf5DatasetPath << "\n";
    out << "        </DataItem>" << "\n";
    out << "      </DataItem>" << "\n";
    out << "    </Attribute>" << "\n"
        << "\n";

    // Second Slab
    out << "    <Attribute Name=\"" << arrayName << " (Feature 1)\" ";
    out << "AttributeType=\"" << attrType << "\" ";

    out << "Center=\"" << centering << "\">" << "\n";
    // Open the <DataItem> Tag
    out << R"(      <DataItem ItemType="HyperSlab" Dimensions=")" << dimStrHalf << "\" ";
    out << "Type=\"HyperSlab\" " << "Name=\"" << arrayName << " (Feature 1)\" >" << "\n";
    out << "        <DataItem Dimensions=\"3 2\" " << "Format=\"XML\" >" << "\n";
    out << "          0        " << (numComp / 2) << "\n";
    out << "          1        1" << "\n";
    out << "          " << dimStrHalf << " </DataItem>" << "\n";
    out << "\n";
    out << R"(        <DataItem Format="HDF" Dimensions=")" << dimStr << "\" " << "NumberType=\"" << xdmfTypeName << "\" " << "Precision=\"" << precision << "\" >" << "\n";
    out << "        " << hdf5DatasetPath << "\n";
    out << "        </DataItem>" << "\n";
    out << "      </DataItem>" << "\n";
    out << "    </Attribute>" << "\n";
  }
}

void WriteXdmfGeomFooter(std::ostream& xdmf, std::string_view geomName)
{
  xdmf << "  </Grid>" << "\n";
  xdmf << "  <!-- *************** END OF " << geomName << " *************** -->" << "\n";
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
  WriteXdmfNodeGeometry0D(out, nodeGeom1D, geomName, hdf5FilePath);

  const AttributeMatrix* edgeData = nodeGeom1D.getEdgeAttributeMatrix();
  if(edgeData == nullptr)
  {
    return;
  }
  WriteXdmfAttributeMatrix(out, *edgeData, geomName, hdf5FilePath, "Cell");
}

void WriteXdmfNodeGeometry2D(std::ostream& out, const INodeGeometry2D& nodeGeom2D, std::string_view geomName, std::string_view hdf5FilePath)
{
  WriteXdmfNodeGeometry1D(out, nodeGeom2D, geomName, hdf5FilePath);

  const AttributeMatrix* faceData = nodeGeom2D.getFaceAttributeMatrix();
  if(faceData == nullptr)
  {
    return;
  }
  WriteXdmfAttributeMatrix(out, *faceData, geomName, hdf5FilePath, "Cell");
}

void WriteXdmfNodeGeometry3D(std::ostream& out, const INodeGeometry3D& nodeGeom3D, std::string_view geomName, std::string_view hdf5FilePath)
{
  WriteXdmfNodeGeometry2D(out, nodeGeom3D, geomName, hdf5FilePath);

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
  auto fileReader = HDF5::FileIO::ReadFile(path);
  return GetFileVersion(fileReader);
}

DREAM3D::FileVersionType DREAM3D::GetFileVersion(const nx::core::HDF5::FileIO& fileReader)
{
  auto versionResult = fileReader.readStringAttribute(k_FileVersionTag.str());
  if(versionResult.invalid())
  {
    return versionResult.errors()[0].message;
  }
  return std::move(versionResult.value());
}

DREAM3D::PipelineVersionType DREAM3D::GetPipelineVersion(const nx::core::HDF5::FileIO& fileReader)
{
  auto pipelineGroup = fileReader.openGroup(k_PipelineJsonTag);
  auto valueResult = pipelineGroup.readScalarAttribute<int32>(k_PipelineVersionTag);
  if(valueResult.invalid())
  {
    return k_InvalidPipelineVersion;
  }
  return valueResult.value();
}

Result<DataStructure> ImportDataStructureV8(const nx::core::HDF5::FileIO& fileReader, bool preflight)
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
Result<IDataArray*> createLegacyDataArray(DataStructure& dataStructure, DataObject::IdType parentId, const HDF5::DatasetIO& dataArrayReader, const std::vector<usize>& tDims,
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
    auto dataSpan = dataStore->createSpan();
    Result<> result = dataArrayReader.readIntoSpan(dataSpan);
    if(result.invalid())
    {
      std::string ss = fmt::format("Error reading HDF5 Data set: {}", dataArrayReader.getName());
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
Result<> readLegacyDataArrayDims(const nx::core::HDF5::DatasetIO& dataArrayReader, std::vector<usize>& tDims, std::vector<usize>& cDims)
{
  Result<std::vector<usize>> cDimsResult = dataArrayReader.readVectorAttribute<usize>(Legacy::CompDims);
  if(cDimsResult.invalid())
  {
    return ConvertResult<std::vector<usize>>(std::move(cDimsResult));
  }
  cDims = std::move(cDimsResult.value());

  auto tDimsResult = dataArrayReader.readVectorAttribute<usize>(Legacy::TupleDims);
  if(tDimsResult.invalid())
  {
    return ConvertResult<std::vector<usize>>(std::move(tDimsResult));
  }
  tDims = std::move(tDimsResult.value());

  std::ranges::reverse(tDims); // SIMPL writes the Tuple Dimensions in reverse order to this attribute

  return {};
}

Result<> readLegacyStringArray(DataStructure& dataStructure, const nx::core::HDF5::DatasetIO& dataArrayReader, DataObject::IdType parentId, bool preflight = false)
{
  const std::string daName = dataArrayReader.getName();

  if(preflight)
  {
    ShapeType tDims;
    ShapeType cDims;
    auto result = readLegacyDataArrayDims(dataArrayReader, tDims, cDims);
    if(result.invalid())
    {
      return result;
    }

    auto numElements =
        std::accumulate(tDims.cbegin(), tDims.cend(), static_cast<usize>(1), std::multiplies<>()) * std::accumulate(cDims.cbegin(), cDims.cend(), static_cast<usize>(1), std::multiplies<>());
    const std::vector<std::string> strings(numElements);
    StringArray::CreateWithValues(dataStructure, daName, tDims, strings, parentId);
  }
  else
  {
    const std::vector<std::string> strings = dataArrayReader.readAsVectorOfStrings();
    StringArray::CreateWithValues(dataStructure, daName, ShapeType{strings.size()}, strings, parentId);
  }
  return {};
}

Result<> finishImportingLegacyStringArray(DataStructure& dataStructure, const nx::core::HDF5::DatasetIO& dataArrayReader, const DataPath& dataPath)
{
  auto* existingArray = dataStructure.getDataAs<StringArray>(dataPath);
  if(existingArray == nullptr)
  {
    return MakeErrorResult(-4210428, fmt::format("Failed to finish importing legacy StringArray at path '{}'. Imported StringArray not found.", dataPath.toString()));
  }

  const std::vector<std::string> strings = dataArrayReader.readAsVectorOfStrings();
  ShapeType tupShape = existingArray->getTupleShape();
  if(existingArray->getNumberOfTuples() != strings.size())
  {
    tupShape = ShapeType{strings.size()};
  }

  existingArray->setStore(std::make_shared<StringStore>(strings, tupShape));

  return {};
}

Result<IDataArray*> readLegacyDataArray(DataStructure& dataStructure, const nx::core::HDF5::DatasetIO& dataArrayReader, DataObject::IdType parentId, bool preflight = false)
{
  auto dataTypeResult = dataArrayReader.getDataType();
  if(dataTypeResult.invalid())
  {
    auto errors = dataTypeResult.errors();
    return MakeErrorResult<IDataArray*>(errors[0].code, errors[0].message);
  }
  auto dataType = std::move(dataTypeResult.value());

  ShapeType tDims;
  ShapeType cDims;
  Result<> dimsResult = readLegacyDataArrayDims(dataArrayReader, tDims, cDims);
  if(dimsResult.invalid())
  {
    auto& error = dimsResult.errors()[0];
    return MakeErrorResult<IDataArray*>(error.code, error.message);
  }

  Result<IDataArray*> daResult;
  switch(dataType)
  {
  case DataType::float32:
    daResult = createLegacyDataArray<float32>(dataStructure, parentId, dataArrayReader, tDims, cDims, preflight);
    break;
  case DataType::float64:
    daResult = createLegacyDataArray<float64>(dataStructure, parentId, dataArrayReader, tDims, cDims, preflight);
    break;
  case DataType::int8:
    daResult = createLegacyDataArray<int8>(dataStructure, parentId, dataArrayReader, tDims, cDims, preflight);
    break;
  case DataType::int16:
    daResult = createLegacyDataArray<int16>(dataStructure, parentId, dataArrayReader, tDims, cDims, preflight);
    break;
  case DataType::int32:
    daResult = createLegacyDataArray<int32>(dataStructure, parentId, dataArrayReader, tDims, cDims, preflight);
    break;
  case DataType::int64:
    daResult = createLegacyDataArray<int64>(dataStructure, parentId, dataArrayReader, tDims, cDims, preflight);
    break;
  case DataType::boolean:
    [[fallthrough]];
  case DataType::uint8: {
    std::string typeTag;
    auto typeTagResult = dataArrayReader.readStringAttribute(Constants::k_ObjectTypeTag);
    if(typeTagResult.invalid())
    {
      return ConvertInvalidResult<IDataArray*, std::string>(std::move(typeTagResult));
    }
    typeTag = std::move(typeTagResult.value());
    if(typeTag == "DataArray<bool>")
    {
      daResult = createLegacyDataArray<bool>(dataStructure, parentId, dataArrayReader, tDims, cDims, preflight);
    }
    else
    {
      daResult = createLegacyDataArray<uint8>(dataStructure, parentId, dataArrayReader, tDims, cDims, preflight);
    }
    break;
  }
  case DataType::uint16:
    daResult = createLegacyDataArray<uint16>(dataStructure, parentId, dataArrayReader, tDims, cDims, preflight);
    break;
  case DataType::uint32:
    daResult = createLegacyDataArray<uint32>(dataStructure, parentId, dataArrayReader, tDims, cDims, preflight);
    break;
  case DataType::uint64:
    daResult = createLegacyDataArray<uint64>(dataStructure, parentId, dataArrayReader, tDims, cDims, preflight);
    break;
  }

  return daResult;
}

template <typename T>
Result<> finishImportingLegacyDataArrayImpl(DataStructure& dataStructure, const HDF5::DatasetIO& dataSetIO, const DataPath& dataPath)
{
  auto* existingArray = dataStructure.getDataAs<DataArray<T>>(dataPath);
  if(existingArray == nullptr)
  {
    return MakeErrorResult(-4210423, fmt::format("Failed to finish importing legacy DataArray at path '{}'. Imported DataArray not found.", dataPath.toString()));
  }

  auto tupleShape = nx::core::HDF5::IDataStoreIO::ReadTupleShape(dataSetIO);
  auto componentShape = nx::core::HDF5::IDataStoreIO::ReadComponentShape(dataSetIO);

  // Reverse the tuple shape because the attribute tuple dimensions was written in reverse for these legacy data arrays
  tupleShape = {tupleShape.rbegin(), tupleShape.rend()};

  auto dataStorePtr = dataSetIO.readAsDataStore<T>(tupleShape, componentShape);
  if(dataStorePtr == nullptr)
  {
    return MakeErrorResult(-4210424, fmt::format("Failed to finish importing legacy DataArray at path '{}'. Could not import data from HDF5.", dataPath.toString()));
  }

  existingArray->setDataStore(dataStorePtr);
  return {};
}

Result<> finishImportingLegacyDataArray(DataStructure& dataStructure, const HDF5::DatasetIO& dataSetIO, const DataPath& dataPath)
{
  auto dataTypeResult = dataSetIO.getDataType();
  if(dataTypeResult.invalid())
  {
    auto errors = dataTypeResult.errors();
    return MakeErrorResult(errors[0].code, errors[0].message);
  }
  auto dataType = std::move(dataTypeResult.value());
  switch(dataType)
  {
  case DataType::float32:
    return finishImportingLegacyDataArrayImpl<float32>(dataStructure, dataSetIO, dataPath);
  case DataType::float64:
    return finishImportingLegacyDataArrayImpl<float64>(dataStructure, dataSetIO, dataPath);
    break;
  case DataType::int8:
    return finishImportingLegacyDataArrayImpl<int8>(dataStructure, dataSetIO, dataPath);
    break;
  case DataType::int16:
    return finishImportingLegacyDataArrayImpl<int16>(dataStructure, dataSetIO, dataPath);
    break;
  case DataType::int32:
    return finishImportingLegacyDataArrayImpl<int32>(dataStructure, dataSetIO, dataPath);
    break;
  case DataType::int64:
    return finishImportingLegacyDataArrayImpl<int64>(dataStructure, dataSetIO, dataPath);
    break;
  case DataType::boolean:
    [[fallthrough]];
  case DataType::uint8: {
    std::string typeTag;
    auto typeTagResult = dataSetIO.readStringAttribute(Constants::k_ObjectTypeTag);
    if(typeTagResult.invalid())
    {
      return ConvertInvalidResult<void, std::string>(std::move(typeTagResult));
    }
    typeTag = std::move(typeTagResult.value());
    if(typeTag == "DataArray<bool>")
    {
      return finishImportingLegacyDataArrayImpl<bool>(dataStructure, dataSetIO, dataPath);
    }
    else
    {
      return finishImportingLegacyDataArrayImpl<uint8>(dataStructure, dataSetIO, dataPath);
    }
    break;
  }
  case DataType::uint16:
    return finishImportingLegacyDataArrayImpl<uint16>(dataStructure, dataSetIO, dataPath);
    break;
  case DataType::uint32:
    return finishImportingLegacyDataArrayImpl<uint32>(dataStructure, dataSetIO, dataPath);
    break;
  case DataType::uint64:
    return finishImportingLegacyDataArrayImpl<uint64>(dataStructure, dataSetIO, dataPath);
    break;
  }

  return {};
}

Result<UInt64Array*> readLegacyNodeConnectivityList(DataStructure& dataStructure, IGeometry* geometry, const HDF5::GroupIO& geomGroup, const std::string& arrayName, bool preflight = false)
{
  HDF5::DatasetIO dataArrayReader = geomGroup.openDataset(arrayName);
  DataObject::IdType parentId = geometry->getId();

  ShapeType tDims;
  ShapeType cDims;
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
Result<> createLegacyNeighborList(DataStructure& dataStructure, DataObject ::IdType parentId, const nx::core::HDF5::GroupIO& parentReader, const nx::core::HDF5::DatasetIO& datasetReader,
                                  const ShapeType& tupleDims)
{
  auto listStore = HDF5::NeighborListIO<T>::ReadHdf5Data(parentReader, datasetReader);
  auto* neighborList = NeighborList<T>::Create(dataStructure, datasetReader.getName(), listStore, parentId);
  if(neighborList == nullptr)
  {
    std::string ss = fmt::format("Failed to create NeighborList: '{}'", datasetReader.getName());
    return MakeErrorResult(Legacy::k_FailedCreatingNeighborList_Code, ss);
  }
  return {};
}

Result<> readLegacyNeighborList(DataStructure& dataStructure, const nx::core::HDF5::GroupIO& parentReader, const nx::core::HDF5::DatasetIO& datasetReader, DataObject::IdType parentId)
{
  auto dataTypeResult = datasetReader.getDataType();
  if(dataTypeResult.invalid())
  {
    return ConvertResult(std::move(dataTypeResult));
  }
  auto dataType = dataTypeResult.value();

  ShapeType tDims;
  auto tDimsResult = datasetReader.readVectorAttribute<usize>(Legacy::TupleDims);
  tDims = std::move(tDimsResult.value());

  Result<> result;

  switch(dataType)
  {
  case DataType::float32:
    result = createLegacyNeighborList<float32>(dataStructure, parentId, parentReader, datasetReader, tDims);
    break;
  case DataType::float64:
    result = createLegacyNeighborList<float64>(dataStructure, parentId, parentReader, datasetReader, tDims);
    break;
  case DataType::boolean:
    [[fallthrough]];
  case DataType::int8:
    result = createLegacyNeighborList<int8>(dataStructure, parentId, parentReader, datasetReader, tDims);
    break;
  case DataType::int16:
    result = createLegacyNeighborList<int16>(dataStructure, parentId, parentReader, datasetReader, tDims);
    break;
  case DataType::int32:
    result = createLegacyNeighborList<int32>(dataStructure, parentId, parentReader, datasetReader, tDims);
    break;
  case DataType::int64:
    result = createLegacyNeighborList<int64>(dataStructure, parentId, parentReader, datasetReader, tDims);
    break;
  case DataType::uint8:
    result = createLegacyNeighborList<uint8>(dataStructure, parentId, parentReader, datasetReader, tDims);
    break;
  case DataType::uint16:
    result = createLegacyNeighborList<uint16>(dataStructure, parentId, parentReader, datasetReader, tDims);
    break;
  case DataType::uint32:
    result = createLegacyNeighborList<uint32>(dataStructure, parentId, parentReader, datasetReader, tDims);
    break;
  case DataType::uint64:
    result = createLegacyNeighborList<uint64>(dataStructure, parentId, parentReader, datasetReader, tDims);
    break;
  }

  return result;
}

template <typename T>
Result<> finishImportingLegacyNeighborListImpl(DataStructure& dataStructure, const nx::core::HDF5::GroupIO& parentReader, const HDF5::DatasetIO& datasetReader, const DataPath& dataPath)
{
  auto* existingList = dataStructure.getDataAs<NeighborList<T>>(dataPath);
  if(existingList == nullptr)
  {
    return MakeErrorResult(-4210426, fmt::format("Failed to finish importing legacy NeighborList at path '{}'. Imported NeighborList not found.", dataPath.toString()));
  }

  auto listStore = HDF5::NeighborListIO<T>::ReadHdf5Data(parentReader, datasetReader);
  if(listStore == nullptr)
  {
    return MakeErrorResult(-4210427, fmt::format("Failed to finish importing legacy NeighborList at path '{}'. Failed to import HDF5 data.", dataPath.toString()));
  }
  existingList->setStore(listStore);
  return {};
}

Result<> finishImportingLegacyNeighborList(DataStructure& dataStructure, const nx::core::HDF5::GroupIO& parentReader, const HDF5::DatasetIO& datasetReader, const DataPath& dataPath)
{
  auto dataTypeResult = datasetReader.getDataType();
  if(dataTypeResult.invalid())
  {
    return ConvertResult(std::move(dataTypeResult));
  }
  auto dataType = dataTypeResult.value();
  switch(dataType)
  {
  case DataType::float32:
    return finishImportingLegacyNeighborListImpl<float32>(dataStructure, parentReader, datasetReader, dataPath);
    break;
  case DataType::float64:
    return finishImportingLegacyNeighborListImpl<float64>(dataStructure, parentReader, datasetReader, dataPath);
    break;
  case DataType::boolean:
    [[fallthrough]];
  case DataType::int8:
    return finishImportingLegacyNeighborListImpl<int8>(dataStructure, parentReader, datasetReader, dataPath);
    break;
  case DataType::int16:
    return finishImportingLegacyNeighborListImpl<int16>(dataStructure, parentReader, datasetReader, dataPath);
    break;
  case DataType::int32:
    return finishImportingLegacyNeighborListImpl<int32>(dataStructure, parentReader, datasetReader, dataPath);
    break;
  case DataType::int64:
    return finishImportingLegacyNeighborListImpl<int64>(dataStructure, parentReader, datasetReader, dataPath);
    break;
  case DataType::uint8:
    return finishImportingLegacyNeighborListImpl<uint8>(dataStructure, parentReader, datasetReader, dataPath);
    break;
  case DataType::uint16:
    return finishImportingLegacyNeighborListImpl<uint16>(dataStructure, parentReader, datasetReader, dataPath);
    break;
  case DataType::uint32:
    return finishImportingLegacyNeighborListImpl<uint32>(dataStructure, parentReader, datasetReader, dataPath);
    break;
  case DataType::uint64:
    return finishImportingLegacyNeighborListImpl<uint64>(dataStructure, parentReader, datasetReader, dataPath);
    break;
  }

  return MakeErrorResult(-4210429, fmt::format("Failed to finish importing legacy NeighborList at path '{}'. Type could not be determined", dataPath.toString()));
}

bool isLegacyNeighborList(const nx::core::HDF5::DatasetIO& arrayReader)
{
  auto objectTypeResult = arrayReader.readStringAttribute("ObjectType");
  if(objectTypeResult.invalid())
  {
    return false;
  }
  return objectTypeResult.value() == "NeighborList<T>";
}

bool isLegacyStringArray(const nx::core::HDF5::DatasetIO& arrayReader)
{
  auto objectTypeResult = arrayReader.readStringAttribute("ObjectType");
  if(objectTypeResult.invalid())
  {
    return false;
  }
  return objectTypeResult.value() == "StringDataArray";
}

Result<> readLegacyArray(DataStructure& dataStructure, const nx::core::HDF5::GroupIO& amGroupReader, const std::string& arrayName, bool preflight)
{
  auto dataArraySet = amGroupReader.openDataset(arrayName);
  if(isLegacyNeighborList(dataArraySet))
  {
    return readLegacyNeighborList(dataStructure, amGroupReader, dataArraySet, 0);
  }
  else if(isLegacyStringArray(dataArraySet))
  {
    return readLegacyStringArray(dataStructure, dataArraySet, preflight);
  }
  else
  {
    return ConvertResult<>(std::move(readLegacyDataArray(dataStructure, dataArraySet, preflight)));
  }
}

Result<> finishImportingLegacyArray(DataStructure& dataStructure, const nx::core::HDF5::GroupIO& parentReader, const DataPath& dataPath)
{
  auto dcGroup = parentReader.openGroup(dataPath[0]);
  auto amGroup = dcGroup.openGroup(dataPath[1]);
  auto dataArraySet = amGroup.openDataset(dataPath.getTargetName());
  if(isLegacyNeighborList(dataArraySet))
  {
    return finishImportingLegacyNeighborList(dataStructure, amGroup, dataArraySet, dataPath);
  }
  else if(isLegacyStringArray(dataArraySet))
  {
    return finishImportingLegacyStringArray(dataStructure, dataArraySet, dataPath);
  }
  else
  {
    return finishImportingLegacyDataArray(dataStructure, dataArraySet, dataPath);
  }
}

Result<> readDatasetAsDataArray(DataStructure& dataStructure, const HDF5::DatasetIO& datasetIO, DataObject::IdType parentId, bool preflight)
{
  ShapeType tDims = datasetIO.getDimensions();
  if(tDims.empty())
  {
    return MakeErrorResult(-13345, "Unable to read dataset dimensions");
  }
  ShapeType cDims = {1};
  Result<DataType> dataTypeResult = datasetIO.getDataType();
  if(dataTypeResult.invalid())
  {
    hid_t datasetId = datasetIO.getId();
    hid_t typeId = H5Dget_type(datasetId);
    H5T_class_t classType = H5Tget_class(typeId);
    H5Tclose(typeId);
    if(classType == H5T_STRING)
    {
      usize size = std::accumulate(tDims.cbegin(), tDims.cend(), static_cast<usize>(1), std::multiplies<>());
      StringArray* stringArray = StringArray::CreateWithValues(dataStructure, datasetIO.getName(), tDims, std::vector<std::string>(size), parentId);
      return {};
    }
  }

  DataType dataType = dataTypeResult.value();
  switch(dataType)
  {
  case DataType::float32: {
    return ConvertResult(createLegacyDataArray<float32>(dataStructure, parentId, datasetIO, tDims, cDims, preflight));
  }
  case DataType::float64: {
    return ConvertResult(createLegacyDataArray<float64>(dataStructure, parentId, datasetIO, tDims, cDims, preflight));
  }
  case DataType::int8: {
    return ConvertResult(createLegacyDataArray<int8>(dataStructure, parentId, datasetIO, tDims, cDims, preflight));
  }
  case DataType::int16: {
    return ConvertResult(createLegacyDataArray<int16>(dataStructure, parentId, datasetIO, tDims, cDims, preflight));
  }
  case DataType::int32: {
    return ConvertResult(createLegacyDataArray<int32>(dataStructure, parentId, datasetIO, tDims, cDims, preflight));
  }
  case DataType::int64: {
    return ConvertResult(createLegacyDataArray<int64>(dataStructure, parentId, datasetIO, tDims, cDims, preflight));
  }
  case DataType::boolean: {
    return ConvertResult(createLegacyDataArray<bool>(dataStructure, parentId, datasetIO, tDims, cDims, preflight));
  }
  case DataType::uint8: {
    return ConvertResult(createLegacyDataArray<uint8>(dataStructure, parentId, datasetIO, tDims, cDims, preflight));
  }
  case DataType::uint16: {
    return ConvertResult(createLegacyDataArray<uint16>(dataStructure, parentId, datasetIO, tDims, cDims, preflight));
  }
  case DataType::uint32: {
    return ConvertResult(createLegacyDataArray<uint32>(dataStructure, parentId, datasetIO, tDims, cDims, preflight));
  }
  case DataType::uint64: {
    return ConvertResult(createLegacyDataArray<uint64>(dataStructure, parentId, datasetIO, tDims, cDims, preflight));
  }
  }
  return MakeErrorResult(-456345, fmt::format("StatsReader: Unsupported array type: {}", to_underlying(dataType)));
}

Result<> readLegacyStatsDataArrayDatasetChild(DataStructure& dataStructure, const nx::core::HDF5::DatasetIO& datasetIO, DataObject::IdType parentId, bool preflight)
{
  Result<std::string> objectTypeResult = datasetIO.readStringAttribute(Constants::k_ObjectTypeTag);
  if(objectTypeResult.invalid())
  {
    return readDatasetAsDataArray(dataStructure, datasetIO, parentId, preflight);
  }
  std::string objectType = std::move(objectTypeResult.value());
  if(objectType.starts_with("DataArray<"))
  {
    return ConvertResult(readLegacyDataArray(dataStructure, datasetIO, parentId, preflight));
  }
  return MakeErrorResult(-343254, fmt::format("Unable to read dataset \"{}\"", datasetIO.getName()));
}

template <typename T>
Result<IDataArray*> CreateDataArrayFromAttribute(DataStructure& dataStructure, DataObject::IdType parentId, const HDF5::ObjectIO& objectIO, const std::string& attributeName,
                                                 const std::string& dataArrayName, bool preflight)
{
  Result<std::vector<T>> result = objectIO.readVectorAttribute<T>(attributeName);
  if(result.invalid())
  {
    return nx::core::MakeErrorResult<IDataArray*>(Legacy::k_FailedReadingDataArrayData_Code, fmt::format("Error reading HDF5 attribute: {}", attributeName));
  }
  std::vector<T> data = std::move(result.value());

  std::vector<usize> tDims = {data.size()};
  std::vector<usize> cDims = {1};

  DataArray<T>* dataArray = nullptr;

  if(preflight)
  {
    dataArray = DataArray<T>::template CreateWithStore<EmptyDataStore<T>>(dataStructure, dataArrayName, tDims, cDims, parentId);
  }
  else
  {
    auto dataStore = std::make_unique<DataStore<T>>(tDims, cDims, static_cast<T>(0));
    std::copy(data.begin(), data.end(), dataStore->begin());
    dataArray = DataArray<T>::Create(dataStructure, dataArrayName, std::move(dataStore), parentId);
  }

  if(nullptr == dataArray)
  {
    return nx::core::MakeErrorResult<IDataArray*>(Legacy::k_FailedCreatingArray_Code, fmt::format("Failed to create DataArray: '{}'", dataArrayName));
  }

  return {dataArray};
}

Result<> ReadAttributeAsDataArray(HDF5::ObjectIO& objectIO, const std::string& attributeName, DataStructure& dataStructure, DataObject::IdType parentId, bool preflight, std::string_view prefix)
{
  HDF_ERROR_HANDLER_OFF
  hid_t attribId = H5Aopen(objectIO.getId(), attributeName.c_str(), H5P_DEFAULT);
  HDF_ERROR_HANDLER_ON
  if(attribId < 0)
  {
    return MakeErrorResult(-16565, fmt::format("Unable to open attribute \"\"", attributeName));
  }
  hid_t typeId = H5Aget_type(attribId);

  std::string daName = fmt::format("{}{}", prefix, attributeName);

  Result<> result;

  H5T_class_t classType = H5Tget_class(typeId);
  if(classType == H5T_STRING)
  {
    Result<std::string> stringResult = objectIO.readStringAttribute(attributeName);
    if(result.valid())
    {
      auto* stringArray = StringArray::CreateWithValues(dataStructure, daName, {1}, std::vector<std::string>{std::move(stringResult.value())}, parentId);
      if(stringArray == nullptr)
      {
        result = MakeErrorResult(-16566, "Unable to create StringArray");
      }
    }
    else
    {
      result = ConvertResult(std::move(stringResult));
    }
  }
  else
  {
    HDF5::Type type = HDF5::getTypeFromId(typeId);

    switch(type)
    {
    case HDF5::Type::int8: {
      result = ConvertResult(CreateDataArrayFromAttribute<int8>(dataStructure, parentId, objectIO, attributeName, daName, preflight));
      break;
    }
    case HDF5::Type::int16: {
      result = ConvertResult(CreateDataArrayFromAttribute<int16>(dataStructure, parentId, objectIO, attributeName, daName, preflight));
      break;
    }
    case HDF5::Type::int32: {
      result = ConvertResult(CreateDataArrayFromAttribute<int32>(dataStructure, parentId, objectIO, attributeName, daName, preflight));
      break;
    }
    case HDF5::Type::int64: {
      result = ConvertResult(CreateDataArrayFromAttribute<int64>(dataStructure, parentId, objectIO, attributeName, daName, preflight));
      break;
    }
    case HDF5::Type::uint8: {
      result = ConvertResult(CreateDataArrayFromAttribute<uint8>(dataStructure, parentId, objectIO, attributeName, daName, preflight));
      break;
    }
    case HDF5::Type::uint16: {
      result = ConvertResult(CreateDataArrayFromAttribute<uint16>(dataStructure, parentId, objectIO, attributeName, daName, preflight));
      break;
    }
    case HDF5::Type::uint32: {
      result = ConvertResult(CreateDataArrayFromAttribute<uint32>(dataStructure, parentId, objectIO, attributeName, daName, preflight));
      break;
    }
    case HDF5::Type::uint64: {
      result = ConvertResult(CreateDataArrayFromAttribute<uint64>(dataStructure, parentId, objectIO, attributeName, daName, preflight));
      break;
    }
    case HDF5::Type::float32: {
      result = ConvertResult(CreateDataArrayFromAttribute<float32>(dataStructure, parentId, objectIO, attributeName, daName, preflight));
      break;
    }
    case HDF5::Type::float64: {
      result = ConvertResult(CreateDataArrayFromAttribute<float64>(dataStructure, parentId, objectIO, attributeName, daName, preflight));
      break;
    }
    default: {
      result = MakeErrorResult(-16567, "Invalid HDF5 for DataArray");
      break;
    }
    }
  }

  H5Aclose(attribId);
  H5Tclose(typeId);

  return ConvertResult(std::move(result));
}

Result<> ReadAllAttributesAsDataArrays(DataStructure& dataStructure, nx::core::HDF5::ObjectIO& objectIO, DataObject::IdType parentId, bool preflight, std::string_view prefix,
                                       const std::set<std::string>& exclusions)
{
  auto attributeNames = objectIO.getAttributeNames();
  for(const auto& name : attributeNames)
  {
    if(exclusions.contains(name))
    {
      continue;
    }
    auto result = ReadAttributeAsDataArray(objectIO, name, dataStructure, parentId, preflight, prefix);
    if(result.invalid())
    {
      return result;
    }
  }
  return {};
}

Result<> readLegacyStatsDataArrayChild(DataStructure& dataStructure, const nx::core::HDF5::GroupIO& parentReader, const std::string& name, DataObject::IdType parentId, bool preflight)
{
  if(parentReader.isDataset(name))
  {
    HDF5::DatasetIO datasetIO = parentReader.openDataset(name);
    std::string prefix = fmt::format("{}_", datasetIO.getName());
    static const std::set<std::string> exclusions = {Legacy::CompDims, Legacy::TupleDims, "ObjectType", "Tuple Axis Dimensions", "DataArrayVersion"};
    Result<> attributeResult = ReadAllAttributesAsDataArrays(dataStructure, datasetIO, parentId, false, prefix, exclusions);
    if(attributeResult.invalid())
    {
      return attributeResult;
    }
    return readLegacyStatsDataArrayDatasetChild(dataStructure, datasetIO, parentId, preflight);
  }
  if(parentReader.isGroup(name))
  {
    HDF5::GroupIO groupIO = parentReader.openGroup(name);
    DataGroup* dataGroup = DataGroup::Create(dataStructure, name, parentId);
    if(dataGroup == nullptr)
    {
      return MakeErrorResult(-1434535, fmt::format("Unable to create group \"{}\"", name));
    }
    DataObject::IdType groupId = dataGroup->getId();
    Result<> attributeResult = ReadAllAttributesAsDataArrays(dataStructure, groupIO, groupId, false, "", {});
    if(attributeResult.invalid())
    {
      return attributeResult;
    }
    std::vector<std::string> groupChildren = groupIO.getChildNames();
    for(const auto& childName : groupChildren)
    {
      Result<> result = readLegacyStatsDataArrayChild(dataStructure, groupIO, childName, groupId, preflight);
      if(result.invalid())
      {
        return result;
      }
    }
    return {};
  }
  return MakeErrorResult(-769634, fmt::format("StatsReader: Unsupported object type for \"{}\"", name));
}

Result<> readLegacyStatsDataArray(DataStructure& dataStructure, const nx::core::HDF5::GroupIO& statsReader, DataObject::IdType parentId, bool /*preflight*/)
{
  // Always fully import Statistics data (ignoring the preflight parameter) because
  // the Statistics hierarchy produces DataPaths of depth > 3 (up to depth 6) which
  // FinishImportingLegacyDataObject cannot handle. Since StatsDataArray data is
  // relatively small, fully importing during the initial read is safe and avoids
  // the need for a separate finish-importing step.
  std::string statsGroupName = "Statistics";
  DataGroup* dataGroup = DataGroup::Create(dataStructure, statsGroupName, parentId);
  if(dataGroup == nullptr)
  {
    return MakeErrorResult(-1434547, fmt::format("Unable to create group \"{}\"", statsGroupName));
  }
  std::vector<std::string> childNames = statsReader.getChildNames();
  for(const auto& name : childNames)
  {
    Result<> result = readLegacyStatsDataArrayChild(dataStructure, statsReader, name, dataGroup->getId(), preflight);
    if(result.invalid())
    {
      return result;
    }
  }
  return {};
}

Result<> readLegacyAttributeMatrix(DataStructure& dataStructure, const nx::core::HDF5::GroupIO& amGroupReader, DataObject& parent, bool preflight = false, bool importChildren = true)
{
  DataObject::IdType parentId = parent.getId();
  const std::string amName = amGroupReader.getName();

  auto tDimsResult = amGroupReader.readVectorAttribute<usize>("TupleDimensions");
  if(tDimsResult.invalid())
  {
    return ConvertResult(std::move(tDimsResult));
  }
  ShapeType tDims = std::move(tDimsResult.value());
  auto reversedTDims = ShapeType(tDims.crbegin(), tDims.crend());

  auto* attributeMatrix = AttributeMatrix::Create(dataStructure, amName, reversedTDims, parentId);

  std::vector<Result<>> daResults;
  if(importChildren)
  {
    auto childNames = amGroupReader.getChildNames();
    for(const auto& childName : childNames)
    {
      if(!amGroupReader.isDataset(childName))
      {
        auto groupReader = amGroupReader.openGroup(childName);
        auto objectTypeResult = groupReader.readStringAttribute(Constants::k_ObjectTypeTag);
        if(objectTypeResult.valid() && objectTypeResult.value() == "Statistics")
        {
          daResults.push_back(readLegacyStatsDataArray(dataStructure, groupReader, parentId, preflight));
        }
        else
        {
          Result<> unsupportedDataResult = MakeWarningVoidResult(-298012, fmt::format("DataObject '{}' is not a supported simplnx data type", childName));
          daResults.push_back(unsupportedDataResult);
        }
      }
      else
      {
        auto dataArraySet = amGroupReader.openDataset(childName);

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
          Result<> result = ConvertResult(readLegacyDataArray(dataStructure, dataArraySet, attributeMatrix->getId(), preflight));
          daResults.push_back(result);
        }
      }
    }
  }

  uint32 amType;
  auto amTypeResult = amGroupReader.readScalarAttribute<uint32>("AttributeMatrixType");
  if(amTypeResult.invalid())
  {
    return ConvertResult(std::move(amTypeResult));
  }
  amType = std::move(amTypeResult.value());
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
void readGenericGeomDims(IGeometry* geom, const nx::core::HDF5::GroupIO& geomGroup)
{
  int32 sDims = 0;
  if(auto sDimsResult = geomGroup.readScalarAttribute<int32>("SpatialDimensionality"); sDimsResult.valid())
  {
    sDims = std::move(sDimsResult.value());
  }

  int32 uDims = 0;
  if(auto uDimsResult = geomGroup.readScalarAttribute<int32>("UnitDimensionality"); uDimsResult.valid())
  {
    uDims = std::move(uDimsResult.value());
  }

  geom->setSpatialDimensionality(sDims);
  geom->setUnitDimensionality(uDims);
}

Result<IDataArray*> readLegacyGeomArray(DataStructure& dataStructure, IGeometry* geometry, const nx::core::HDF5::GroupIO& geomGroup, const std::string& arrayName, bool preflight)
{
  auto dataArraySet = geomGroup.openDataset(arrayName);
  return readLegacyDataArray(dataStructure, dataArraySet, geometry->getId(), preflight);
}

template <typename T>
Result<T*> readLegacyGeomArrayAs(DataStructure& dataStructure, IGeometry* geometry, const nx::core::HDF5::GroupIO& geomGroup, const std::string& arrayName, bool preflight)
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

DataObject* readLegacyVertexGeom(DataStructure& dataStructure, const nx::core::HDF5::GroupIO& geomGroup, const std::string& name, bool preflight)
{
  auto* geom = VertexGeom::Create(dataStructure, name);
  readGenericGeomDims(geom, geomGroup);
  Result<Float32Array*> sharedVertexList = readLegacyGeomArrayAs<Float32Array>(dataStructure, geom, geomGroup, Legacy::VertexListName, preflight);

  geom->setVertices(*sharedVertexList.value());
  return geom;
}

DataObject* readLegacyTriangleGeom(DataStructure& dataStructure, const nx::core::HDF5::GroupIO& geomGroup, const std::string& name, bool preflight)
{
  auto geom = TriangleGeom::Create(dataStructure, name);
  readGenericGeomDims(geom, geomGroup);
  auto sharedVertexList = readLegacyGeomArrayAs<Float32Array>(dataStructure, geom, geomGroup, Legacy::VertexListName, preflight);
  auto sharedTriList = readLegacyNodeConnectivityList(dataStructure, geom, geomGroup, Legacy::TriListName, preflight);

  geom->setVertices(*sharedVertexList.value());
  geom->setFaceList(*sharedTriList.value());

  return geom;
}

DataObject* readLegacyTetrahedralGeom(DataStructure& dataStructure, const nx::core::HDF5::GroupIO& geomGroup, const std::string& name, bool preflight)
{
  auto geom = TetrahedralGeom::Create(dataStructure, name);
  readGenericGeomDims(geom, geomGroup);
  auto sharedVertexList = readLegacyGeomArrayAs<Float32Array>(dataStructure, geom, geomGroup, Legacy::VertexListName, preflight);
  auto sharedTetList = readLegacyNodeConnectivityList(dataStructure, geom, geomGroup, Legacy::TetraListName, preflight);

  geom->setVertices(*sharedVertexList.value());
  geom->setPolyhedraList(*sharedTetList.value());

  return geom;
}

DataObject* readLegacyRectGridGeom(DataStructure& dataStructure, const nx::core::HDF5::GroupIO& geomGroup, const std::string& name, bool preflight)
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

DataObject* readLegacyQuadGeom(DataStructure& dataStructure, const nx::core::HDF5::GroupIO& geomGroup, const std::string& name, bool preflight)
{
  auto geom = QuadGeom::Create(dataStructure, name);
  readGenericGeomDims(geom, geomGroup);
  auto sharedVertexList = readLegacyGeomArrayAs<Float32Array>(dataStructure, geom, geomGroup, Legacy::VertexListName, preflight);
  auto sharedQuadList = readLegacyNodeConnectivityList(dataStructure, geom, geomGroup, Legacy::QuadListName, preflight);

  geom->setVertices(*sharedVertexList.value());
  geom->setFaceList(*sharedQuadList.value());

  return geom;
}

DataObject* readLegacyHexGeom(DataStructure& dataStructure, const nx::core::HDF5::GroupIO& geomGroup, const std::string& name, bool preflight)
{
  auto geom = HexahedralGeom::Create(dataStructure, name);
  readGenericGeomDims(geom, geomGroup);
  auto sharedVertexList = readLegacyGeomArrayAs<Float32Array>(dataStructure, geom, geomGroup, Legacy::VertexListName, preflight);
  auto sharedHexList = readLegacyNodeConnectivityList(dataStructure, geom, geomGroup, Legacy::HexListName, preflight);

  geom->setVertices(*sharedVertexList.value());
  geom->setPolyhedraList(*sharedHexList.value());

  return geom;
}

DataObject* readLegacyEdgeGeom(DataStructure& dataStructure, const nx::core::HDF5::GroupIO& geomGroup, const std::string& name, bool preflight)
{
  auto geom = EdgeGeom::Create(dataStructure, name);
  readGenericGeomDims(geom, geomGroup);
  auto sharedVertexList = readLegacyGeomArrayAs<Float32Array>(dataStructure, geom, geomGroup, Legacy::VertexListName, preflight);
  auto sharedEdgeList = readLegacyNodeConnectivityList(dataStructure, geom, geomGroup, Legacy::EdgeListName, preflight);

  geom->setVertices(*sharedVertexList.value());
  geom->setEdgeList(*sharedEdgeList.value());

  return geom;
}

DataObject* readLegacyImageGeom(DataStructure& dataStructure, const nx::core::HDF5::GroupIO& geomGroup, const std::string& name)
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

Result<> readLegacyDataContainer(DataStructure& dataStructure, const nx::core::HDF5::GroupIO& dcGroup, bool preflight = false, bool importChildren = true)
{
  DataObject* container = nullptr;
  const std::string dcName = dcGroup.getName();

  // Check for geometry
  auto geomGroup = dcGroup.openGroup(Legacy::GeometryTag.c_str());
  if(geomGroup.isValid())
  {
    std::string geomName;
    auto geomNameResult = geomGroup.readStringAttribute(Legacy::GeometryTypeNameTag);
    if(geomNameResult.invalid())
    {
      return ConvertResult(std::move(geomNameResult));
    }
    geomName = std::move(geomNameResult.value());
    if(geomName == Legacy::Type::ImageGeom)
    {
      container = readLegacyImageGeom(dataStructure, geomGroup, dcName);
    }
    else if(geomName == Legacy::Type::EdgeGeom)
    {
      container = readLegacyEdgeGeom(dataStructure, geomGroup, dcName, false);
    }
    else if(geomName == Legacy::Type::HexGeom)
    {
      container = readLegacyHexGeom(dataStructure, geomGroup, dcName, false);
    }
    else if(geomName == Legacy::Type::QuadGeom)
    {
      container = readLegacyQuadGeom(dataStructure, geomGroup, dcName, false);
    }
    else if(geomName == Legacy::Type::RectGridGeom)
    {
      container = readLegacyRectGridGeom(dataStructure, geomGroup, dcName, false);
    }
    else if(geomName == Legacy::Type::TetrahedralGeom)
    {
      container = readLegacyTetrahedralGeom(dataStructure, geomGroup, dcName, false);
    }
    else if(geomName == Legacy::Type::TriangleGeom)
    {
      container = readLegacyTriangleGeom(dataStructure, geomGroup, dcName, false);
    }
    else if(geomName == Legacy::Type::VertexGeom)
    {
      container = readLegacyVertexGeom(dataStructure, geomGroup, dcName, false);
    }
  }

  // No geometry found. Create a DataGroup instead
  if(!container)
  {
    container = DataGroup::Create(dataStructure, dcName);
  }

  if(!importChildren)
  {
    return {};
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

Result<> finishImportingLegacyImageGeom(DataStructure& dataStructure, IGeometry* geometry, const HDF5::GroupIO& geometryReader)
{
  auto* imageGeom = dynamic_cast<ImageGeom*>(geometry);
  if(imageGeom == nullptr)
  {
    return MakeErrorResult(-502678, "Failed to finish importing legacy Image Geometry. Existing geometry is not of the correct type");
  }

  return {};
}

Result<> finishImportingLegacyEdgeGeom(DataStructure& dataStructure, IGeometry* geometryPtr, const HDF5::GroupIO& geomGroup)
{
  auto* edgeGeom = dynamic_cast<EdgeGeom*>(geometryPtr);
  if(edgeGeom == nullptr)
  {
    return MakeErrorResult(-502677, "Failed to finish importing legacy Edge Geometry. Existing geometry is not of the correct type");
  }

  readGenericGeomDims(edgeGeom, geomGroup);
  return {};
}

Result<> finishImportingLegacyHexGeom(DataStructure& dataStructure, IGeometry* geometryPtr, const HDF5::GroupIO& geomGroup)
{
  auto* hexGeom = dynamic_cast<HexahedralGeom*>(geometryPtr);
  if(hexGeom == nullptr)
  {
    return MakeErrorResult(-502676, "Failed to finish importing legacy Hex Geometry. Existing geometry is not of the correct type");
  }

  readGenericGeomDims(hexGeom, geomGroup);
  return {};
}

Result<> finishImportingLegacyQuadGeom(DataStructure& dataStructure, IGeometry* geometryPtr, const HDF5::GroupIO& geomGroup)
{
  auto* quadGeom = dynamic_cast<QuadGeom*>(geometryPtr);
  if(quadGeom == nullptr)
  {
    return MakeErrorResult(-502675, "Failed to finish importing legacy Quad Geometry. Existing geometry is not of the correct type");
  }

  readGenericGeomDims(quadGeom, geomGroup);
  return {};
}

Result<> finishImportingLegacyRectGridGeom(DataStructure& dataStructure, IGeometry* geometryPtr, const HDF5::GroupIO& geomGroup)
{
  auto* rectGridGeom = dynamic_cast<RectGridGeom*>(geometryPtr);
  if(rectGridGeom == nullptr)
  {
    return MakeErrorResult(-502674, "Failed to finish importing legacy RectGrid Geometry. Existing geometry is not of the correct type");
  }

  readGenericGeomDims(rectGridGeom, geomGroup);

  // DIMENSIONS array
  {
    auto dimsDataset = geomGroup.openDataset("DIMENSIONS");
    auto dims = dimsDataset.readAsVector<int64>();
    rectGridGeom->setDimensions(SizeVec3(dims[0], dims[1], dims[2]));
  }

  return {};
}

Result<> finishImportingLegacyTetrahedralGeom(DataStructure& dataStructure, IGeometry* geometryPtr, const HDF5::GroupIO& geomGroup)
{
  auto* tetrahedralGeom = dynamic_cast<TetrahedralGeom*>(geometryPtr);
  if(tetrahedralGeom == nullptr)
  {
    return MakeErrorResult(-502673, "Failed to finish importing legacy Tetrahedral Geometry. Existing geometry is not of the correct type");
  }

  readGenericGeomDims(tetrahedralGeom, geomGroup);
  return {};
}

Result<> finishImportingLegacyTriangleGeom(DataStructure& dataStructure, IGeometry* geometryPtr, const HDF5::GroupIO& geomGroup)
{
  auto* triangleGeom = dynamic_cast<TriangleGeom*>(geometryPtr);
  if(triangleGeom == nullptr)
  {
    return MakeErrorResult(-502672, "Failed to finish importing legacy Triangle Geometry. Existing geometry is not of the correct type");
  }

  readGenericGeomDims(triangleGeom, geomGroup);
  return {};
}

Result<> finishImportingLegacyVertexGeom(DataStructure& dataStructure, IGeometry* geometryPtr, const HDF5::GroupIO& geomGroup)
{
  auto* vertexGeom = dynamic_cast<VertexGeom*>(geometryPtr);
  if(vertexGeom == nullptr)
  {
    return MakeErrorResult(-502671, "Failed to finish importing legacy Vertex Geometry. Existing geometry is not of the correct type");
  }

  readGenericGeomDims(vertexGeom, geomGroup);
  return {};
}

Result<> finishImportingLegacyDataContainer(DataStructure& dataStructure, const HDF5::GroupIO& parentReader, const DataPath& dataPath)
{
  std::string dcName = dataPath[0];
  auto dcGroup = parentReader.openGroup(dcName);

  // Check for geometry
  auto geomGroup = dcGroup.openGroup(Legacy::GeometryTag.c_str());
  if(geomGroup.isValid())
  {
    auto* geometryPtr = dataStructure.getDataAs<IGeometry>(dataPath);
    if(geometryPtr == nullptr)
    {
      return MakeErrorResult(-502679, fmt::format("Failed to finish importing of geometry at path '{}'. Existing geometry not found.", dataPath.toString()));
    }

    std::string geomName;
    auto geomNameResult = geomGroup.readStringAttribute(Legacy::GeometryTypeNameTag);
    if(geomNameResult.invalid())
    {
      return ConvertResult(std::move(geomNameResult));
    }
    geomName = std::move(geomNameResult.value());
    if(geomName == Legacy::Type::ImageGeom)
    {
      return finishImportingLegacyImageGeom(dataStructure, geometryPtr, geomGroup);
    }
    else if(geomName == Legacy::Type::EdgeGeom)
    {
      return finishImportingLegacyEdgeGeom(dataStructure, geometryPtr, geomGroup);
    }
    else if(geomName == Legacy::Type::HexGeom)
    {
      return finishImportingLegacyHexGeom(dataStructure, geometryPtr, geomGroup);
    }
    else if(geomName == Legacy::Type::QuadGeom)
    {
      return finishImportingLegacyQuadGeom(dataStructure, geometryPtr, geomGroup);
    }
    else if(geomName == Legacy::Type::RectGridGeom)
    {
      return finishImportingLegacyRectGridGeom(dataStructure, geometryPtr, geomGroup);
    }
    else if(geomName == Legacy::Type::TetrahedralGeom)
    {
      return finishImportingLegacyTetrahedralGeom(dataStructure, geometryPtr, geomGroup);
    }
    else if(geomName == Legacy::Type::TriangleGeom)
    {
      return finishImportingLegacyTriangleGeom(dataStructure, geometryPtr, geomGroup);
    }
    else if(geomName == Legacy::Type::VertexGeom)
    {
      return finishImportingLegacyVertexGeom(dataStructure, geometryPtr, geomGroup);
    }
  }

  return {};
}

Result<std::vector<std::shared_ptr<DataObject>>> ImportLegacyDataObjectFromFile(const nx::core::HDF5::FileIO& fileReader, const DataPath& dataPath)
{
  DataStructure dataStructure;
  auto dcaGroup = fileReader.openGroup(k_LegacyDataStructureGroupTag);

  Result<> result;
  switch(dataPath.getLength())
  {
  case 1: {
    auto dcGroup = dcaGroup.openGroup(dataPath.toString());
    result = readLegacyDataContainer(dataStructure, dcGroup, false, false);
    break;
  }
  case 2: {
    auto attributeMatrixGroup = dcaGroup.openGroup(dataPath.toString());
    DataPath dcPath({dataPath[0]});
    dataStructure.makePath(dcPath);
    auto& container = dataStructure.getDataRef(dcPath);
    result = readLegacyAttributeMatrix(dataStructure, attributeMatrixGroup, container, false, false);
    break;
  }
  case 3: {
    auto dcGroup = dcaGroup.openGroup(dataPath[0]);
    auto attributeMatrixGroup = dcGroup.openGroup(dataPath[1]);
    result = readLegacyArray(dataStructure, attributeMatrixGroup, dataPath[2], false);
    break;
  }
  default:
    return MakeErrorResult<std::vector<std::shared_ptr<DataObject>>>(-59040,
                                                                     fmt::format("Failed to import DataObject at path '{}'. DataPath not supported by legacy DataStructure.", dataPath.toString()));
  }

  if(result.invalid())
  {
    return ConvertInvalidResult<std::vector<std::shared_ptr<DataObject>>>(std::move(result));
  }

  const DataMap& dataMap = dataStructure.getDataMap();
  if(dataMap.getSize() == 0)
  {
    return MakeErrorResult<std::vector<std::shared_ptr<DataObject>>>(-69040, fmt::format("Failed to import DataObject at path '{}'", dataPath.toString()));
  }

  auto item = dataMap.begin();
  return {std::vector<std::shared_ptr<DataObject>>{(*item).second}};
}

Result<> FinishImportingLegacyDataObject(DataStructure& dataStructure, const nx::core::HDF5::GroupIO& parentReader, const DataPath& dataPath)
{
  // Statistics data is fully imported during the initial read (readLegacyStatsDataArray
  // always imports with preflight=false), so skip the finish-importing step for all
  // Statistics paths. The Statistics group is placed as a sibling of the AttributeMatrix
  // under the DataContainer, so any path with "Statistics" at index 1 is part of this hierarchy.
  if(dataPath.getLength() >= 2 && dataPath[1] == "Statistics")
  {
    return {};
  }

  switch(dataPath.getLength())
  {
  case 1:
    finishImportingLegacyDataContainer(dataStructure, parentReader, dataPath);
    break;
  case 2:
    break;
  case 3:
    finishImportingLegacyArray(dataStructure, parentReader, dataPath);
    break;
  default:
    return MakeErrorResult(-520156, fmt::format("Could not read legacy DREAM3D data at path '{}'", dataPath.toString()));
    break;
  }
  return {};
}

Result<DataStructure> ImportLegacyDataStructure(const nx::core::HDF5::FileIO& fileReader, bool preflight)
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

Result<DataStructure> DREAM3D::ImportDataStructureFromFile(const nx::core::HDF5::FileIO& fileReader, bool preflight)
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
  return MakeErrorResult<DataStructure>(k_InvalidDataStructureVersion, fmt::format("Could not parse DataStructure version {}. Expected versions: {} or {}. Actual value: {}", fileVersion,
                                                                                   k_CurrentFileVersion, k_LegacyFileVersion, fileVersion));
}

Result<DataStructure> DREAM3D::ImportDataStructureFromFile(const std::filesystem::path& filePath, bool preflight)
{
  auto fileReader = nx::core::HDF5::FileIO::ReadFile(filePath);
  if(!fileReader.isValid())
  {
    return MakeErrorResult<DataStructure>(-1, fmt::format("DREAM3D::ImportDataStructureFromFile: Unable to open '{}' for reading", filePath.string()));
  }

  return ImportDataStructureFromFile(fileReader, preflight);
}

Result<Pipeline> DREAM3D::ImportPipelineFromFile(const nx::core::HDF5::FileIO& fileReader)
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

Result<nlohmann::json> DREAM3D::ImportPipelineJsonFromFile(const nx::core::HDF5::FileIO& fileReader)
{
  auto pipelineGroupReader = fileReader.openGroup(k_PipelineJsonTag);

  auto pipelineDatasetReader = pipelineGroupReader.openDataset(k_PipelineJsonTag);
  auto pipelineJsonString = pipelineDatasetReader.readAsString();
  if(pipelineJsonString.empty())
  {
    return {nlohmann::json()};
  }
  return {nlohmann::json::parse(pipelineJsonString)};
}

Result<Pipeline> DREAM3D::ImportPipelineFromFile(const std::filesystem::path& filePath)
{
  if(!std::filesystem::exists(filePath))
  {
    return MakeErrorResult<Pipeline>(-1, fmt::format("DREAM3D::ImportPipelineFromFile: File does not exist. '{}'", filePath.string()));
  }
  auto fileReader = nx::core::HDF5::FileIO::ReadFile(filePath);
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
  auto fileReader = nx::core::HDF5::FileIO::ReadFile(filePath);
  if(!fileReader.isValid())
  {
    return MakeErrorResult<nlohmann::json>(-1, fmt::format("DREAM3D::ImportPipelineFromFile: Unable to open '{}' for reading", filePath.string()));
  }

  return ImportPipelineJsonFromFile(fileReader);
}

Result<std::shared_ptr<DataObject>> DREAM3D::ImportDataObjectFromFile(const nx::core::HDF5::FileIO& fileReader, const DataPath& dataPath)
{
  const auto fileVersion = GetFileVersion(fileReader);
  if(fileVersion == k_CurrentFileVersion)
  {
    return HDF5::DataStructureReader::ReadObject(fileReader, dataPath);
  }
  else if(fileVersion == k_LegacyFileVersion)
  {
    auto result = ImportLegacyDataObjectFromFile(fileReader, dataPath);
    if(result.invalid())
    {
      return ConvertInvalidResult<std::shared_ptr<DataObject>>(std::move(result));
    }
    std::vector<std::shared_ptr<DataObject>> value = result.value();
    if(value.size() != 0)
    {
      return MakeErrorResult<std::shared_ptr<DataObject>>(-48264, fmt::format("Error extracting a single DataObject from legacy DREAM3D file at path '{}'", dataPath.toString()));
    }
    return {result.value().front()};
  }
  return MakeErrorResult<std::shared_ptr<DataObject>>(-523242, fmt::format("Error extracting a single DataObject from legacy DREAM3D file at path '{}'", dataPath.toString()));
}

Result<std::vector<std::shared_ptr<DataObject>>> DREAM3D::ImportSelectDataObjectsFromFile(const nx::core::HDF5::FileIO& fileReader, const std::vector<DataPath>& dataPaths)
{
  std::vector<std::shared_ptr<DataObject>> dataObjects;
  for(const DataPath& dataPath : dataPaths)
  {
    auto importResult = ImportDataObjectFromFile(fileReader, dataPath);
    if(importResult.invalid())
    {
      return ConvertInvalidResult<std::vector<std::shared_ptr<DataObject>>>(std::move(importResult));
    }
    dataObjects.push_back(std::move(importResult.value()));
  }

  return {dataObjects};
}

Result<> DREAM3D::FinishImportingObjectPreflight(DataStructure& importStructure, DataStructure& dataStructure, const DataPath& dataPath)
{
  if(!importStructure.containsData(dataPath))
  {
    return MakeErrorResult(-6200, fmt::format("DataStructure Object Path '{}' does not exist for importing.", dataPath.toString()));
  }
  const auto importObject = importStructure.getSharedData(dataPath);
  const auto importData = std::shared_ptr<DataObject>(importObject->shallowCopy());
  // Clear all children before inserting into the DataStructure
  if(const auto importGroup = std::dynamic_pointer_cast<BaseGroup>(importData); importGroup != nullptr)
  {
    importGroup->clear();
  }

  if(!dataStructure.insert(importData, dataPath.getParent()))
  {
    return MakeErrorResult(-6202, fmt::format("Unable to insert DataObject at DatPath '{}' into the DataStructure", dataPath.toString()));
  }
  return {};
}

Result<> DREAM3D::FinishImportingObject(DataStructure& importStructure, DataStructure& dataStructure, const DataPath& dataPath, const nx::core::HDF5::FileIO& fileReader, bool preflight)
{
  // Insert the (metadata-only) object first; in preflight mode this is all the
  // work there is. The bulk-array read below is skipped for preflight because
  // preflight never populates array contents.
  Result<> insertResult = FinishImportingObjectPreflight(importStructure, dataStructure, dataPath);
  if(insertResult.invalid() || preflight)
  {
    return insertResult;
  }

  const auto dataPtr = dataStructure.getSharedData(dataPath);
  if(dataPtr == nullptr)
  {
    return MakeErrorResult(-1502234, fmt::format("Cannot finish importing HDF5 data at DataPath '{}'. DataObject does not exist to copy data into.", dataPath.toString()));
  }

  const auto fileVersion = GetFileVersion(fileReader);
  if(fileVersion == k_CurrentFileVersion)
  {
    return HDF5::DataStructureReader::FinishImportingObject(dataStructure, fileReader, dataPath);
  }
  else if(fileVersion == k_LegacyFileVersion)
  {
    const auto dataStructureReader = fileReader.openGroup(k_LegacyDataStructureGroupTag);
    return FinishImportingLegacyDataObject(dataStructure, dataStructureReader, dataPath);
  }
  return {};
}

Result<DREAM3D::FileData> DREAM3D::ReadFile(const nx::core::HDF5::FileIO& fileReader, bool preflight)
{
  // Pipeline pipeline;
  auto pipeline = ImportPipelineFromFile(fileReader);
  if(pipeline.invalid())
  {
    return {{nonstd::make_unexpected(std::move(pipeline.errors()))}, std::move(pipeline.warnings())};
  }

  auto dataStructure = ImportDataStructureFromFile(fileReader, preflight);
  if(dataStructure.invalid())
  {
    return {{nonstd::make_unexpected(std::move(dataStructure.errors()))}, std::move(dataStructure.warnings())};
  }

  return {DREAM3D::FileData{std::move(pipeline.value()), std::move(dataStructure.value())}};
}

Result<DREAM3D::FileData> DREAM3D::ReadFile(const std::filesystem::path& path)
{
  auto reader = nx::core::HDF5::FileIO::ReadFile(path);
  nx::core::HDF5::ErrorType error = 0;

  Result<FileData> fileData = ReadFile(reader, error);
  if(error < 0)
  {
    return MakeErrorResult<FileData>(-1, fmt::format("DREAM3D::ReadFile: Unable to read '{}'", path.string()));
  }
  return fileData;
}

Result<> WritePipeline(nx::core::HDF5::FileIO& fileWriter, const Pipeline& pipeline)
{
  if(!fileWriter.isValid())
  {
    return MakeErrorResult(-100, "Cannot Write to Invalid FileWriter");
  }

  auto pipelineGroupWriter = fileWriter.createGroup(k_PipelineJsonTag);
  if(Result<> result = pipelineGroupWriter.writeScalarAttribute(k_PipelineVersionTag, static_cast<DREAM3D::PipelineVersionType>(k_CurrentPipelineVersion)); result.invalid())
  {
    return result;
  }
  if(Result<> result = pipelineGroupWriter.writeStringAttribute(k_PipelineNameTag, pipeline.getName()); result.invalid())
  {
    return result;
  }

  auto pipelineDatasetWriter = pipelineGroupWriter.createDataset(k_PipelineJsonTag);
  std::string pipelineString = pipeline.toJson().dump();
  return pipelineDatasetWriter.writeString(pipelineString);
}

Result<> WriteDataStructure(nx::core::HDF5::FileIO& fileWriter, const DataStructure& dataStructure, const nx::core::HDF5::DataStructureWriter::WriteOptions& options)
{
  return HDF5::DataStructureWriter::WriteFile(dataStructure, fileWriter, options);
}

Result<> WriteFileVersion(nx::core::HDF5::FileIO& fileWriter)
{
  return fileWriter.writeStringAttribute(k_FileVersionTag, DREAM3D::k_CurrentFileVersion.str());
}

Result<> DREAM3D::WriteFile(nx::core::HDF5::FileIO& fileWriter, const FileData& fileData)
{
  return WriteFile(fileWriter, fileData.first, fileData.second);
}

Result<> DREAM3D::WriteFile(nx::core::HDF5::FileIO& fileWriter, const Pipeline& pipeline, const DataStructure& dataStructure)
{
  return WriteFile(fileWriter, pipeline, dataStructure, nx::core::HDF5::DataStructureWriter::WriteOptions{});
}

Result<> DREAM3D::WriteFile(nx::core::HDF5::FileIO& fileWriter, const Pipeline& pipeline, const DataStructure& dataStructure, const nx::core::HDF5::DataStructureWriter::WriteOptions& options)
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
  return WriteDataStructure(fileWriter, dataStructure, options);
}

Result<> DREAM3D::WriteFile(const std::filesystem::path& path, const DataStructure& dataStructure, const Pipeline& pipeline, bool writeXdmf)
{
  return WriteFile(path, dataStructure, pipeline, writeXdmf, nx::core::HDF5::DataStructureWriter::WriteOptions{});
}

Result<> DREAM3D::WriteFile(const std::filesystem::path& path, const DataStructure& dataStructure, const Pipeline& pipeline, bool writeXdmf,
                            const nx::core::HDF5::DataStructureWriter::WriteOptions& options)
{
  auto fileWriter = nx::core::HDF5::FileIO::WriteFile(path);
  if(!fileWriter.isValid())
  {
    return MakeErrorResult(-9045, fmt::format("Failed to create DREAM3D file at path {}", path.string()));
  }

  auto result = WriteFile(fileWriter, pipeline, dataStructure, options);
  if(result.invalid())
  {
    return result;
  }

  if(writeXdmf)
  {
    std::filesystem::path xdmfFilePath = std::filesystem::path(path).replace_extension(".xdmf");
    WriteXdmf(xdmfFilePath, dataStructure, path.filename().string());
  }

  return {};
}

Result<> DREAM3D::AppendFile(const std::filesystem::path& path, const DataStructure& dataStructure, const DataPath& dataPath)
{
  auto file = nx::core::HDF5::FileIO::AppendFile(path);
  if(!file.isValid())
  {
    return MakeErrorResult(-1, fmt::format("DREAM3D::AppendFile: Unable to open '{}' for appending", path.string()));
  }

  const auto fileVersion = GetFileVersion(file);
  if(fileVersion != k_CurrentFileVersion)
  {
    return MakeErrorResult(-2, fmt::format("DREAM3D::AppendFile: Incompatible file version '{}'. Expected '{}'", fileVersion, k_CurrentFileVersion));
  }
  return HDF5::DataStructureWriter::AppendFile(file, dataStructure, dataPath);
}

std::vector<nx::core::DataPath> DREAM3D::ExpandSelectedPathsToAncestors(const std::vector<nx::core::DataPath>& selectedPaths)
{
  std::vector<nx::core::DataPath> finalDataPaths;
  for(const auto& dataPath : selectedPaths)
  {
    auto pathVector = dataPath.getPathVector();
    for(size_t i = 1; i <= dataPath.getLength(); ++i)
    {
      auto dataPathPart = nx::core::DataPath(std::vector<std::string>(pathVector.begin(), pathVector.begin() + i));
      if(std::find(finalDataPaths.begin(), finalDataPaths.end(), dataPathPart) == finalDataPaths.end())
      {
        finalDataPaths.push_back(dataPathPart);
      }
    }
  }

  return finalDataPaths;
}

std::vector<nx::core::DataPath> DREAM3D::ExpandSelectedPathsToDescendants(const std::vector<nx::core::DataPath>& selectedPaths, const std::vector<nx::core::DataPath>& allPaths)
{
  std::vector<nx::core::DataPath> expandedDataPaths = selectedPaths;
  for(const auto& dataPath : selectedPaths)
  {
    for(const auto& candidateDataPath : allPaths)
    {
      if(candidateDataPath.getLength() <= dataPath.getLength())
      {
        continue;
      }

      bool isEqual = true;
      for(size_t i = 0; i < dataPath.getPathVector().size(); ++i)
      {
        if(dataPath.getPathVector()[i] != candidateDataPath.getPathVector()[i])
        {
          isEqual = false;
        }
      }
      if(isEqual)
      {
        expandedDataPaths.push_back(candidateDataPath);
      }
    }
  }

  return expandedDataPaths;
}
