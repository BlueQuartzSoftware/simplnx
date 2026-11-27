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
#include "simplnx/Utilities/DataObjectUtilities.hpp"

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

static std::vector<std::string> KDataInitOptionStrings = {"None", "Elements Only", "Elements & No Data", "Elements & Data"};

static std::string GetDataInitOptionString(const DataInitOption& initOption)
{
  return KDataInitOptionStrings[to_underlying(initOption)];
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
static std::ostream& operator<<(std::ostream& os, const GeometryInitOptions& opts)
{
  os << "Vertex: " << GetDataInitOptionString(opts.vertex) << ", Edge: " << GetDataInitOptionString(opts.edge) << ", Face: " << GetDataInitOptionString(opts.face)
     << ", Poly: " << GetDataInitOptionString(opts.poly);
  return os;
}

template <typename T>
static DataArray<T>* CreateDataArray(AttributeMatrix& attrMatrix, DataStructure& dataStructure, const std::string& arrayName, const std::vector<usize>& cDims)
{
  return DataArray<T>::template CreateWithStore<DataStore<T>>(dataStructure, arrayName, {attrMatrix.getNumberOfTuples()}, cDims, attrMatrix.getId());
}

template <typename T, usize CompCount>
static DataArray<T>* CreateDataArray(AttributeMatrix& attrMatrix, DataStructure& dataStructure, const std::string& arrayName)
{
  return CreateDataArray<T>(attrMatrix, dataStructure, arrayName, {CompCount});
}

template <typename T, usize CompCount>
static void AddDataArray(AttributeMatrix& attrMatrix, DataStructure& dataStructure)
{
  DataType dataType = GetDataType<T>();
  std::string dataTypeStr = DataTypeToString(dataType);
  auto arrPtr = CreateDataArray<T, CompCount>(attrMatrix, dataStructure, fmt::format("{}_Array_{}", dataTypeStr, CompCount));
  std::iota(arrPtr->begin(), arrPtr->end(), static_cast<T>(0));
}

static StringArray* CreateStringArray(AttributeMatrix& attrMatrix, DataStructure& dataStructure, const std::string& arrayName)
{
  return StringArray::CreateWithValues(dataStructure, arrayName, attrMatrix.getShape(), std::vector<std::string>(attrMatrix.getNumberOfTuples()), attrMatrix.getId());
}

static void AddStringArray(AttributeMatrix& attrMatrix, DataStructure& dataStructure)
{
  auto* strArrayPtr = CreateStringArray(attrMatrix, dataStructure, "StringArray");
  int counter = 1;
  std::generate(strArrayPtr->begin(), strArrayPtr->end(), [&counter]() { return "Item" + std::to_string(counter++); });
}

template <typename T>
static NeighborList<T>* CreateNeighborList(AttributeMatrix& attrMatrix, DataStructure& dataStructure, const std::string& nlName)
{
  return NeighborList<T>::Create(dataStructure, nlName, attrMatrix.getShape(), attrMatrix.getId());
}

template <typename T>
static void AddNeighborList(AttributeMatrix& attrMatrix, DataStructure& dataStructure)
{
  DataType dataType = GetDataType<T>();
  std::string dataTypeStr = DataTypeToString(dataType);
  auto nlPtr = CreateNeighborList<T>(attrMatrix, dataStructure, fmt::format("{}_NeighborList", dataTypeStr));
  for(usize i = 0; i < attrMatrix.getNumberOfTuples(); i++)
  {
    typename NeighborList<T>::SharedVectorType inputList(new std::vector<T>(k_NeighborListListSize));
    std::iota(inputList->begin(), inputList->end(), static_cast<T>(0));
    nlPtr->setList(i, inputList);
  }
}

template <usize CompCount>
static void CreateDataArrays(AttributeMatrix& attrMatrix, DataStructure& dataStructure)
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

static void CreateDataArrays(AttributeMatrix& attrMatrix, DataStructure& dataStructure)
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

static AttributeMatrix* CreateNodeAttributeMatrix(IGeometry& geom, DataStructure& dataStructure, const std::string& attrMatrixName, usize numElements)
{
  return AttributeMatrix::Create(dataStructure, attrMatrixName, {numElements}, geom.getId());
}

template <typename TGeom, typename TSetAttrMatrix>
static void CreateNodeAttributeMatrix(TGeom& geom, DataStructure& dataStructure, const std::string& attrMatrixName, usize numElements, TSetAttrMatrix setAttrMatrix)
{
  auto attrMatrixPtr = CreateNodeAttributeMatrix(geom, dataStructure, attrMatrixName, numElements);
  setAttrMatrix(geom, *attrMatrixPtr);
}

template <typename TNode>
static DataArray<TNode>* CreateNodeArray(IGeometry& geom, DataStructure& dataStructure, const std::string& sharedListName, usize numElements, usize numPerElement)
{
  return DataArray<TNode>::template CreateWithStore<DataStore<TNode>>(dataStructure, sharedListName, {numElements}, {numPerElement}, geom.getId());
}

template <typename TGeom, typename TNode, typename TGetNumPerElement, typename TSetNodeArray>
static void CreateNodeArray(TGeom& geom, DataStructure& dataStructure, const std::string& sharedListName, usize numElements, TGetNumPerElement getNumPerElement, TSetNodeArray setNodeArray)
{
  auto arrPtr = CreateNodeArray<TNode>(geom, dataStructure, sharedListName, numElements, getNumPerElement(geom));
  std::iota(arrPtr->begin(), arrPtr->end(), static_cast<TNode>(0));
  setNodeArray(geom, *arrPtr);
}

template <typename TGeom, typename TNode, typename TGetNumPerElement, typename TGetNodeArray, typename TSetNodeArray, typename TGetAttrMatrix, typename TSetAttrMatrix>
static void InitializeNodeGeometry(TGeom& geom, DataStructure& dataStructure, DataInitOption initOption, const std::string& attrMatrixName, const std::string& sharedListName, usize numElements,
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
    auto* attrMatrixPtr = getAttrMatrix(geom);
    if(attrMatrixPtr == nullptr)
    {
      CreateNodeAttributeMatrix(geom, dataStructure, attrMatrixName, numElements, setAttrMatrix);
      attrMatrixPtr = getAttrMatrix(geom);
    }
    if(initOption == DataInitOption::ElementsAndData)
    {
      CreateDataArrays(*attrMatrixPtr, dataStructure);
    }
  }
}

static void InitializeNode0DGeometry(INodeGeometry0D& geom, DataStructure& dataStructure, DataInitOption vertexDataInitOption)
{
  usize numOfVertexComps = 3;
  InitializeNodeGeometry<INodeGeometry0D, float32>(
      geom, dataStructure, vertexDataInitOption, INodeGeometry0D::k_VertexAttributeMatrixName, INodeGeometry0D::k_SharedVertexListName, geom.getNumberOfVertices(),
      [numOfVertexComps](const INodeGeometry0D& /*g*/) { return numOfVertexComps; }, [](INodeGeometry0D& g) { return g.getVertices(); },
      [](INodeGeometry0D& g, const Float32ArrayType& da) { g.setVertices(da); }, [](INodeGeometry0D& g) { return g.getVertexAttributeMatrix(); },
      [](INodeGeometry0D& g, AttributeMatrix& m) { g.setVertexAttributeMatrix(m); });
}

static void InitializeNode1DGeometry(INodeGeometry1D& geom, DataStructure& dataStructure, DataInitOption vertexDataInitOption, DataInitOption edgeDataInitOption)
{
  InitializeNode0DGeometry(geom, dataStructure, vertexDataInitOption);

  usize numOfEdges = geom.getNumberOfVertices() / geom.getNumberOfVerticesPerEdge();
  InitializeNodeGeometry<INodeGeometry1D, uint64>(
      geom, dataStructure, edgeDataInitOption, INodeGeometry1D::k_EdgeAttributeMatrixName, INodeGeometry1D::k_SharedEdgeListName, numOfEdges,
      [](const INodeGeometry1D& g) { return g.getNumberOfVerticesPerEdge(); }, [](INodeGeometry1D& g) { return g.getEdges(); },
      [](INodeGeometry1D& g, const IGeometry::SharedEdgeListType& da) { g.setEdgeList(da); }, [](INodeGeometry1D& g) { return g.getEdgeAttributeMatrix(); },
      [](INodeGeometry1D& g, AttributeMatrix& m) { g.setEdgeAttributeMatrix(m); });
}

static void InitializeNode2DGeometry(INodeGeometry2D& geom, DataStructure& dataStructure, DataInitOption vertexDataInitOption, DataInitOption edgeDataInitOption, DataInitOption faceDataInitOption)
{
  InitializeNode1DGeometry(geom, dataStructure, vertexDataInitOption, edgeDataInitOption);

  usize numOfFaces = geom.getNumberOfVertices() / geom.getNumberOfVerticesPerFace();
  InitializeNodeGeometry<INodeGeometry2D, uint64>(
      geom, dataStructure, faceDataInitOption, INodeGeometry2D::k_FaceAttributeMatrixName, INodeGeometry2D::k_SharedFacesListName, numOfFaces,
      [](const INodeGeometry2D& g) { return g.getNumberOfVerticesPerFace(); }, [](INodeGeometry2D& g) { return g.getFaces(); },
      [](INodeGeometry2D& g, const IGeometry::SharedFaceListType& da) { g.setFaceList(da); }, [](INodeGeometry2D& g) { return g.getFaceAttributeMatrix(); },
      [](INodeGeometry2D& g, AttributeMatrix& m) { g.setFaceAttributeMatrix(m); });
}

static void InitializeNode3DGeometry(INodeGeometry3D& geom, DataStructure& dataStructure, DataInitOption vertexDataInitOption, DataInitOption edgeDataInitOption, DataInitOption faceDataInitOption,
                                     DataInitOption polyDataInitOption)
{
  InitializeNode2DGeometry(geom, dataStructure, vertexDataInitOption, edgeDataInitOption, faceDataInitOption);

  usize numElements = geom.getNumberOfVertices() / geom.getNumberOfVerticesPerCell();
  InitializeNodeGeometry<INodeGeometry3D, uint64>(
      geom, dataStructure, polyDataInitOption, INodeGeometry3D::k_PolyhedronDataName, INodeGeometry3D::k_SharedPolyhedronListName, numElements,
      [](const INodeGeometry3D& g) { return g.getNumberOfVerticesPerCell(); }, [](INodeGeometry3D& g) { return g.getPolyhedra(); },
      [](INodeGeometry3D& g, const IGeometry::SharedFaceListType& da) { g.setPolyhedraList(da); }, [](INodeGeometry3D& g) { return g.getPolyhedraAttributeMatrix(); },
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

static void ValidateStringArraysImpl(const DataStructure& dataStructure, const DataPath& inputArrayPath1, const DataPath& inputArrayPath2, const DataPath& combinedArrayPath)
{
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<StringArray>(inputArrayPath1));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<StringArray>(inputArrayPath2));
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<StringArray>(combinedArrayPath));

  const auto& inputStringArray1 = dataStructure.getDataRefAs<StringArray>(inputArrayPath1);
  const auto& inputStringArray2 = dataStructure.getDataRefAs<StringArray>(inputArrayPath2);
  const auto& combinedStringArray = dataStructure.getDataRefAs<StringArray>(combinedArrayPath);

  REQUIRE(combinedStringArray.getNumberOfTuples() == inputStringArray1.getNumberOfTuples() + inputStringArray2.getNumberOfTuples());
  REQUIRE(std::equal(inputStringArray1.begin(), inputStringArray1.end(), combinedStringArray.begin()));
  REQUIRE(std::equal(inputStringArray2.begin(), inputStringArray2.end(), combinedStringArray.begin() + inputStringArray1.size()));
}

static void ValidateArrays(const DataStructure& dataStructure, const DataPath& inputArrayPath1, const DataPath& inputArrayPath2, const DataPath& combinedArrayPath)
{
  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IArray>(inputArrayPath1));
  const auto& array1 = dataStructure.getDataRefAs<IArray>(inputArrayPath1);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IArray>(inputArrayPath2));
  const auto& array2 = dataStructure.getDataRefAs<IArray>(inputArrayPath2);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<IArray>(combinedArrayPath));
  const auto& combinedArray = dataStructure.getDataRefAs<IArray>(combinedArrayPath);

  REQUIRE(combinedArray.getArrayType() == array1.getArrayType());
  REQUIRE(combinedArray.getArrayType() == array2.getArrayType());

  auto combinedArrayType = combinedArray.getArrayType();
  switch(combinedArrayType)
  {
  case IArray::ArrayType::DataArray: {
    const auto& dataArray1 = dataStructure.getDataRefAs<IDataArray>(inputArrayPath1);
    const auto& dataArray2 = dataStructure.getDataRefAs<IDataArray>(inputArrayPath2);
    const auto& combinedDataArray = dataStructure.getDataRefAs<IDataArray>(combinedArrayPath);
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
    const auto& nl1 = dataStructure.getDataRefAs<INeighborList>(inputArrayPath1);
    const auto& nl2 = dataStructure.getDataRefAs<INeighborList>(inputArrayPath2);
    const auto& combinedNL = dataStructure.getDataRefAs<INeighborList>(combinedArrayPath);
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
static void ValidateElementsArray(const DataArray<T>* elementsArray1Ptr, const DataArray<T>* elementsArray2Ptr, const DataArray<T>* combinedElementsArrayPtr, uint64 offset = 0)
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

static void ValidateAttributeMatrixArrays(const DataStructure& dataStructure, const AttributeMatrix* attrMatrix1, const AttributeMatrix* attrMatrix2, const AttributeMatrix* combinedAttrMatrix)
{
  bool allMatricesMissing = (attrMatrix1 == nullptr && attrMatrix2 == nullptr && combinedAttrMatrix == nullptr);
  if(allMatricesMissing)
  {
    // No arrays to compare
    return;
  }

  bool allMatricesExist = (attrMatrix1 != nullptr && attrMatrix2 != nullptr && combinedAttrMatrix != nullptr);
  REQUIRE(allMatricesExist);
  REQUIRE(combinedAttrMatrix->getNumberOfTuples() == attrMatrix1->getNumberOfTuples() + attrMatrix2->getNumberOfTuples());

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
static void CombineNodeBasedGeometriesImpl(const DataPath& inputGeom1Path, const DataPath& inputGeom2Path, const DataPath& outputGeomPath, DataStructure& dataStructure)
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

  auto* inputVertices1Ptr = inputGeom1.getVertices();
  REQUIRE(inputVertices1Ptr != nullptr);

  if constexpr(std::is_base_of_v<INodeGeometry0D, NodeGeom>)
  {
    auto* inputVertices2Ptr = inputGeom2.getVertices();
    auto* combinedVerticesPtr = combinedGeom.getVertices();
    ValidateElementsArray(inputVertices1Ptr, inputVertices2Ptr, combinedVerticesPtr);

    auto* vertexAM1Ptr = inputGeom1.getVertexAttributeMatrix();
    auto* vertexAM2Ptr = inputGeom2.getVertexAttributeMatrix();
    auto* combinedVertexAmPtr = combinedGeom.getVertexAttributeMatrix();
    ValidateAttributeMatrixArrays(dataStructure, vertexAM1Ptr, vertexAM2Ptr, combinedVertexAmPtr);
  }
  if constexpr(std::is_base_of_v<INodeGeometry1D, NodeGeom>)
  {
    auto* inputEdges1Ptr = inputGeom1.getEdges();
    auto* inputEdges2Ptr = inputGeom2.getEdges();
    auto* combinedEdgesPtr = combinedGeom.getEdges();
    ValidateElementsArray(inputEdges1Ptr, inputEdges2Ptr, combinedEdgesPtr, inputVertices1Ptr->getNumberOfTuples());

    auto* edgesAM1Ptr = inputGeom1.getEdgeAttributeMatrix();
    auto* edgesAM2Ptr = inputGeom2.getEdgeAttributeMatrix();
    auto* combinedEdgesAmPtr = combinedGeom.getEdgeAttributeMatrix();
    ValidateAttributeMatrixArrays(dataStructure, edgesAM1Ptr, edgesAM2Ptr, combinedEdgesAmPtr);
  }
  if constexpr(std::is_base_of_v<INodeGeometry2D, NodeGeom>)
  {
    auto* inputFaces1Ptr = inputGeom1.getFaces();
    auto* inputFaces2Ptr = inputGeom2.getFaces();
    auto* combinedFacesPtr = combinedGeom.getFaces();
    ValidateElementsArray(inputFaces1Ptr, inputFaces2Ptr, combinedFacesPtr, inputVertices1Ptr->getNumberOfTuples());

    auto* facesAM1Ptr = inputGeom1.getFaceAttributeMatrix();
    auto* facesAM2Ptr = inputGeom2.getFaceAttributeMatrix();
    auto* combinedFacesAmPtr = combinedGeom.getFaceAttributeMatrix();
    ValidateAttributeMatrixArrays(dataStructure, facesAM1Ptr, facesAM2Ptr, combinedFacesAmPtr);
  }
  if constexpr(std::is_base_of_v<INodeGeometry3D, NodeGeom>)
  {
    auto* inputPolyhedra1Ptr = inputGeom1.getPolyhedra();
    auto* inputPolyhedra2Ptr = inputGeom2.getPolyhedra();
    auto* combinedPolyhedraPtr = combinedGeom.getPolyhedra();
    ValidateElementsArray(inputPolyhedra1Ptr, inputPolyhedra2Ptr, combinedPolyhedraPtr, inputVertices1Ptr->getNumberOfTuples());

    auto* polyAM1Ptr = inputGeom1.getPolyhedraAttributeMatrix();
    auto* polyAM2Ptr = inputGeom2.getPolyhedraAttributeMatrix();
    auto* combinedPolyAmPtr = combinedGeom.getPolyhedraAttributeMatrix();
    ValidateAttributeMatrixArrays(dataStructure, polyAM1Ptr, polyAM2Ptr, combinedPolyAmPtr);
  }
}

template <typename NodeGeom>
static void InitializeNodeBasedGeometries(DataStructure& dataStructure)
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
static void LoadAndExecute0DNodeGeometriesTest(const fs::path& inputFilePath)
{
  DataStructure dataStructure = unit_test::LoadDataStructure(inputFilePath);
  InitializeNodeBasedGeometries<NodeGeom>(dataStructure);

  // Test two 0D node-based geometries
  CombineNodeBasedGeometriesImpl<NodeGeom>(k_InputGeometry1Path, k_InputGeometry2Path, k_OutputGeometryPath4, dataStructure);

  unit_test::CheckArraysInheritTupleDims(dataStructure);

  SUCCEED("Test completed successfully.");
}

template <typename NodeGeom>
static void LoadAndExecute1DNodeGeometriesTest(const fs::path& inputFilePath)
{
  DataStructure dataStructure = unit_test::LoadDataStructure(inputFilePath);
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

  unit_test::CheckArraysInheritTupleDims(dataStructure);

  SUCCEED("Test completed successfully.");
}

template <typename NodeGeom>
static void LoadAndExecute2DNodeGeometriesTest(const fs::path& inputFilePath)
{
  DataStructure dataStructure = unit_test::LoadDataStructure(inputFilePath);
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

  unit_test::CheckArraysInheritTupleDims(dataStructure);

  SUCCEED("Test completed successfully.");
}

template <typename NodeGeom>
static void LoadAndExecute3DNodeGeometriesTest(const fs::path& inputFilePath)
{
  DataStructure dataStructure = unit_test::LoadDataStructure(inputFilePath);
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

  unit_test::CheckArraysInheritTupleDims(dataStructure);

  SUCCEED("Test completed successfully.");
}

TEST_CASE("Combine Node Geometries: Vertex", "[SimplnxCore][CombineNodeBasedGeometries]")
{
  static const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "combine_node_based_geometries.tar.gz", "combine_vertex_geometries.dream3d");
  auto inputFilePath = fs::path(fmt::format("{}/combine_node_based_geometries/combine_vertex_geometries.dream3d", unit_test::k_TestFilesDir));
  LoadAndExecute0DNodeGeometriesTest<VertexGeom>(inputFilePath);
}

TEST_CASE("Combine Node Geometries: Edge", "[SimplnxCore][CombineNodeBasedGeometries]")
{
  static const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "combine_node_based_geometries.tar.gz", "combine_edge_geometries.dream3d");
  auto inputFilePath = fs::path(fmt::format("{}/combine_node_based_geometries/combine_edge_geometries.dream3d", unit_test::k_TestFilesDir));
  LoadAndExecute1DNodeGeometriesTest<EdgeGeom>(inputFilePath);
}

TEST_CASE("Combine Node Geometries: Triangle", "[SimplnxCore][CombineNodeBasedGeometries]")
{
  static const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "combine_node_based_geometries.tar.gz", "combine_triangle_geometries.dream3d");
  auto inputFilePath = fs::path(fmt::format("{}/combine_node_based_geometries/combine_triangle_geometries.dream3d", unit_test::k_TestFilesDir));
  LoadAndExecute2DNodeGeometriesTest<TriangleGeom>(inputFilePath);
}

TEST_CASE("Combine Node Geometries: Quad", "[SimplnxCore][CombineNodeBasedGeometries]")
{
  static const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "combine_node_based_geometries.tar.gz", "combine_quad_geometries.dream3d");
  auto inputFilePath = fs::path(fmt::format("{}/combine_node_based_geometries/combine_quad_geometries.dream3d", unit_test::k_TestFilesDir));
  LoadAndExecute2DNodeGeometriesTest<QuadGeom>(inputFilePath);
}

TEST_CASE("Combine Node Geometries: Tetrahedral", "[SimplnxCore][CombineNodeBasedGeometries]")
{
  static const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "combine_node_based_geometries.tar.gz", "combine_tetrahedral_geometries.dream3d");
  auto inputFilePath = fs::path(fmt::format("{}/combine_node_based_geometries/combine_tetrahedral_geometries.dream3d", unit_test::k_TestFilesDir));
  LoadAndExecute3DNodeGeometriesTest<TetrahedralGeom>(inputFilePath);
}

TEST_CASE("Combine Node Geometries: Hexahedral", "[SimplnxCore][CombineNodeBasedGeometries]")
{
  static const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "combine_node_based_geometries.tar.gz", "combine_hexahedral_geometries.dream3d");

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
    auto* geom1Ptr = VertexGeom::Create(dataStructure, k_Geometry1Name);
    auto* verticesPtr = CreateNodeArray<float32>(*geom1Ptr, dataStructure, INodeGeometry0D::k_SharedVertexListName, 10, 3);
    geom1Ptr->setVertices(*verticesPtr);

    auto* geom2Ptr = EdgeGeom::Create(dataStructure, k_Geometry2Name);
    verticesPtr = CreateNodeArray<float32>(*geom2Ptr, dataStructure, INodeGeometry0D::k_SharedVertexListName, 10, 3);
    geom2Ptr->setVertices(*verticesPtr);

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
        auto* geom1Ptr = EdgeGeom::Create(dataStructure, k_Geometry1Name);
        auto* geom2Ptr = EdgeGeom::Create(dataStructure, k_Geometry2Name);

        auto* verticesPtr = CreateNodeArray<float32>(*geom1Ptr, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom1Ptr->getNumberOfVerticesPerEdge(), 3);
        geom1Ptr->setVertices(*verticesPtr);
        verticesPtr = CreateNodeArray<float32>(*geom2Ptr, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom2Ptr->getNumberOfVerticesPerEdge(), 3);
        geom2Ptr->setVertices(*verticesPtr);

        auto* edgesPtr = CreateNodeArray<uint64>(*geom1Ptr, dataStructure, INodeGeometry1D::k_SharedEdgeListName, 1, geom1Ptr->getNumberOfVerticesPerEdge());
        geom1Ptr->setEdgeList(*edgesPtr);
      }
      SECTION("2D Geometry")
      {
        auto* geom1Ptr = TriangleGeom::Create(dataStructure, k_Geometry1Name);
        auto* geom2Ptr = TriangleGeom::Create(dataStructure, k_Geometry2Name);

        auto* verticesPtr = CreateNodeArray<float32>(*geom1Ptr, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom1Ptr->getNumberOfVerticesPerFace(), 3);
        geom1Ptr->setVertices(*verticesPtr);
        verticesPtr = CreateNodeArray<float32>(*geom2Ptr, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom2Ptr->getNumberOfVerticesPerFace(), 3);
        geom2Ptr->setVertices(*verticesPtr);

        auto* facesPtr = CreateNodeArray<uint64>(*geom1Ptr, dataStructure, INodeGeometry2D::k_SharedFacesListName, 1, geom1Ptr->getNumberOfVerticesPerFace());
        geom1Ptr->setFaceList(*facesPtr);
      }

      SECTION("3D Geometry")
      {
        auto* geom1Ptr = HexahedralGeom::Create(dataStructure, k_Geometry1Name);
        auto* geom2Ptr = HexahedralGeom::Create(dataStructure, k_Geometry2Name);

        auto* verticesPtr = CreateNodeArray<float32>(*geom1Ptr, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom1Ptr->getNumberOfVerticesPerCell(), 3);
        geom1Ptr->setVertices(*verticesPtr);
        verticesPtr = CreateNodeArray<float32>(*geom2Ptr, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom2Ptr->getNumberOfVerticesPerCell(), 3);
        geom2Ptr->setVertices(*verticesPtr);

        auto* polyhedraPtr = CreateNodeArray<uint64>(*geom1Ptr, dataStructure, INodeGeometry3D::k_SharedPolyhedronListName, 1, geom1Ptr->getNumberOfVerticesPerCell());
        geom1Ptr->setPolyhedraList(*polyhedraPtr);
      }

      errorCode = CombineNodeBasedGeometries::ErrorCodes::InconsistentGeometryElements;
    }
    SECTION("Vertex Attribute Matrix")
    {
      auto* geom1Ptr = TetrahedralGeom::Create(dataStructure, k_Geometry1Name);
      auto* geom2Ptr = TetrahedralGeom::Create(dataStructure, k_Geometry2Name);

      auto* verticesPtr = CreateNodeArray<float32>(*geom1Ptr, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom1Ptr->getNumberOfVerticesPerCell(), 3);
      geom1Ptr->setVertices(*verticesPtr);
      verticesPtr = CreateNodeArray<float32>(*geom2Ptr, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom2Ptr->getNumberOfVerticesPerCell(), 3);
      geom2Ptr->setVertices(*verticesPtr);

      auto* vertexAttrMatrixPtr = CreateNodeAttributeMatrix(*geom1Ptr, dataStructure, INodeGeometry0D::k_VertexAttributeMatrixName, geom1Ptr->getNumberOfVerticesPerCell());
      geom1Ptr->setVertexAttributeMatrix(*vertexAttrMatrixPtr);

      errorCode = CombineNodeBasedGeometries::ErrorCodes::InconsistentGeometryElements;
    }
    SECTION("Vertex Data Arrays")
    {
      SECTION("Data Array Missing")
      {
        auto* geom1Ptr = EdgeGeom::Create(dataStructure, k_Geometry1Name);
        auto* geom2Ptr = EdgeGeom::Create(dataStructure, k_Geometry2Name);

        auto* verticesPtr = CreateNodeArray<float32>(*geom1Ptr, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom1Ptr->getNumberOfVerticesPerEdge(), 3);
        geom1Ptr->setVertices(*verticesPtr);
        verticesPtr = CreateNodeArray<float32>(*geom2Ptr, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom2Ptr->getNumberOfVerticesPerEdge(), 3);
        geom2Ptr->setVertices(*verticesPtr);

        auto* vertexAttrMatrixPtr = CreateNodeAttributeMatrix(*geom1Ptr, dataStructure, INodeGeometry0D::k_VertexAttributeMatrixName, geom1Ptr->getNumberOfVerticesPerEdge());
        geom1Ptr->setVertexAttributeMatrix(*vertexAttrMatrixPtr);

        vertexAttrMatrixPtr = CreateNodeAttributeMatrix(*geom2Ptr, dataStructure, INodeGeometry0D::k_VertexAttributeMatrixName, geom2Ptr->getNumberOfVerticesPerEdge());
        geom2Ptr->setVertexAttributeMatrix(*vertexAttrMatrixPtr);

        AddDataArray<uint8, 2>(*vertexAttrMatrixPtr, dataStructure);

        errorCode = CombineNodeBasedGeometries::ErrorCodes::InconsistentGeometryElements;
      }
      SECTION("Mismatching Data Types")
      {
        auto* geom1Ptr = QuadGeom::Create(dataStructure, k_Geometry1Name);
        auto* geom2Ptr = QuadGeom::Create(dataStructure, k_Geometry2Name);

        auto* verticesPtr = CreateNodeArray<float32>(*geom1Ptr, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom1Ptr->getNumberOfVerticesPerFace(), 3);
        geom1Ptr->setVertices(*verticesPtr);
        verticesPtr = CreateNodeArray<float32>(*geom2Ptr, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom2Ptr->getNumberOfVerticesPerFace(), 3);
        geom2Ptr->setVertices(*verticesPtr);

        auto* vertexAttrMatrixPtr = CreateNodeAttributeMatrix(*geom1Ptr, dataStructure, INodeGeometry0D::k_VertexAttributeMatrixName, geom1Ptr->getNumberOfVerticesPerFace());
        geom1Ptr->setVertexAttributeMatrix(*vertexAttrMatrixPtr);

        CreateDataArray<uint8, 2>(*vertexAttrMatrixPtr, dataStructure, "TestArray");

        vertexAttrMatrixPtr = CreateNodeAttributeMatrix(*geom2Ptr, dataStructure, INodeGeometry0D::k_VertexAttributeMatrixName, geom2Ptr->getNumberOfVerticesPerFace());
        geom2Ptr->setVertexAttributeMatrix(*vertexAttrMatrixPtr);

        CreateDataArray<uint16, 2>(*vertexAttrMatrixPtr, dataStructure, "TestArray");

        errorCode = CombineNodeBasedGeometries::ErrorCodes::InconsistentGeometryElementDataTypes;
      }
      SECTION("Mismatching Array Types")
      {
        auto* geom1Ptr = TriangleGeom::Create(dataStructure, k_Geometry1Name);
        auto* geom2Ptr = TriangleGeom::Create(dataStructure, k_Geometry2Name);

        auto* verticesPtr = CreateNodeArray<float32>(*geom1Ptr, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom1Ptr->getNumberOfVerticesPerFace(), 3);
        geom1Ptr->setVertices(*verticesPtr);
        verticesPtr = CreateNodeArray<float32>(*geom2Ptr, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom2Ptr->getNumberOfVerticesPerFace(), 3);
        geom2Ptr->setVertices(*verticesPtr);

        auto* vertexAttrMatrixPtr = CreateNodeAttributeMatrix(*geom1Ptr, dataStructure, INodeGeometry0D::k_VertexAttributeMatrixName, geom1Ptr->getNumberOfVerticesPerFace());
        geom1Ptr->setVertexAttributeMatrix(*vertexAttrMatrixPtr);

        CreateDataArray<uint8, 2>(*vertexAttrMatrixPtr, dataStructure, "TestArray");

        vertexAttrMatrixPtr = CreateNodeAttributeMatrix(*geom2Ptr, dataStructure, INodeGeometry0D::k_VertexAttributeMatrixName, geom2Ptr->getNumberOfVerticesPerFace());
        geom2Ptr->setVertexAttributeMatrix(*vertexAttrMatrixPtr);

        CreateStringArray(*vertexAttrMatrixPtr, dataStructure, "TestArray");

        errorCode = CombineNodeBasedGeometries::ErrorCodes::InconsistentGeometryElementArrayTypes;
      }
      SECTION("Mismatching Component Dimensions")
      {
        auto* geom1Ptr = TriangleGeom::Create(dataStructure, k_Geometry1Name);
        auto* geom2Ptr = TriangleGeom::Create(dataStructure, k_Geometry2Name);

        auto* verticesPtr = CreateNodeArray<float32>(*geom1Ptr, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom1Ptr->getNumberOfVerticesPerFace(), 3);
        geom1Ptr->setVertices(*verticesPtr);
        verticesPtr = CreateNodeArray<float32>(*geom2Ptr, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom2Ptr->getNumberOfVerticesPerFace(), 3);
        geom2Ptr->setVertices(*verticesPtr);

        auto* vertexAttrMatrixPtr = CreateNodeAttributeMatrix(*geom1Ptr, dataStructure, INodeGeometry0D::k_VertexAttributeMatrixName, geom1Ptr->getNumberOfVerticesPerFace());
        geom1Ptr->setVertexAttributeMatrix(*vertexAttrMatrixPtr);

        CreateDataArray<int32>(*vertexAttrMatrixPtr, dataStructure, "TestArray", {2, 3});

        vertexAttrMatrixPtr = CreateNodeAttributeMatrix(*geom2Ptr, dataStructure, INodeGeometry0D::k_VertexAttributeMatrixName, geom2Ptr->getNumberOfVerticesPerFace());
        geom2Ptr->setVertexAttributeMatrix(*vertexAttrMatrixPtr);

        CreateDataArray<int32>(*vertexAttrMatrixPtr, dataStructure, "TestArray", {4, 5});

        errorCode = CombineNodeBasedGeometries::ErrorCodes::InconsistentGeometryElementCompDims;
      }
    }
    SECTION("Cell Data Arrays")
    {
      SECTION("Data Array Missing")
      {
        auto* geom1Ptr = EdgeGeom::Create(dataStructure, k_Geometry1Name);
        auto* geom2Ptr = EdgeGeom::Create(dataStructure, k_Geometry2Name);

        auto* verticesPtr = CreateNodeArray<float32>(*geom1Ptr, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom1Ptr->getNumberOfVerticesPerEdge(), 3);
        geom1Ptr->setVertices(*verticesPtr);
        verticesPtr = CreateNodeArray<float32>(*geom2Ptr, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom2Ptr->getNumberOfVerticesPerEdge(), 3);
        geom2Ptr->setVertices(*verticesPtr);

        auto* cellAttrMatrixPtr = CreateNodeAttributeMatrix(*geom1Ptr, dataStructure, INodeGeometry0D::k_VertexAttributeMatrixName, 1);
        geom1Ptr->setEdgeAttributeMatrix(*cellAttrMatrixPtr);

        cellAttrMatrixPtr = CreateNodeAttributeMatrix(*geom2Ptr, dataStructure, INodeGeometry0D::k_VertexAttributeMatrixName, 1);
        geom2Ptr->setEdgeAttributeMatrix(*cellAttrMatrixPtr);

        AddDataArray<uint8, 2>(*cellAttrMatrixPtr, dataStructure);

        errorCode = CombineNodeBasedGeometries::ErrorCodes::InconsistentGeometryElements;
      }
      SECTION("Mismatching Data Types")
      {
        auto* geom1Ptr = QuadGeom::Create(dataStructure, k_Geometry1Name);
        auto* geom2Ptr = QuadGeom::Create(dataStructure, k_Geometry2Name);

        auto* verticesPtr = CreateNodeArray<float32>(*geom1Ptr, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom1Ptr->getNumberOfVerticesPerFace(), 3);
        geom1Ptr->setVertices(*verticesPtr);
        verticesPtr = CreateNodeArray<float32>(*geom2Ptr, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom2Ptr->getNumberOfVerticesPerFace(), 3);
        geom2Ptr->setVertices(*verticesPtr);

        auto* cellAttrMatrixPtr = CreateNodeAttributeMatrix(*geom1Ptr, dataStructure, INodeGeometry0D::k_VertexAttributeMatrixName, 1);
        geom1Ptr->setFaceAttributeMatrix(*cellAttrMatrixPtr);

        CreateDataArray<uint8, 2>(*cellAttrMatrixPtr, dataStructure, "TestArray");

        cellAttrMatrixPtr = CreateNodeAttributeMatrix(*geom2Ptr, dataStructure, INodeGeometry0D::k_VertexAttributeMatrixName, 1);
        geom2Ptr->setFaceAttributeMatrix(*cellAttrMatrixPtr);

        CreateDataArray<uint16, 2>(*cellAttrMatrixPtr, dataStructure, "TestArray");

        errorCode = CombineNodeBasedGeometries::ErrorCodes::InconsistentGeometryElementDataTypes;
      }
      SECTION("Mismatching Array Types")
      {
        auto* geom1Ptr = TriangleGeom::Create(dataStructure, k_Geometry1Name);
        auto* geom2Ptr = TriangleGeom::Create(dataStructure, k_Geometry2Name);

        auto* verticesPtr = CreateNodeArray<float32>(*geom1Ptr, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom1Ptr->getNumberOfVerticesPerFace(), 3);
        geom1Ptr->setVertices(*verticesPtr);
        verticesPtr = CreateNodeArray<float32>(*geom2Ptr, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom2Ptr->getNumberOfVerticesPerFace(), 3);
        geom2Ptr->setVertices(*verticesPtr);

        auto* cellAttrMatrixPtr = CreateNodeAttributeMatrix(*geom1Ptr, dataStructure, INodeGeometry0D::k_VertexAttributeMatrixName, 1);
        geom1Ptr->setFaceAttributeMatrix(*cellAttrMatrixPtr);

        CreateDataArray<uint8, 2>(*cellAttrMatrixPtr, dataStructure, "TestArray");

        cellAttrMatrixPtr = CreateNodeAttributeMatrix(*geom2Ptr, dataStructure, INodeGeometry0D::k_VertexAttributeMatrixName, 1);
        geom2Ptr->setFaceAttributeMatrix(*cellAttrMatrixPtr);

        CreateStringArray(*cellAttrMatrixPtr, dataStructure, "TestArray");

        errorCode = CombineNodeBasedGeometries::ErrorCodes::InconsistentGeometryElementArrayTypes;
      }
      SECTION("Mismatching Component Dimensions")
      {
        auto* geom1Ptr = TriangleGeom::Create(dataStructure, k_Geometry1Name);
        auto* geom2Ptr = TriangleGeom::Create(dataStructure, k_Geometry2Name);

        auto* verticesPtr = CreateNodeArray<float32>(*geom1Ptr, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom1Ptr->getNumberOfVerticesPerFace(), 3);
        geom1Ptr->setVertices(*verticesPtr);
        verticesPtr = CreateNodeArray<float32>(*geom2Ptr, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom2Ptr->getNumberOfVerticesPerFace(), 3);
        geom2Ptr->setVertices(*verticesPtr);

        auto* cellAttrMatrixPtr = CreateNodeAttributeMatrix(*geom1Ptr, dataStructure, INodeGeometry0D::k_VertexAttributeMatrixName, 1);
        geom1Ptr->setFaceAttributeMatrix(*cellAttrMatrixPtr);

        CreateDataArray<int32>(*cellAttrMatrixPtr, dataStructure, "TestArray", {2, 3});

        cellAttrMatrixPtr = CreateNodeAttributeMatrix(*geom2Ptr, dataStructure, INodeGeometry0D::k_VertexAttributeMatrixName, 1);
        geom2Ptr->setFaceAttributeMatrix(*cellAttrMatrixPtr);

        CreateDataArray<int32>(*cellAttrMatrixPtr, dataStructure, "TestArray", {4, 5});

        errorCode = CombineNodeBasedGeometries::ErrorCodes::InconsistentGeometryElementCompDims;
      }
    }
    SECTION("Cell Attribute Matrix")
    {
      auto* geom1Ptr = HexahedralGeom::Create(dataStructure, k_Geometry1Name);
      auto* geom2Ptr = HexahedralGeom::Create(dataStructure, k_Geometry2Name);

      auto* verticesPtr = CreateNodeArray<float32>(*geom1Ptr, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom1Ptr->getNumberOfVerticesPerCell(), 3);
      geom1Ptr->setVertices(*verticesPtr);
      verticesPtr = CreateNodeArray<float32>(*geom2Ptr, dataStructure, INodeGeometry0D::k_SharedVertexListName, geom2Ptr->getNumberOfVerticesPerCell(), 3);
      geom2Ptr->setVertices(*verticesPtr);

      auto* vertexAttrMatrixPtr = CreateNodeAttributeMatrix(*geom1Ptr, dataStructure, INodeGeometry0D::k_VertexAttributeMatrixName, 1);
      geom1Ptr->setPolyhedraAttributeMatrix(*vertexAttrMatrixPtr);

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

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::Combine Node Geometries: Missing Required Lists", "[SimplnxCore][CombineNodeBasedGeometries]")
{
  // Every input has one vertex but none has the element list that defines its geometry type.
  const auto geometryType = GENERATE(IGeometry::Type::Edge, IGeometry::Type::Triangle, IGeometry::Type::Quad, IGeometry::Type::Tetrahedral, IGeometry::Type::Hexahedral);
  CAPTURE(geometryType);
  DataStructure dataStructure;
  for(const auto& name : {k_Geometry1Name, k_Geometry2Name})
  {
    INodeGeometry0D* geomPtr = nullptr;
    switch(geometryType)
    {
    case IGeometry::Type::Edge:
      geomPtr = EdgeGeom::Create(dataStructure, name);
      break;
    case IGeometry::Type::Triangle:
      geomPtr = TriangleGeom::Create(dataStructure, name);
      break;
    case IGeometry::Type::Quad:
      geomPtr = QuadGeom::Create(dataStructure, name);
      break;
    case IGeometry::Type::Tetrahedral:
      geomPtr = TetrahedralGeom::Create(dataStructure, name);
      break;
    case IGeometry::Type::Hexahedral:
      geomPtr = HexahedralGeom::Create(dataStructure, name);
      break;
    default:
      FAIL("Unexpected geometry type");
    }
    REQUIRE(geomPtr != nullptr);
    auto* verticesPtr = CreateNodeArray<float32>(*geomPtr, dataStructure, INodeGeometry0D::k_SharedVertexListName, 1, 3);
    REQUIRE(verticesPtr != nullptr);
    geomPtr->setVertices(*verticesPtr);
  }
  const int32 expectedCode = to_underlying(CombineNodeBasedGeometries::ErrorCodes::NodeGeometryHasNoElements);
  CombineNodeBasedGeometriesFilter filter;
  Arguments args;
  args.insertOrAssign(CombineNodeBasedGeometriesFilter::k_InputGeometries_Key, std::vector<DataPath>{k_InputGeometry1Path, k_InputGeometry2Path});
  args.insertOrAssign(CombineNodeBasedGeometriesFilter::k_OutputGeometryPath_Key, k_OutputGeometryPath1);
  auto preflight = filter.preflight(dataStructure, args);
  // CHECK (not REQUIRE) so that execute also runs; execute must return the same error and not crash.
  CHECK(preflight.output_actions.invalid());
  if(preflight.output_actions.invalid())
  {
    CHECK((preflight.output_actions.errors().size() == 1 && preflight.output_actions.errors()[0].code == expectedCode));
    if(!preflight.output_actions.errors().empty())
    {
      const auto& message = preflight.output_actions.errors()[0].message;
      CHECK(message.find(k_InputGeometry1Path.toString()) != std::string::npos);
      CHECK(message.find(k_InputGeometry2Path.toString()) != std::string::npos);
      CHECK(message.find("has an element list (the array that defines its elements, usually '") != std::string::npos);
      CHECK(message.find(fmt::format("Select {} geometries that have an element list.", GeometryTypeToString(geometryType))) != std::string::npos);
    }
  }
  auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(result.result);
  REQUIRE(result.result.errors()[0].code == expectedCode);
  unit_test::CheckArraysInheritTupleDims(dataStructure);
}
