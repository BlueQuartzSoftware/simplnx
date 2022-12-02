#include <catch2/catch.hpp>

#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/ExtractVertexGeometryFilter.hpp"

using namespace complex;

namespace
{
const std::string k_ImageGeometryName = "ImageGeometry";
const std::string k_WrongGeometryName = "TriangleGeometry";
const std::string k_CellAttrMatName = "CellData";
const std::string k_CellAttrMat2Name = "CellData2";
const std::string k_WrongAttrMatName = "WrongAttrMatrix";
const std::string k_FloatArrayName = "FloatArray";
const std::string k_MaskArrayName = "MaskArray";
const DataPath k_VertexDataContainerPath = {{"VertexDataContainer"}};
const DataPath k_BadDataContainerName({"BadDataContainer"});
const DataPath k_WrongGeomDataContainerName({"WrongGeomDataContainer"});
const std::string k_FeatureAttrMatName = "Feature Attr Mat";
const std::string k_FeatureArrayName = "featureData";
const int32 k_MoveArrays = 0;
const int32 k_CopyArrays = 1;

// -----------------------------------------------------------------------------
DataStructure createDataStructure()
{
  DataStructure dataStructure;

  // Create an ImageGeometry
  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, k_ImageGeometryName);
  std::vector<usize> dims = {10ULL, 20ULL, 30ULL};
  usize cellCount = std::accumulate(dims.begin(), dims.end(), static_cast<usize>(1), std::multiplies<usize>());
  imageGeom->setDimensions(dims);

  // Create wrong geometry
  TriangleGeom::Create(dataStructure, k_WrongGeometryName);

  // Create the Cell AttributeMatrix
  AttributeMatrix* cellAttrMat = AttributeMatrix::Create(dataStructure, k_CellAttrMatName, imageGeom->getId());
  cellAttrMat->setShape(dims);

  // Generate a "mask"
  BoolArray* maskData = BoolArray::CreateWithStore<BoolDataStore>(dataStructure, k_MaskArrayName, {cellCount}, {1}, cellAttrMat->getId());
  maskData->fill(true);
  (*maskData)[1] = false;
  (*maskData)[4] = false;
  (*maskData)[9] = false;
  (*maskData)[13] = false;
  (*maskData)[14] = false;

  AttributeMatrix* cellAttrMat2 = AttributeMatrix::Create(dataStructure, k_CellAttrMat2Name, imageGeom->getId());
  cellAttrMat2->setShape(dims);

  // Create a cell attribute array
  Float32Array* f32Data = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, k_FloatArrayName, {cellCount}, {1}, cellAttrMat->getId());
  f32Data->fill(45.243f);

  Float32Array* f32Data2 = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, k_FloatArrayName, {cellCount}, {1}, cellAttrMat2->getId());
  f32Data2->fill(45.243f);

  AttributeMatrix* wrongTuplesAttrMatrix = AttributeMatrix::Create(dataStructure, k_WrongAttrMatName, imageGeom->getId());
  wrongTuplesAttrMatrix->setShape({3});

  Float32Array* wrongTupleCountData = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, k_FloatArrayName, {3}, {1}, wrongTuplesAttrMatrix->getId());
  BoolArray* maskWrongTupleCountData = BoolArray::CreateWithStore<BoolDataStore>(dataStructure, k_MaskArrayName, {3}, {1}, wrongTuplesAttrMatrix->getId());

  Float32Array* noParentData = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, k_FloatArrayName, {cellCount}, {1});

  Float32Array* wrongParentData = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, k_FloatArrayName, {cellCount}, {1}, imageGeom->getId());

  return dataStructure;
}
} // namespace

TEST_CASE("Core::ExtractVertexGeometry: Data Array With No Parent", "[Core][ExtractVertexGeometry]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ExtractVertexGeometryFilter filter;
  DataStructure ds = createDataStructure();
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ExtractVertexGeometryFilter::k_ArrayHandling_Key, std::make_any<ChoicesParameter::ValueType>(k_MoveArrays));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_InputGeometryPath_Key, std::make_any<DataPath>(DataPath{{k_ImageGeometryName}}));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_IncludedDataArrayPaths_Key,
                      std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{DataPath{{k_FloatArrayName}}}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.invalid());
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -2004);
}

TEST_CASE("Core::ExtractVertexGeometry: Data Array With No AttributeMatrix Parent", "[Core][ExtractVertexGeometry]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ExtractVertexGeometryFilter filter;
  DataStructure ds = createDataStructure();
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ExtractVertexGeometryFilter::k_ArrayHandling_Key, std::make_any<ChoicesParameter::ValueType>(k_MoveArrays));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_InputGeometryPath_Key, std::make_any<DataPath>(DataPath{{k_ImageGeometryName}}));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_IncludedDataArrayPaths_Key,
                      std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{DataPath{{k_ImageGeometryName, k_FloatArrayName}}}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.invalid());
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -2005);
}

TEST_CASE("Core::ExtractVertexGeometry: Data Array With Wrong Tuple Count", "[Core][ExtractVertexGeometry]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ExtractVertexGeometryFilter filter;
  DataStructure ds = createDataStructure();
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ExtractVertexGeometryFilter::k_ArrayHandling_Key, std::make_any<ChoicesParameter::ValueType>(k_MoveArrays));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_InputGeometryPath_Key, std::make_any<DataPath>(DataPath{{k_ImageGeometryName}}));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_IncludedDataArrayPaths_Key,
                      std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{DataPath{{k_ImageGeometryName, k_WrongAttrMatName, k_FloatArrayName}}}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.invalid());
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -2006);
}

TEST_CASE("Core::ExtractVertexGeometry: Mask Array With Wrong Tuple Count", "[Core][ExtractVertexGeometry]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ExtractVertexGeometryFilter filter;
  DataStructure ds = createDataStructure();
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ExtractVertexGeometryFilter::k_ArrayHandling_Key, std::make_any<ChoicesParameter::ValueType>(k_MoveArrays));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_InputGeometryPath_Key, std::make_any<DataPath>(DataPath{{k_ImageGeometryName}}));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_IncludedDataArrayPaths_Key,
                      std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{DataPath{{k_ImageGeometryName, k_CellAttrMatName, k_FloatArrayName}}}));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_UseMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath{{k_ImageGeometryName, k_WrongAttrMatName, k_MaskArrayName}}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.invalid());
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -2003);
}

TEST_CASE("Core::ExtractVertexGeometry: Conflicting Attribute Matrices", "[Core][ExtractVertexGeometry]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ExtractVertexGeometryFilter filter;
  DataStructure ds = createDataStructure();
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ExtractVertexGeometryFilter::k_ArrayHandling_Key, std::make_any<ChoicesParameter::ValueType>(k_MoveArrays));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_InputGeometryPath_Key, std::make_any<DataPath>(DataPath{{k_ImageGeometryName}}));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_IncludedDataArrayPaths_Key,
                      std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{DataPath{{k_ImageGeometryName, k_CellAttrMatName, k_FloatArrayName}},
                                                                                                                     DataPath{{k_ImageGeometryName, k_CellAttrMat2Name, k_FloatArrayName}}}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.invalid());
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -2007);
}

TEST_CASE("Core::ExtractVertexGeometry: Move cell data arrays", "[Core][ExtractVertexGeometry]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ExtractVertexGeometryFilter filter;
  DataStructure ds = createDataStructure();
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ExtractVertexGeometryFilter::k_ArrayHandling_Key, std::make_any<ChoicesParameter::ValueType>(k_MoveArrays));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_InputGeometryPath_Key, std::make_any<DataPath>(DataPath{{k_ImageGeometryName}}));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_IncludedDataArrayPaths_Key,
                      std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{DataPath{{k_ImageGeometryName, k_CellAttrMatName, k_FloatArrayName}}}));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_VertexGeometryPath_Key, std::make_any<DataPath>(DataPath{k_VertexDataContainerPath}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());

  REQUIRE_THROWS(ds.getDataRefAs<Float32Array>(DataPath{{k_ImageGeometryName, k_CellAttrMatName, k_FloatArrayName}}));
  REQUIRE_NOTHROW(ds.getDataRefAs<Float32Array>(k_VertexDataContainerPath.createChildPath(k_CellAttrMatName).createChildPath(k_FloatArrayName)));
}

TEST_CASE("Core::ExtractVertexGeometry: Copy cell data arrays", "[Core][ExtractVertexGeometry]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ExtractVertexGeometryFilter filter;
  DataStructure ds = createDataStructure();
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ExtractVertexGeometryFilter::k_ArrayHandling_Key, std::make_any<ChoicesParameter::ValueType>(k_CopyArrays));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_InputGeometryPath_Key, std::make_any<DataPath>(DataPath{{k_ImageGeometryName}}));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_IncludedDataArrayPaths_Key,
                      std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{DataPath{{k_ImageGeometryName, k_CellAttrMatName, k_FloatArrayName}}}));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_VertexGeometryPath_Key, std::make_any<DataPath>(DataPath{k_VertexDataContainerPath}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());

  REQUIRE_NOTHROW(ds.getDataRefAs<Float32Array>(DataPath{{k_ImageGeometryName, k_CellAttrMatName, k_FloatArrayName}}));
  REQUIRE_NOTHROW(ds.getDataRefAs<Float32Array>(k_VertexDataContainerPath.createChildPath(k_CellAttrMatName).createChildPath(k_FloatArrayName)));

  const Float32Array& srcDataArray = ds.getDataRefAs<Float32Array>(k_VertexDataContainerPath.createChildPath(k_CellAttrMatName).createChildPath(k_FloatArrayName));
  const Float32Array& destDataArray = ds.getDataRefAs<Float32Array>(k_VertexDataContainerPath.createChildPath(k_CellAttrMatName).createChildPath(k_FloatArrayName));
  REQUIRE(srcDataArray.getTupleShape() == destDataArray.getTupleShape());
  REQUIRE(srcDataArray.getComponentShape() == destDataArray.getComponentShape());
  REQUIRE(srcDataArray.getSize() == destDataArray.getSize());

  for(usize i = 0; i < srcDataArray.getSize(); i++)
  {
    REQUIRE(srcDataArray[i] == destDataArray[i]);
  }
}

TEST_CASE("Core::ExtractVertexGeometry: Move cell data arrays with mask", "[Core][ExtractVertexGeometry]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ExtractVertexGeometryFilter filter;
  DataStructure ds = createDataStructure();
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ExtractVertexGeometryFilter::k_ArrayHandling_Key, std::make_any<ChoicesParameter::ValueType>(k_MoveArrays));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_InputGeometryPath_Key, std::make_any<DataPath>(DataPath{{k_ImageGeometryName}}));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_IncludedDataArrayPaths_Key,
                      std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{DataPath{{k_ImageGeometryName, k_CellAttrMatName, k_FloatArrayName}}}));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_VertexGeometryPath_Key, std::make_any<DataPath>(DataPath{k_VertexDataContainerPath}));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_UseMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath{{k_ImageGeometryName, k_CellAttrMatName, k_MaskArrayName}}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());

  REQUIRE_NOTHROW(ds.getDataRefAs<Float32Array>(DataPath{{k_ImageGeometryName, k_CellAttrMatName, k_FloatArrayName}}));
  REQUIRE_NOTHROW(ds.getDataRefAs<Float32Array>(k_VertexDataContainerPath.createChildPath(k_CellAttrMatName).createChildPath(k_FloatArrayName)));
  REQUIRE_NOTHROW(ds.getDataRefAs<BoolArray>(DataPath{{k_ImageGeometryName, k_CellAttrMatName, k_MaskArrayName}}));
}

TEST_CASE("Core::ExtractVertexGeometry: Copy cell data arrays with mask", "[Core][ExtractVertexGeometry]")
{
  // Instantiate the filter, a DataStructure object and an Arguments Object
  ExtractVertexGeometryFilter filter;
  DataStructure ds = createDataStructure();
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ExtractVertexGeometryFilter::k_ArrayHandling_Key, std::make_any<ChoicesParameter::ValueType>(k_CopyArrays));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_InputGeometryPath_Key, std::make_any<DataPath>(DataPath{{k_ImageGeometryName}}));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_IncludedDataArrayPaths_Key,
                      std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{DataPath{{k_ImageGeometryName, k_CellAttrMatName, k_FloatArrayName}}}));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_VertexGeometryPath_Key, std::make_any<DataPath>(DataPath{k_VertexDataContainerPath}));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_UseMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath{{k_ImageGeometryName, k_CellAttrMatName, k_MaskArrayName}}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(ds, args);
  REQUIRE(preflightResult.outputActions.valid());

  // Execute the filter and check the result
  auto executeResult = filter.execute(ds, args);
  REQUIRE(executeResult.result.valid());

  REQUIRE_NOTHROW(ds.getDataRefAs<Float32Array>(DataPath{{k_ImageGeometryName, k_CellAttrMatName, k_FloatArrayName}}));
  REQUIRE_NOTHROW(ds.getDataRefAs<Float32Array>(k_VertexDataContainerPath.createChildPath(k_CellAttrMatName).createChildPath(k_FloatArrayName)));
  REQUIRE_NOTHROW(ds.getDataRefAs<BoolArray>(DataPath{{k_ImageGeometryName, k_CellAttrMatName, k_MaskArrayName}}));

  const Float32Array& srcDataArray = ds.getDataRefAs<Float32Array>(DataPath{{k_ImageGeometryName, k_CellAttrMatName, k_FloatArrayName}});
  const Float32Array& destDataArray = ds.getDataRefAs<Float32Array>(k_VertexDataContainerPath.createChildPath(k_CellAttrMatName).createChildPath(k_FloatArrayName));
  const BoolArray& maskArray = ds.getDataRefAs<BoolArray>(DataPath{{k_ImageGeometryName, k_CellAttrMatName, k_MaskArrayName}});
  usize validTuples = std::count(maskArray.begin(), maskArray.end(), true);
  REQUIRE(srcDataArray.getTupleShape() == maskArray.getTupleShape());
  REQUIRE(destDataArray.getTupleShape() == std::vector<usize>{validTuples});
  REQUIRE(srcDataArray.getComponentShape() == destDataArray.getComponentShape());
  REQUIRE(destDataArray.getComponentShape() == maskArray.getComponentShape());
  REQUIRE(srcDataArray.getSize() == maskArray.getSize());
  REQUIRE(destDataArray.getSize() == validTuples);

  usize destIdx = 0;
  for(usize i = 0; i < srcDataArray.getSize(); i++)
  {
    if(maskArray[i])
    {
      REQUIRE(srcDataArray[i] == destDataArray[destIdx]);
      destIdx++;
    }
  }
}
