#include "SimplnxCore/Filters/Algorithms/CombineNodeBasedGeometries.hpp"
#include "SimplnxCore/Filters/CombineNodeBasedGeometriesFilter.hpp"

#include "simplnx/DataStructure/Geometry/EdgeGeom.hpp"
#include "simplnx/DataStructure/Geometry/HexahedralGeom.hpp"
#include "simplnx/DataStructure/Geometry/QuadGeom.hpp"
#include "simplnx/DataStructure/Geometry/TetrahedralGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;

const std::string k_Geometry1Name = "Geometry 1";
const DataPath k_InputGeometry1Path = DataPath({k_Geometry1Name});
const std::string k_Geometry2Name = "Geometry 2";
const DataPath k_InputGeometry2Path = DataPath({k_Geometry2Name});
const std::string k_Geometry3Name = "Geometry 3";
const DataPath k_InputGeometry3Path = DataPath({k_Geometry3Name});
const std::string k_Geometry4Name = "Geometry 4";
const DataPath k_InputGeometry4Path = DataPath({k_Geometry4Name});
const std::string k_Geometry5Name = "Geometry 5";
const DataPath k_InputGeometry5Path = DataPath({k_Geometry5Name});
const usize k_NeighborListListSize = 5;
const DataPath k_OutputGeometryPath1 = DataPath({"Combined Geometry 1"});
const DataPath k_OutputGeometryPath2 = DataPath({"Combined Geometry 2"});
const DataPath k_OutputGeometryPath3 = DataPath({"Combined Geometry 3"});
const DataPath k_OutputGeometryPath4 = DataPath({"Combined Geometry 4"});

enum class DataInitOption : uint8
{
  None,
  ElementsOnly,
  ElementsAndNoData,
  ElementsAndData
};

std::vector<std::string> k_DataInitOptionStrings = {"None", "Elements Only", "Elements & No Data", "Elements & Data"};

std::string GetDataInitOptionString(const DataInitOption& initOption)
{
  return k_DataInitOptionStrings[to_underlying(initOption)];
}

const std::vector<DataInitOption> k_DataInitOptions = {DataInitOption::None, DataInitOption::ElementsOnly, DataInitOption::ElementsAndNoData, DataInitOption::ElementsAndData};

struct GeometryInitOptions
{
  DataInitOption vertex;
  DataInitOption edge;
  DataInitOption face;
  DataInitOption poly;
};

// Overload the stream operator for easy logging with CAPTURE or INFO.
std::ostream& operator<<(std::ostream& os, const GeometryInitOptions& opts)
{
  os << "Vertex: " << GetDataInitOptionString(opts.vertex) << ", Edge: " << GetDataInitOptionString(opts.edge) << ", Face: " << GetDataInitOptionString(opts.face)
     << ", Poly: " << GetDataInitOptionString(opts.poly);
  return os;
}

template <typename T>
DataArray<T>* CreateDataArray(AttributeMatrix& attrMatrix, DataStructure& dataStructure, const std::string& arrayName, const std::vector<usize>& cDims)
{
  return DataArray<T>::template CreateWithStore<DataStore<T>>(dataStructure, arrayName, {attrMatrix.getNumTuples()}, cDims, attrMatrix.getId());
}

template <typename T, usize CompCount>
DataArray<T>* CreateDataArray(AttributeMatrix& attrMatrix, DataStructure& dataStructure, const std::string& arrayName)
{
  return CreateDataArray<T>(attrMatrix, dataStructure, arrayName, {CompCount});
}

template <typename T, usize CompCount>
void AddDataArray(AttributeMatrix& attrMatrix, DataStructure& dataStructure)
{
  DataType dataType = GetDataType<T>();
  std::string dataTypeStr = DataTypeToString(dataType);
  auto arr = CreateDataArray<T, CompCount>(attrMatrix, dataStructure, fmt::format("{}_Array_{}", dataTypeStr, CompCount));
  std::iota(arr->begin(), arr->end(), static_cast<T>(0));
}

StringArray* CreateStringArray(AttributeMatrix& attrMatrix, DataStructure& dataStructure, const std::string& arrayName)
{
  return StringArray::CreateWithValues(dataStructure, arrayName, attrMatrix.getShape(), std::vector<std::string>(attrMatrix.getNumTuples()), attrMatrix.getId());
}

void AddStringArray(AttributeMatrix& attrMatrix, DataStructure& dataStructure)
{
  auto strArray = CreateStringArray(attrMatrix, dataStructure, "StringArray");
  int counter = 1;
  std::generate(strArray->begin(), strArray->end(), [&counter]() { return "Item" + std::to_string(counter++); });
}

template <typename T>
NeighborList<T>* CreateNeighborList(AttributeMatrix& attrMatrix, DataStructure& dataStructure, const std::string& nlName)
{
  return NeighborList<T>::Create(dataStructure, nlName, attrMatrix.getShape(), attrMatrix.getId());
}

template <typename T>
void AddNeighborList(AttributeMatrix& attrMatrix, DataStructure& dataStructure)
{
  DataType dataType = GetDataType<T>();
  std::string dataTypeStr = DataTypeToString(dataType);
  auto nl = CreateNeighborList<T>(attrMatrix, dataStructure, fmt::format("{}_NeighborList", dataTypeStr));
  for(usize i = 0; i < attrMatrix.getNumTuples(); i++)
  {
    typename NeighborList<T>::SharedVectorType inputList(new std::vector<T>(k_NeighborListListSize));
    std::iota(inputList->begin(), inputList->end(), 0);
    nl->setList(i, inputList);
  }
}

template <usize CompCount>
void CreateDataArrays(AttributeMatrix& attrMatrix, DataStructure& dataStructure)
{
  AddDataArray<int8, CompCount>(attrMatrix, dataStructure);
  AddDataArray<uint8, CompCount>(attrMatrix, dataStructure);
  AddDataArray<int16, CompCount>(attrMatrix, dataStructure);
  AddDataArray<uint16, CompCount>(attrMatrix, dataStructure);
  AddDataArray<int32, CompCount>(attrMatrix, dataStructure);
  AddDataArray<uint32, CompCount>(attrMatrix, dataStructure);
  AddDataArray<int64, CompCount>(attrMatrix, dataStructure);
  AddDataArray<uint64, CompCount>(attrMatrix, dataStructure);
  AddDataArray<float32, CompCount>(attrMatrix, dataStructure);
  AddDataArray<float64, CompCount>(attrMatrix, dataStructure);
}

void CreateDataArrays(AttributeMatrix& attrMatrix, DataStructure& dataStructure)
{
  CreateDataArrays<1>(attrMatrix, dataStructure);
  CreateDataArrays<2>(attrMatrix, dataStructure);
  CreateDataArrays<3>(attrMatrix, dataStructure);

  AddStringArray(attrMatrix, dataStructure);

  AddNeighborList<int8>(attrMatrix, dataStructure);
  AddNeighborList<uint8>(attrMatrix, dataStructure);
  AddNeighborList<int16>(attrMatrix, dataStructure);
  AddNeighborList<uint16>(attrMatrix, dataStructure);
  AddNeighborList<int32>(attrMatrix, dataStructure);
  AddNeighborList<uint32>(attrMatrix, dataStructure);
  AddNeighborList<int64>(attrMatrix, dataStructure);
  AddNeighborList<uint64>(attrMatrix, dataStructure);
  AddNeighborList<float32>(attrMatrix, dataStructure);
  AddNeighborList<float64>(attrMatrix, dataStructure);
}

AttributeMatrix* CreateNodeAttributeMatrix(IGeometry& geom, DataStructure& dataStructure, const std::string& attrMatrixName, usize numElements)
{
  return AttributeMatrix::Create(dataStructure, attrMatrixName, {numElements}, geom.getId());
}

template <typename TGeom, typename TSetAttrMatrix>
void CreateNodeAttributeMatrix(TGeom& geom, DataStructure& dataStructure, const std::string& attrMatrixName, usize numElements, TSetAttrMatrix setAttrMatrix)
{
  auto attrMatrix = CreateNodeAttributeMatrix(geom, dataStructure, attrMatrixName, numElements);
  setAttrMatrix(geom, *attrMatrix);
}

template <typename TNode>
DataArray<TNode>* CreateNodeArray(IGeometry& geom, DataStructure& dataStructure, const std::string& sharedListName, usize numElements, usize numPerElement)
{
  return DataArray<TNode>::template CreateWithStore<DataStore<TNode>>(dataStructure, sharedListName, {numElements}, {numPerElement}, geom.getId());
}

template <typename TGeom, typename TNode, typename TGetNumPerElement, typename TSetNodeArray>
void CreateNodeArray(TGeom& geom, DataStructure& dataStructure, const std::string& sharedListName, usize numElements, TGetNumPerElement getNumPerElement, TSetNodeArray setNodeArray)
{
  auto arr = CreateNodeArray<TNode>(geom, dataStructure, sharedListName, numElements, getNumPerElement(geom));
  std::iota(arr->begin(), arr->end(), 0);
  setNodeArray(geom, *arr);
}

template <typename TGeom, typename TNode, typename TGetNumPerElement, typename TGetNodeArray, typename TSetNodeArray, typename TGetAttrMatrix, typename TSetAttrMatrix>
void InitializeNodeGeometry(TGeom& geom, DataStructure& dataStructure, DataInitOption initOption, const std::string& attrMatrixName, const std::string& sharedListName, usize numElements,
                            TGetNumPerElement getNumPerElement, TGetNodeArray getNodeArray, TSetNodeArray setNodeArray, TGetAttrMatrix getAttrMatrix, TSetAttrMatrix setAttrMatrix)
{
  if(initOption == DataInitOption::None)
  {
    return;
  }

  if(getNodeArray(geom) == nullptr)
  {
    CreateNodeArray<TGeom, TNode>(geom, dataStructure, sharedListName, numElements, getNumPerElement, setNodeArray);
  }

  if(initOption == DataInitOption::ElementsAndNoData || initOption == DataInitOption::ElementsAndData)
  {
    auto* attrMatrix = getAttrMatrix(geom);
    if(attrMatrix == nullptr)
    {
      CreateNodeAttributeMatrix(geom, dataStructure, attrMatrixName, numElements, setAttrMatrix);
      attrMatrix = getAttrMatrix(geom);
    }
    if(initOption == DataInitOption::ElementsAndData)
    {
      CreateDataArrays(*attrMatrix, dataStructure);
    }
  }
}

void InitializeNode0DGeometry(INodeGeometry0D& geom, DataStructure& dataStructure, DataInitOption vertexDataInitOption)
{
  usize numOfVertexComps = 3;
  InitializeNodeGeometry<INodeGeometry0D, float32>(
      geom, dataStructure, vertexDataInitOption, INodeGeometry0D::k_VertexAttributeMatrixName, INodeGeometry0D::k_SharedVertexListName, geom.getNumberOfVertices(),
      [numOfVertexComps](const INodeGeometry0D& g) { return numOfVertexComps; }, [](INodeGeometry0D& g) { return g.getVertices(); },
      [](INodeGeometry0D& g, const Float32Array& da) { g.setVertices(da); }, [](INodeGeometry0D& g) { return g.getVertexAttributeMatrix(); },
      [](INodeGeometry0D& g, AttributeMatrix& m) { g.setVertexAttributeMatrix(m); });
}

void InitializeNode1DGeometry(INodeGeometry1D& geom, DataStructure& dataStructure, DataInitOption vertexDataInitOption, DataInitOption edgeDataInitOption)
{
  InitializeNode0DGeometry(geom, dataStructure, vertexDataInitOption);

  usize numOfEdges = geom.getNumberOfVertices() / geom.getNumberOfVerticesPerEdge();
  InitializeNodeGeometry<INodeGeometry1D, uint64>(
      geom, dataStructure, edgeDataInitOption, INodeGeometry1D::k_EdgeAttributeMatrixName, INodeGeometry1D::k_SharedEdgeListName, numOfEdges,
      [](const INodeGeometry1D& g) { return g.getNumberOfVerticesPerEdge(); }, [](INodeGeometry1D& g) { return g.getEdges(); },
      [](INodeGeometry1D& g, const IGeometry::SharedEdgeList& da) { g.setEdgeList(da); }, [](INodeGeometry1D& g) { return g.getEdgeAttributeMatrix(); },
      [](INodeGeometry1D& g, AttributeMatrix& m) { g.setEdgeAttributeMatrix(m); });
}

void InitializeNode2DGeometry(INodeGeometry2D& geom, DataStructure& dataStructure, DataInitOption vertexDataInitOption, DataInitOption edgeDataInitOption, DataInitOption faceDataInitOption)
{
  InitializeNode1DGeometry(geom, dataStructure, vertexDataInitOption, edgeDataInitOption);

  usize numOfFaces = geom.getNumberOfVertices() / geom.getNumberOfVerticesPerFace();
  InitializeNodeGeometry<INodeGeometry2D, uint64>(
      geom, dataStructure, faceDataInitOption, INodeGeometry2D::k_FaceAttributeMatrixName, INodeGeometry2D::k_SharedFacesListName, numOfFaces,
      [](const INodeGeometry2D& g) { return g.getNumberOfVerticesPerFace(); }, [](INodeGeometry2D& g) { return g.getFaces(); },
      [](INodeGeometry2D& g, const IGeometry::SharedFaceList& da) { g.setFaceList(da); }, [](INodeGeometry2D& g) { return g.getFaceAttributeMatrix(); },
      [](INodeGeometry2D& g, AttributeMatrix& m) { g.setFaceAttributeMatrix(m); });
}

void InitializeNode3DGeometry(INodeGeometry3D& geom, DataStructure& dataStructure, DataInitOption vertexDataInitOption, DataInitOption edgeDataInitOption, DataInitOption faceDataInitOption,
                              DataInitOption polyDataInitOption)
{
  InitializeNode2DGeometry(geom, dataStructure, vertexDataInitOption, edgeDataInitOption, faceDataInitOption);

  usize numElements = geom.getNumberOfVertices() / geom.getNumberOfVerticesPerCell();
  InitializeNodeGeometry<INodeGeometry3D, uint64>(
      geom, dataStructure, polyDataInitOption, INodeGeometry3D::k_PolyhedronDataName, INodeGeometry3D::k_SharedPolyhedronListName, numElements,
      [](const INodeGeometry3D& g) { return g.getNumberOfVerticesPerCell(); }, [](INodeGeometry3D& g) { return g.getPolyhedra(); },
      [](INodeGeometry3D& g, const IGeometry::SharedFaceList& da) { g.setPolyhedraList(da); }, [](INodeGeometry3D& g) { return g.getPolyhedraAttributeMatrix(); },
      [](INodeGeometry3D& g, AttributeMatrix& m) { g.setPolyhedraAttributeMatrix(m); });
}

struct ValidateDataArraysImpl
{
  template <typename T>
  void operator()(const DataStructure& dataStructure, const DataPath& inputArrayPath1, const DataPath& inputArrayPath2, const DataPath& combinedArrayPath)
  {
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath1));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath2));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<T>>(combinedArrayPath));

    auto& inputArray1 = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath1);
    auto& inputArray2 = dataStructure.getDataRefAs<DataArray<T>>(inputArrayPath2);
    auto& combinedArray = dataStructure.getDataRefAs<DataArray<T>>(combinedArrayPath);

    REQUIRE(combinedArray.getNumberOfTuples() == inputArray1.getNumberOfTuples() + inputArray2.getNumberOfTuples());
    REQUIRE(std::equal(inputArray1.begin(), inputArray1.end(), combinedArray.begin()));
    REQUIRE(std::equal(inputArray2.begin(), inputArray2.end(), combinedArray.begin() + inputArray1.size()));
  }
};

struct ValidateNeighborListsImpl
{
  template <typename T>
  void operator()(const DataStructure& dataStructure, const DataPath& inputArrayPath1, const DataPath& inputArrayPath2, const DataPath& combinedArrayPath)
  {
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<NeighborList<T>>(inputArrayPath1));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<NeighborList<T>>(inputArrayPath2));
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<NeighborList<T>>(combinedArrayPath));

    auto& inputNL1 = dataStructure.getDataRefAs<NeighborList<T>>(inputArrayPath1);
    auto& inputNL2 = dataStructure.getDataRefAs<NeighborList<T>>(inputArrayPath2);
    auto& combinedNL = dataStructure.getDataRefAs<NeighborList<T>>(combinedArrayPath);

    REQUIRE(combinedNL.getNumberOfLists() == inputNL1.getNumberOfLists() + inputNL2.getNumberOfLists());
    REQUIRE(std::equal(inputNL1.begin(), inputNL1.end(), combinedNL.begin(), [](auto& ptr1, auto& ptr2) { return ptr1 == ptr2; }));
    REQUIRE(std::equal(inputNL2.begin(), inputNL2.end(), combinedNL.begin() + inputNL1.getNumberOfLists(), [](auto& ptr1, auto& ptr2) { return ptr1 == ptr2; }));
  }
};

void ValidateStringArraysImpl(const DataStructure& dataStructure, const DataPath& inputArrayPath1, const DataPath& inputArrayPath2, const DataPath& combinedArrayPath)
{
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<StringArray>(inputArrayPath1));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<StringArray>(inputArrayPath2));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<StringArray>(combinedArrayPath));

  auto& inputStringArray1 = dataStructure.getDataRefAs<StringArray>(inputArrayPath1);
  auto& inputStringArray2 = dataStructure.getDataRefAs<StringArray>(inputArrayPath2);
  auto& combinedStringArray = dataStructure.getDataRefAs<StringArray>(combinedArrayPath);

  REQUIRE(combinedStringArray.getNumberOfTuples() == inputStringArray1.getNumberOfTuples() + inputStringArray2.getNumberOfTuples());
  REQUIRE(std::equal(inputStringArray1.begin(), inputStringArray1.end(), combinedStringArray.begin()));
  REQUIRE(std::equal(inputStringArray2.begin(), inputStringArray2.end(), combinedStringArray.begin() + inputStringArray1.size()));
}

void ValidateArrays(const DataStructure& dataStructure, const DataPath& inputArrayPath1, const DataPath& inputArrayPath2, const DataPath& combinedArrayPath)
{
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IArray>(inputArrayPath1));
  auto& array1 = dataStructure.getDataRefAs<IArray>(inputArrayPath1);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IArray>(inputArrayPath2));
  auto& array2 = dataStructure.getDataRefAs<IArray>(inputArrayPath2);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IArray>(combinedArrayPath));
  auto& combinedArray = dataStructure.getDataRefAs<IArray>(combinedArrayPath);

  REQUIRE(combinedArray.getArrayType() == array1.getArrayType());
  REQUIRE(combinedArray.getArrayType() == array2.getArrayType());

  auto combinedArrayType = combinedArray.getArrayType();
  switch(combinedArrayType)
  {
  case IArray::ArrayType::DataArray: {
    auto& dataArray1 = dataStructure.getDataRefAs<IDataArray>(inputArrayPath1);
    auto& dataArray2 = dataStructure.getDataRefAs<IDataArray>(inputArrayPath2);
    auto& combinedDataArray = dataStructure.getDataRefAs<IDataArray>(combinedArrayPath);
    REQUIRE(combinedDataArray.getDataType() == dataArray1.getDataType());
    REQUIRE(combinedDataArray.getDataType() == dataArray2.getDataType());
    ExecuteDataFunction(ValidateDataArraysImpl{}, combinedDataArray.getDataType(), dataStructure, inputArrayPath1, inputArrayPath2, combinedArrayPath);
    break;
  }
  case IArray::ArrayType::StringArray: {
    ValidateStringArraysImpl(dataStructure, inputArrayPath1, inputArrayPath2, combinedArrayPath);
    break;
  }
  case IArray::ArrayType::NeighborListArray: {
    auto& nl1 = dataStructure.getDataRefAs<INeighborList>(inputArrayPath1);
    auto& nl2 = dataStructure.getDataRefAs<INeighborList>(inputArrayPath2);
    auto& combinedNL = dataStructure.getDataRefAs<INeighborList>(combinedArrayPath);
    REQUIRE(combinedNL.getDataType() == nl1.getDataType());
    REQUIRE(combinedNL.getDataType() == nl2.getDataType());
    ExecuteNeighborFunction(ValidateNeighborListsImpl{}, combinedNL.getDataType(), dataStructure, inputArrayPath1, inputArrayPath2, combinedArrayPath);
    break;
  }
  case IArray::ArrayType::Any: {
    // This SHOULD NOT happen
    REQUIRE(0 == 1);
  }
  default: {
    // This SHOULD NOT happen
    REQUIRE(1 == 2);
  }
  }
}

template <typename T>
void ValidateElementsArray(const DataArray<T>* elementsArray1Ptr, const DataArray<T>* elementsArray2Ptr, const DataArray<T>* combinedElementsArrayPtr, uint64 offset = 0)
{
  bool allArraysMissing = (elementsArray1Ptr == nullptr && elementsArray2Ptr == nullptr && combinedElementsArrayPtr == nullptr);
  bool allArraysExist = (elementsArray1Ptr != nullptr && elementsArray2Ptr != nullptr && combinedElementsArrayPtr != nullptr);
  bool allArraysMissingOrExist = allArraysMissing || allArraysExist;
  REQUIRE(allArraysMissingOrExist);

  if(allArraysExist)
  {
    auto& elementsArray1 = *elementsArray1Ptr;
    auto& elementsArray2 = *elementsArray2Ptr;
    auto& combinedElementsArray = *combinedElementsArrayPtr;

    REQUIRE(combinedElementsArray.getNumberOfTuples() == elementsArray1.getNumberOfTuples() + elementsArray2.getNumberOfTuples());
    REQUIRE(combinedElementsArray.getNumberOfComponents() == elementsArray1.getNumberOfComponents());
    REQUIRE(combinedElementsArray.getNumberOfComponents() == elementsArray2.getNumberOfComponents());
    REQUIRE(std::equal(elementsArray1.begin(), elementsArray1.end(), combinedElementsArray.begin()));
    REQUIRE(std::equal(elementsArray2.begin(), elementsArray2.end(), combinedElementsArray.begin() + elementsArray1.size(),
                       [offset](T array2Val, T combinedVal) { return combinedVal == array2Val + offset; }));
  }
}

void ValidateAttributeMatrixArrays(const DataStructure& dataStructure, const AttributeMatrix* attrMatrix1, const AttributeMatrix* attrMatrix2, const AttributeMatrix* combinedAttrMatrix)
{
  bool allMatricesMissing = (attrMatrix1 == nullptr && attrMatrix2 == nullptr && combinedAttrMatrix == nullptr);
  if(allMatricesMissing)
  {
    // No arrays to compare
    return;
  }

  bool allMatricesExist = (attrMatrix1 != nullptr && attrMatrix2 != nullptr && combinedAttrMatrix != nullptr);
  REQUIRE(allMatricesExist);
  REQUIRE(combinedAttrMatrix->getNumTuples() == attrMatrix1->getNumTuples() + attrMatrix2->getNumTuples());

  for(const auto& [combinedId, combinedObj] : *combinedAttrMatrix)
  {
    REQUIRE(combinedObj != nullptr);
    auto iter1 = attrMatrix1->find(combinedObj->getName());
    auto iter2 = attrMatrix2->find(combinedObj->getName());
    auto obj1 = (*iter1).second;
    auto obj2 = (*iter2).second;
    REQUIRE(obj1 != nullptr);
    REQUIRE(obj2 != nullptr);
    REQUIRE(!obj1->getDataPaths().empty());
    REQUIRE(!obj2->getDataPaths().empty());
    REQUIRE(!combinedObj->getDataPaths().empty());
    ValidateArrays(dataStructure, obj1->getDataPaths()[0], obj2->getDataPaths()[0], combinedObj->getDataPaths()[0]);
  }
}

template <typename NodeGeom>
void CombineNodeBasedGeometriesImpl(const DataPath& inputGeom1Path, const DataPath& inputGeom2Path, const DataPath& outputGeomPath, DataStructure& dataStructure)
{
  CombineNodeBasedGeometriesFilter filter;
  Arguments args;
  args.insertOrAssign(CombineNodeBasedGeometriesFilter::k_InputGeometries_Key, std::vector<DataPath>{inputGeom1Path, inputGeom2Path});
  args.insertOrAssign(CombineNodeBasedGeometriesFilter::k_OutputGeometryPath_Key, outputGeomPath);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<NodeGeom>(inputGeom1Path));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<NodeGeom>(inputGeom2Path));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<NodeGeom>(outputGeomPath));
  auto& inputGeom1 = dataStructure.getDataRefAs<NodeGeom>(inputGeom1Path);
  auto& inputGeom2 = dataStructure.getDataRefAs<NodeGeom>(inputGeom2Path);
  auto& combinedGeom = dataStructure.getDataRefAs<NodeGeom>(outputGeomPath);

  auto* inputVertices1 = inputGeom1.getVertices();
  REQUIRE(inputVertices1 != nullptr);

  if constexpr(std::is_base_of_v<INodeGeometry0D, NodeGeom>)
  {
    auto* inputVertices2 = inputGeom2.getVertices();
    auto* combinedVertices = combinedGeom.getVertices();
    ValidateElementsArray(inputVertices1, inputVertices2, combinedVertices);

    auto* vertexAM1 = inputGeom1.getVertexAttributeMatrix();
    auto* vertexAM2 = inputGeom2.getVertexAttributeMatrix();
    auto* combinedVertexAM = combinedGeom.getVertexAttributeMatrix();
    ValidateAttributeMatrixArrays(dataStructure, vertexAM1, vertexAM2, combinedVertexAM);
  }
  if constexpr(std::is_base_of_v<INodeGeometry1D, NodeGeom>)
  {
    auto* inputEdges1 = inputGeom1.getEdges();
    auto* inputEdges2 = inputGeom2.getEdges();
    auto* combinedEdges = combinedGeom.getEdges();
    ValidateElementsArray(inputEdges1, inputEdges2, combinedEdges, inputVertices1->getNumberOfTuples());

    auto* edgesAM1 = inputGeom1.getEdgeAttributeMatrix();
    auto* edgesAM2 = inputGeom2.getEdgeAttributeMatrix();
    auto* combinedEdgesAM = combinedGeom.getEdgeAttributeMatrix();
    ValidateAttributeMatrixArrays(dataStructure, edgesAM1, edgesAM2, combinedEdgesAM);
  }
  if constexpr(std::is_base_of_v<INodeGeometry2D, NodeGeom>)
  {
    auto* inputFaces1 = inputGeom1.getFaces();
    auto* inputFaces2 = inputGeom2.getFaces();
    auto* combinedFaces = combinedGeom.getFaces();
    ValidateElementsArray(inputFaces1, inputFaces2, combinedFaces, inputVertices1->getNumberOfTuples());

    auto* facesAM1 = inputGeom1.getFaceAttributeMatrix();
    auto* facesAM2 = inputGeom2.getFaceAttributeMatrix();
    auto* combinedFacesAM = combinedGeom.getFaceAttributeMatrix();
    ValidateAttributeMatrixArrays(dataStructure, facesAM1, facesAM2, combinedFacesAM);
  }
  if constexpr(std::is_base_of_v<INodeGeometry3D, NodeGeom>)
  {
    auto* inputPolyhedra1 = inputGeom1.getPolyhedra();
    auto* inputPolyhedra2 = inputGeom2.getPolyhedra();
    auto* combinedPolyhedra = combinedGeom.getPolyhedra();
    ValidateElementsArray(inputPolyhedra1, inputPolyhedra2, combinedPolyhedra, inputVertices1->getNumberOfTuples());

    auto* polyAM1 = inputGeom1.getPolyhedraAttributeMatrix();
    auto* polyAM2 = inputGeom2.getPolyhedraAttributeMatrix();
    auto* combinedPolyAM = combinedGeom.getPolyhedraAttributeMatrix();
    ValidateAttributeMatrixArrays(dataStructure, polyAM1, polyAM2, combinedPolyAM);
  }
}

template <typename NodeGeom>
void InitializeNodeBasedGeometries(DataStructure& dataStructure)
{
  // DO NOT test DataInitOption::None and DataInitOption::ElementsOnly for Vertex and Polyhedra because a vertex array & attr matrix will
  // be automatically created for any geometry, and a polyhedra array and attribute matrix will be automatically created for 3D geometries.
  GeometryInitOptions opts{GENERATE(DataInitOption::ElementsAndData, DataInitOption::ElementsAndNoData), GENERATE(from_range(std::begin(k_DataInitOptions), std::end(k_DataInitOptions))),
                           GENERATE(from_range(std::begin(k_DataInitOptions), std::end(k_DataInitOptions))), GENERATE(DataInitOption::ElementsAndData, DataInitOption::ElementsAndNoData)};

  CAPTURE(opts);

  if constexpr(std::is_base_of_v<INodeGeometry3D, NodeGeom>)
  {
    auto& geom1 = dataStructure.getDataRefAs<NodeGeom>(k_InputGeometry1Path);
    auto& geom2 = dataStructure.getDataRefAs<NodeGeom>(k_InputGeometry2Path);
    auto& geom3 = dataStructure.getDataRefAs<NodeGeom>(k_InputGeometry3Path);
    auto& geom4 = dataStructure.getDataRefAs<NodeGeom>(k_InputGeometry4Path);
    auto& geom5 = dataStructure.getDataRefAs<NodeGeom>(k_InputGeometry5Path);

    InitializeNode3DGeometry(geom1, dataStructure, opts.vertex, opts.edge, opts.face, opts.poly);
    InitializeNode3DGeometry(geom2, dataStructure, opts.vertex, opts.edge, opts.face, opts.poly);
    InitializeNode3DGeometry(geom3, dataStructure, opts.vertex, opts.edge, opts.face, opts.poly);
    InitializeNode3DGeometry(geom4, dataStructure, opts.vertex, opts.edge, opts.face, opts.poly);
    InitializeNode3DGeometry(geom5, dataStructure, opts.vertex, opts.edge, opts.face, opts.poly);
  }
  else if constexpr(std::is_base_of_v<INodeGeometry2D, NodeGeom>)
  {
    auto& geom1 = dataStructure.getDataRefAs<NodeGeom>(k_InputGeometry1Path);
    auto& geom2 = dataStructure.getDataRefAs<NodeGeom>(k_InputGeometry2Path);
    auto& geom3 = dataStructure.getDataRefAs<NodeGeom>(k_InputGeometry3Path);
    auto& geom4 = dataStructure.getDataRefAs<NodeGeom>(k_InputGeometry4Path);

    InitializeNode2DGeometry(geom1, dataStructure, opts.vertex, opts.edge, opts.face);
    InitializeNode2DGeometry(geom2, dataStructure, opts.vertex, opts.edge, opts.face);
    InitializeNode2DGeometry(geom3, dataStructure, opts.vertex, opts.edge, opts.face);
    InitializeNode2DGeometry(geom4, dataStructure, opts.vertex, opts.edge, opts.face);
  }
  else if constexpr(std::is_base_of_v<INodeGeometry1D, NodeGeom>)
  {
    auto& geom1 = dataStructure.getDataRefAs<NodeGeom>(k_InputGeometry1Path);
    auto& geom2 = dataStructure.getDataRefAs<NodeGeom>(k_InputGeometry2Path);
    auto& geom3 = dataStructure.getDataRefAs<NodeGeom>(k_InputGeometry3Path);

    InitializeNode1DGeometry(geom1, dataStructure, opts.vertex, opts.edge);
    InitializeNode1DGeometry(geom2, dataStructure, opts.vertex, opts.edge);
    InitializeNode1DGeometry(geom3, dataStructure, opts.vertex, opts.edge);
  }
  else if constexpr(std::is_base_of_v<INodeGeometry0D, NodeGeom>)
  {
    auto& geom1 = dataStructure.getDataRefAs<NodeGeom>(k_InputGeometry1Path);
    auto& geom2 = dataStructure.getDataRefAs<NodeGeom>(k_InputGeometry2Path);

    InitializeNode0DGeometry(geom1, dataStructure, opts.vertex);
    InitializeNode0DGeometry(geom2, dataStructure, opts.vertex);
  }
  else
  {
    // This should not happen
    REQUIRE(0 == 1);
  }
}

template <typename NodeGeom>
void LoadAndExecute0DNodeGeometriesTest(const fs::path& inputFilePath)
{
  DataStructure dataStructure = UnitTest::LoadDataStructure(inputFilePath);
  InitializeNodeBasedGeometries<NodeGeom>(dataStructure);

  // Test two 0D node-based geometries
  CombineNodeBasedGeometriesImpl<NodeGeom>(k_InputGeometry1Path, k_InputGeometry2Path, k_OutputGeometryPath4, dataStructure);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);

  SUCCEED("Test completed successfully.");
}

template <typename NodeGeom>
void LoadAndExecute1DNodeGeometriesTest(const fs::path& inputFilePath)
{
  DataStructure dataStructure = UnitTest::LoadDataStructure(inputFilePath);
  InitializeNodeBasedGeometries<NodeGeom>(dataStructure);

  SECTION("Shared Vertex")
  {
    // Test two 1D node-based geometries that share a vertex
    CombineNodeBasedGeometriesImpl<NodeGeom>(k_InputGeometry1Path, k_InputGeometry2Path, k_OutputGeometryPath3, dataStructure);
  }
  SECTION("Nothing Shared")
  {
    // Test two 1D node-based geometries that do not share anything
    CombineNodeBasedGeometriesImpl<NodeGeom>(k_InputGeometry2Path, k_InputGeometry3Path, k_OutputGeometryPath4, dataStructure);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);

  SUCCEED("Test completed successfully.");
}

template <typename NodeGeom>
void LoadAndExecute2DNodeGeometriesTest(const fs::path& inputFilePath)
{
  DataStructure dataStructure = UnitTest::LoadDataStructure(inputFilePath);
  InitializeNodeBasedGeometries<NodeGeom>(dataStructure);

  SECTION("Shared Edge")
  {
    // Test two 2D node-based geometries that share an edge
    CombineNodeBasedGeometriesImpl<NodeGeom>(k_InputGeometry1Path, k_InputGeometry2Path, k_OutputGeometryPath2, dataStructure);
  }
  SECTION("Shared Vertex")
  {
    // Test two 2D node-based geometries that share a vertex
    CombineNodeBasedGeometriesImpl<NodeGeom>(k_InputGeometry2Path, k_InputGeometry3Path, k_OutputGeometryPath3, dataStructure);
  }
  SECTION("Nothing Shared")
  {
    // Test two 2D node-based geometries that do not share anything
    CombineNodeBasedGeometriesImpl<NodeGeom>(k_InputGeometry3Path, k_InputGeometry4Path, k_OutputGeometryPath4, dataStructure);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);

  SUCCEED("Test completed successfully.");
}

template <typename NodeGeom>
void LoadAndExecute3DNodeGeometriesTest(const fs::path& inputFilePath)
{
  DataStructure dataStructure = UnitTest::LoadDataStructure(inputFilePath);
  InitializeNodeBasedGeometries<NodeGeom>(dataStructure);

  SECTION("Shared Face")
  {
    // Test two 3D node-based geometries that share a face
    CombineNodeBasedGeometriesImpl<NodeGeom>(k_InputGeometry1Path, k_InputGeometry2Path, k_OutputGeometryPath1, dataStructure);
  }
  SECTION("Shared Edge")
  {
    // Test two 3D node-based geometries that share an edge
    CombineNodeBasedGeometriesImpl<NodeGeom>(k_InputGeometry2Path, k_InputGeometry3Path, k_OutputGeometryPath2, dataStructure);
  }
  SECTION("Shared Vertex")
  {
    // Test two 3D node-based geometries that share a vertex
    CombineNodeBasedGeometriesImpl<NodeGeom>(k_InputGeometry3Path, k_InputGeometry4Path, k_OutputGeometryPath3, dataStructure);
  }
  SECTION("Nothing Shared")
  {
    // Test two 3D node-based geometries that do not share anything
    CombineNodeBasedGeometriesImpl<NodeGeom>(k_InputGeometry4Path, k_InputGeometry5Path, k_OutputGeometryPath4, dataStructure);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);

  SUCCEED("Test completed successfully.");
}

TEST_CASE("Combine Node Geometries: Vertex", "[SimplnxCore][CombineNodeBasedGeometries]")
{
  static const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "combine_node_based_geometries.tar.gz",
                                                                     "combine_vertex_geometries.dream3d");
  auto inputFilePath = fs::path(fmt::format("{}/combine_node_based_geometries/combine_vertex_geometries.dream3d", unit_test::k_TestFilesDir));
  LoadAndExecute0DNodeGeometriesTest<VertexGeom>(inputFilePath);
}

TEST_CASE("Combine Node Geometries: Edge", "[SimplnxCore][CombineNodeBasedGeometries]")
{
  static const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "combine_node_based_geometries.tar.gz",
                                                                     "combine_edge_geometries.dream3d");
  auto inputFilePath = fs::path(fmt::format("{}/combine_node_based_geometries/combine_edge_geometries.dream3d", unit_test::k_TestFilesDir));
  LoadAndExecute1DNodeGeometriesTest<EdgeGeom>(inputFilePath);
}

TEST_CASE("Combine Node Geometries: Triangle", "[SimplnxCore][CombineNodeBasedGeometries]")
{
  static const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "combine_node_based_geometries.tar.gz",
                                                                     "combine_triangle_geometries.dream3d");
  auto inputFilePath = fs::path(fmt::format("{}/combine_node_based_geometries/combine_triangle_geometries.dream3d", unit_test::k_TestFilesDir));
  LoadAndExecute2DNodeGeometriesTest<TriangleGeom>(inputFilePath);
}

TEST_CASE("Combine Node Geometries: Quad", "[SimplnxCore][CombineNodeBasedGeometries]")
{
  static const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "combine_node_based_geometries.tar.gz",
                                                                     "combine_quad_geometries.dream3d");
  auto inputFilePath = fs::path(fmt::format("{}/combine_node_based_geometries/combine_quad_geometries.dream3d", unit_test::k_TestFilesDir));
  LoadAndExecute2DNodeGeometriesTest<QuadGeom>(inputFilePath);
}

TEST_CASE("Combine Node Geometries: Tetrahedral", "[SimplnxCore][CombineNodeBasedGeometries]")
{
  static const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "combine_node_based_geometries.tar.gz",
                                                                     "combine_tetrahedral_geometries.dream3d");
  auto inputFilePath = fs::path(fmt::format("{}/combine_node_based_geometries/combine_tetrahedral_geometries.dream3d", unit_test::k_TestFilesDir));
  LoadAndExecute3DNodeGeometriesTest<TetrahedralGeom>(inputFilePath);
}

TEST_CASE("Combine Node Geometries: Hexahedral", "[SimplnxCore][CombineNodeBasedGeometries]")
{
  static const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_CMakeExecutable, nx::core::unit_test::k_TestFilesDir, "combine_node_based_geometries.tar.gz",
                                                                     "combine_hexahedral_geometries.dream3d");

  auto inputFilePath = std::filesystem::path(fmt::format("{}/combine_node_based_geometries/combine_hexahedral_geometries.dream3d", unit_test::k_TestFilesDir));
  LoadAndExecute3DNodeGeometriesTest<HexahedralGeom>(inputFilePath);
}

TEST_CASE("Combine Node Geometries: Error Conditions", "[SimplnxCore][CombineNodeBasedGeometries]")
{
  DataStructure dataStructure;

  CombineNodeBasedGeometries::ErrorCodes errorCode;
  std::vector<DataPath> inputPaths;
  DataPath outputPath;
  SECTION("Fewer Than Two Paths Chosen")
  {
    SECTION("")
    {
      inputPaths = std::vector<DataPath>{};
      outputPath = k_OutputGeometryPath1;
    }
    SECTION("")
    {
      inputPaths = std::vector<DataPath>{k_InputGeometry1Path};
      outputPath = k_OutputGeometryPath1;
    }
    VertexGeom::Create(dataStructure, k_Geometry1Name);
    VertexGeom::Create(dataStructure, k_Geometry2Name);
    errorCode = CombineNodeBasedGeometries::ErrorCodes::FewerThanTwoPathsChosen;
  }
  SECTION("Differing Geometries")
  {
    auto geom1 = VertexGeom::Create(dataStructure, k_Geometry1Name);
    auto vertices = CreateNodeArray<float32>(*geom1, dataStructure, INodeGeometry0D::k_SharedVertexListName, 10, 3);
    geom1->setVertices(*vertices);

    auto geom2 = EdgeGeom::Create(dataStructure, k_Geometry2Name);
    vertices = CreateNodeArray<float32>(*geom2, dataStructure, INodeGeometry0D::k_SharedVertexListName, 10, 3);
    geom2->setVertices(*vertices);

    inputPaths = std::vector<DataPath>{k_InputGeometry1Path, k_InputGeometry2Path};
    outputPath = k_OutputGeometryPath1;
    errorCode = CombineNodeBasedGeometries::ErrorCodes::DifferingGeometryTypes;
  }
  SECTION("Object Not A Geometry")
  {
    DataGroup::Create(dataStructure, k_Geometry1Name);
    DataGroup::Create(dataStructure, k_Geometry2Name);
    inputPaths = std::vector<DataPath>{k_InputGeometry1Path, k_InputGeometry2Path};
    outputPath = k_OutputGeometryPath1;
    errorCode = CombineNodeBasedGeometries::ErrorCodes::ObjectNotAGeometry;
  }
  SECTION("Geometry Not A Node Geometry")
  {
    ImageGeom::Create(dataStructure, k_Geometry1Name);
    ImageGeom::Create(dataStructure, k_Geometry2Name);
    inputPaths = std::vector<DataPath>{k_InputGeometry1Path, k_InputGeometry2Path};
    outputPath = k_OutputGeometryPath1;
    errorCode = CombineNodeBasedGeometries::ErrorCodes::ObjectNotANodeGeometry;
  }
  SECTION("Node Geometry With No Vertices")
  {
    VertexGeom::Create(dataStructure, k_Geometry1Name);
    VertexGeom::Create(dataStructure, k_Geometry2Name);
    inputPaths = std::vector<DataPath>{k_InputGeometry1Path, k_InputGeometry2Path};
    outputPath = k_OutputGeometryPath1;
    errorCode = CombineNodeBasedGeometries::ErrorCodes::NodeGeometryHasNoVertices;
  }
  SECTION("Inconsistent Geometry Elements")
  {
    SECTION("Node Array")
    {
      SECTION("1D Geometry")
      {
        auto geom1 = EdgeGeom::Create(dataStructure, k_Geometry1Name);
        auto geom2 = EdgeGeom::Create(dataStructure, k_Geometry2Name);

        auto vertices = CreateNodeArray<float32>(*geom1, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom1->getNumberOfVerticesPerEdge(), 3);
        geom1->setVertices(*vertices);
        vertices = CreateNodeArray<float32>(*geom2, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom2->getNumberOfVerticesPerEdge(), 3);
        geom2->setVertices(*vertices);

        auto edges = CreateNodeArray<uint64>(*geom1, dataStructure, INodeGeometry1D::k_SharedEdgeListName, 1, geom1->getNumberOfVerticesPerEdge());
        geom1->setEdgeList(*edges);
      }
      SECTION("2D Geometry")
      {
        auto geom1 = TriangleGeom::Create(dataStructure, k_Geometry1Name);
        auto geom2 = TriangleGeom::Create(dataStructure, k_Geometry2Name);

        auto vertices = CreateNodeArray<float32>(*geom1, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom1->getNumberOfVerticesPerFace(), 3);
        geom1->setVertices(*vertices);
        vertices = CreateNodeArray<float32>(*geom2, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom2->getNumberOfVerticesPerFace(), 3);
        geom2->setVertices(*vertices);

        auto faces = CreateNodeArray<uint64>(*geom1, dataStructure, INodeGeometry2D::k_SharedFacesListName, 1, geom1->getNumberOfVerticesPerFace());
        geom1->setFaceList(*faces);
      }

      SECTION("3D Geometry")
      {
        auto geom1 = HexahedralGeom::Create(dataStructure, k_Geometry1Name);
        auto geom2 = HexahedralGeom::Create(dataStructure, k_Geometry2Name);

        auto vertices = CreateNodeArray<float32>(*geom1, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom1->getNumberOfVerticesPerCell(), 3);
        geom1->setVertices(*vertices);
        vertices = CreateNodeArray<float32>(*geom2, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom2->getNumberOfVerticesPerCell(), 3);
        geom2->setVertices(*vertices);

        auto polyhedra = CreateNodeArray<uint64>(*geom1, dataStructure, INodeGeometry3D::k_SharedPolyhedronListName, 1, geom1->getNumberOfVerticesPerCell());
        geom1->setPolyhedraList(*polyhedra);
      }

      errorCode = CombineNodeBasedGeometries::ErrorCodes::InconsistentGeometryElements;
    }
    SECTION("Vertex Attribute Matrix")
    {
      auto geom1 = TetrahedralGeom::Create(dataStructure, k_Geometry1Name);
      auto geom2 = TetrahedralGeom::Create(dataStructure, k_Geometry2Name);

      auto vertices = CreateNodeArray<float32>(*geom1, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom1->getNumberOfVerticesPerCell(), 3);
      geom1->setVertices(*vertices);
      vertices = CreateNodeArray<float32>(*geom2, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom2->getNumberOfVerticesPerCell(), 3);
      geom2->setVertices(*vertices);

      auto vertexAttrMatrix = CreateNodeAttributeMatrix(*geom1, dataStructure, INodeGeometry0D::k_VertexAttributeMatrixName, geom1->getNumberOfVerticesPerCell());
      geom1->setVertexAttributeMatrix(*vertexAttrMatrix);

      errorCode = CombineNodeBasedGeometries::ErrorCodes::InconsistentGeometryElements;
    }
    SECTION("Vertex Data Arrays")
    {
      SECTION("Data Array Missing")
      {
        auto geom1 = EdgeGeom::Create(dataStructure, k_Geometry1Name);
        auto geom2 = EdgeGeom::Create(dataStructure, k_Geometry2Name);

        auto vertices = CreateNodeArray<float32>(*geom1, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom1->getNumberOfVerticesPerEdge(), 3);
        geom1->setVertices(*vertices);
        vertices = CreateNodeArray<float32>(*geom2, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom2->getNumberOfVerticesPerEdge(), 3);
        geom2->setVertices(*vertices);

        auto vertexAttrMatrix = CreateNodeAttributeMatrix(*geom1, dataStructure, INodeGeometry0D::k_VertexAttributeMatrixName, geom1->getNumberOfVerticesPerEdge());
        geom1->setVertexAttributeMatrix(*vertexAttrMatrix);

        vertexAttrMatrix = CreateNodeAttributeMatrix(*geom2, dataStructure, INodeGeometry0D::k_VertexAttributeMatrixName, geom2->getNumberOfVerticesPerEdge());
        geom2->setVertexAttributeMatrix(*vertexAttrMatrix);

        AddDataArray<uint8, 2>(*vertexAttrMatrix, dataStructure);

        errorCode = CombineNodeBasedGeometries::ErrorCodes::InconsistentGeometryElements;
      }
      SECTION("Mismatching Data Types")
      {
        auto geom1 = QuadGeom::Create(dataStructure, k_Geometry1Name);
        auto geom2 = QuadGeom::Create(dataStructure, k_Geometry2Name);

        auto vertices = CreateNodeArray<float32>(*geom1, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom1->getNumberOfVerticesPerFace(), 3);
        geom1->setVertices(*vertices);
        vertices = CreateNodeArray<float32>(*geom2, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom2->getNumberOfVerticesPerFace(), 3);
        geom2->setVertices(*vertices);

        auto vertexAttrMatrix = CreateNodeAttributeMatrix(*geom1, dataStructure, INodeGeometry0D::k_VertexAttributeMatrixName, geom1->getNumberOfVerticesPerFace());
        geom1->setVertexAttributeMatrix(*vertexAttrMatrix);

        CreateDataArray<uint8, 2>(*vertexAttrMatrix, dataStructure, "TestArray");

        vertexAttrMatrix = CreateNodeAttributeMatrix(*geom2, dataStructure, INodeGeometry0D::k_VertexAttributeMatrixName, geom2->getNumberOfVerticesPerFace());
        geom2->setVertexAttributeMatrix(*vertexAttrMatrix);

        CreateDataArray<uint16, 2>(*vertexAttrMatrix, dataStructure, "TestArray");

        errorCode = CombineNodeBasedGeometries::ErrorCodes::InconsistentGeometryElementDataTypes;
      }
      SECTION("Mismatching Array Types")
      {
        auto geom1 = TriangleGeom::Create(dataStructure, k_Geometry1Name);
        auto geom2 = TriangleGeom::Create(dataStructure, k_Geometry2Name);

        auto vertices = CreateNodeArray<float32>(*geom1, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom1->getNumberOfVerticesPerFace(), 3);
        geom1->setVertices(*vertices);
        vertices = CreateNodeArray<float32>(*geom2, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom2->getNumberOfVerticesPerFace(), 3);
        geom2->setVertices(*vertices);

        auto vertexAttrMatrix = CreateNodeAttributeMatrix(*geom1, dataStructure, INodeGeometry0D::k_VertexAttributeMatrixName, geom1->getNumberOfVerticesPerFace());
        geom1->setVertexAttributeMatrix(*vertexAttrMatrix);

        CreateDataArray<uint8, 2>(*vertexAttrMatrix, dataStructure, "TestArray");

        vertexAttrMatrix = CreateNodeAttributeMatrix(*geom2, dataStructure, INodeGeometry0D::k_VertexAttributeMatrixName, geom2->getNumberOfVerticesPerFace());
        geom2->setVertexAttributeMatrix(*vertexAttrMatrix);

        CreateStringArray(*vertexAttrMatrix, dataStructure, "TestArray");

        errorCode = CombineNodeBasedGeometries::ErrorCodes::InconsistentGeometryElementArrayTypes;
      }
      SECTION("Mismatching Component Dimensions")
      {
        auto geom1 = TriangleGeom::Create(dataStructure, k_Geometry1Name);
        auto geom2 = TriangleGeom::Create(dataStructure, k_Geometry2Name);

        auto vertices = CreateNodeArray<float32>(*geom1, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom1->getNumberOfVerticesPerFace(), 3);
        geom1->setVertices(*vertices);
        vertices = CreateNodeArray<float32>(*geom2, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom2->getNumberOfVerticesPerFace(), 3);
        geom2->setVertices(*vertices);

        auto vertexAttrMatrix = CreateNodeAttributeMatrix(*geom1, dataStructure, INodeGeometry0D::k_VertexAttributeMatrixName, geom1->getNumberOfVerticesPerFace());
        geom1->setVertexAttributeMatrix(*vertexAttrMatrix);

        CreateDataArray<int32>(*vertexAttrMatrix, dataStructure, "TestArray", {2, 3});

        vertexAttrMatrix = CreateNodeAttributeMatrix(*geom2, dataStructure, INodeGeometry0D::k_VertexAttributeMatrixName, geom2->getNumberOfVerticesPerFace());
        geom2->setVertexAttributeMatrix(*vertexAttrMatrix);

        CreateDataArray<int32>(*vertexAttrMatrix, dataStructure, "TestArray", {4, 5});

        errorCode = CombineNodeBasedGeometries::ErrorCodes::InconsistentGeometryElementCompDims;
      }
    }
    SECTION("Cell Data Arrays")
    {
      SECTION("Data Array Missing")
      {
        auto geom1 = EdgeGeom::Create(dataStructure, k_Geometry1Name);
        auto geom2 = EdgeGeom::Create(dataStructure, k_Geometry2Name);

        auto vertices = CreateNodeArray<float32>(*geom1, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom1->getNumberOfVerticesPerEdge(), 3);
        geom1->setVertices(*vertices);
        vertices = CreateNodeArray<float32>(*geom2, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom2->getNumberOfVerticesPerEdge(), 3);
        geom2->setVertices(*vertices);

        auto cellAttrMatrix = CreateNodeAttributeMatrix(*geom1, dataStructure, INodeGeometry0D::k_VertexAttributeMatrixName, 1);
        geom1->setEdgeAttributeMatrix(*cellAttrMatrix);

        cellAttrMatrix = CreateNodeAttributeMatrix(*geom2, dataStructure, INodeGeometry0D::k_VertexAttributeMatrixName, 1);
        geom2->setEdgeAttributeMatrix(*cellAttrMatrix);

        AddDataArray<uint8, 2>(*cellAttrMatrix, dataStructure);

        errorCode = CombineNodeBasedGeometries::ErrorCodes::InconsistentGeometryElements;
      }
      SECTION("Mismatching Data Types")
      {
        auto geom1 = QuadGeom::Create(dataStructure, k_Geometry1Name);
        auto geom2 = QuadGeom::Create(dataStructure, k_Geometry2Name);

        auto vertices = CreateNodeArray<float32>(*geom1, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom1->getNumberOfVerticesPerFace(), 3);
        geom1->setVertices(*vertices);
        vertices = CreateNodeArray<float32>(*geom2, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom2->getNumberOfVerticesPerFace(), 3);
        geom2->setVertices(*vertices);

        auto cellAttrMatrix = CreateNodeAttributeMatrix(*geom1, dataStructure, INodeGeometry0D::k_VertexAttributeMatrixName, 1);
        geom1->setFaceAttributeMatrix(*cellAttrMatrix);

        CreateDataArray<uint8, 2>(*cellAttrMatrix, dataStructure, "TestArray");

        cellAttrMatrix = CreateNodeAttributeMatrix(*geom2, dataStructure, INodeGeometry0D::k_VertexAttributeMatrixName, 1);
        geom2->setFaceAttributeMatrix(*cellAttrMatrix);

        CreateDataArray<uint16, 2>(*cellAttrMatrix, dataStructure, "TestArray");

        errorCode = CombineNodeBasedGeometries::ErrorCodes::InconsistentGeometryElementDataTypes;
      }
      SECTION("Mismatching Array Types")
      {
        auto geom1 = TriangleGeom::Create(dataStructure, k_Geometry1Name);
        auto geom2 = TriangleGeom::Create(dataStructure, k_Geometry2Name);

        auto vertices = CreateNodeArray<float32>(*geom1, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom1->getNumberOfVerticesPerFace(), 3);
        geom1->setVertices(*vertices);
        vertices = CreateNodeArray<float32>(*geom2, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom2->getNumberOfVerticesPerFace(), 3);
        geom2->setVertices(*vertices);

        auto cellAttrMatrix = CreateNodeAttributeMatrix(*geom1, dataStructure, INodeGeometry0D::k_VertexAttributeMatrixName, 1);
        geom1->setFaceAttributeMatrix(*cellAttrMatrix);

        CreateDataArray<uint8, 2>(*cellAttrMatrix, dataStructure, "TestArray");

        cellAttrMatrix = CreateNodeAttributeMatrix(*geom2, dataStructure, INodeGeometry0D::k_VertexAttributeMatrixName, 1);
        geom2->setFaceAttributeMatrix(*cellAttrMatrix);

        CreateStringArray(*cellAttrMatrix, dataStructure, "TestArray");

        errorCode = CombineNodeBasedGeometries::ErrorCodes::InconsistentGeometryElementArrayTypes;
      }
      SECTION("Mismatching Component Dimensions")
      {
        auto geom1 = TriangleGeom::Create(dataStructure, k_Geometry1Name);
        auto geom2 = TriangleGeom::Create(dataStructure, k_Geometry2Name);

        auto vertices = CreateNodeArray<float32>(*geom1, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom1->getNumberOfVerticesPerFace(), 3);
        geom1->setVertices(*vertices);
        vertices = CreateNodeArray<float32>(*geom2, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom2->getNumberOfVerticesPerFace(), 3);
        geom2->setVertices(*vertices);

        auto cellAttrMatrix = CreateNodeAttributeMatrix(*geom1, dataStructure, INodeGeometry0D::k_VertexAttributeMatrixName, 1);
        geom1->setFaceAttributeMatrix(*cellAttrMatrix);

        CreateDataArray<int32>(*cellAttrMatrix, dataStructure, "TestArray", {2, 3});

        cellAttrMatrix = CreateNodeAttributeMatrix(*geom2, dataStructure, INodeGeometry0D::k_VertexAttributeMatrixName, 1);
        geom2->setFaceAttributeMatrix(*cellAttrMatrix);

        CreateDataArray<int32>(*cellAttrMatrix, dataStructure, "TestArray", {4, 5});

        errorCode = CombineNodeBasedGeometries::ErrorCodes::InconsistentGeometryElementCompDims;
      }
    }
    SECTION("Cell Attribute Matrix")
    {
      auto geom1 = HexahedralGeom::Create(dataStructure, k_Geometry1Name);
      auto geom2 = HexahedralGeom::Create(dataStructure, k_Geometry2Name);

      auto vertices = CreateNodeArray<float32>(*geom1, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom1->getNumberOfVerticesPerCell(), 3);
      geom1->setVertices(*vertices);
      vertices = CreateNodeArray<float32>(*geom2, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom2->getNumberOfVerticesPerCell(), 3);
      geom2->setVertices(*vertices);

      auto vertexAttrMatrix = CreateNodeAttributeMatrix(*geom1, dataStructure, INodeGeometry0D::k_VertexAttributeMatrixName, 1);
      geom1->setPolyhedraAttributeMatrix(*vertexAttrMatrix);

      errorCode = CombineNodeBasedGeometries::ErrorCodes::InconsistentGeometryElements;
    }

    inputPaths = std::vector<DataPath>{k_InputGeometry1Path, k_InputGeometry2Path};
    outputPath = k_OutputGeometryPath1;
  }

  CombineNodeBasedGeometriesFilter filter;
  Arguments args;
  args.insertOrAssign(CombineNodeBasedGeometriesFilter::k_InputGeometries_Key, inputPaths);
  args.insertOrAssign(CombineNodeBasedGeometriesFilter::k_OutputGeometryPath_Key, outputPath);

  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  REQUIRE(result.result.errors().size() == 1);
  REQUIRE(result.result.errors()[0].code == to_underlying(errorCode));

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
