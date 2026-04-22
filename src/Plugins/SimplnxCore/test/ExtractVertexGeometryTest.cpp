#include "SimplnxCore/Filters/ExtractVertexGeometryFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

namespace
{
const std::string k_ImageGeometryName = "ImageGeometry";
const std::string k_WrongGeometryName = "TriangleGeometry";
const std::string k_CellDataName = "Cell Data";
const std::string k_MaskName = "Mask (IQ)";

const std::string k_CellAttrMatName = "CellData";
const std::string k_CellAttrMat2Name = "CellData2";
const std::string k_WrongAttrMatName = "WrongAttrMatrix";
const std::string k_FloatArrayName = "FloatArray";
const std::string k_MaskArrayName = "MaskArray";
// const DataPath k_VertexDataContainerPath = {{"VertexDataContainer"}};

const DataPath k_InputImageGeometryPath = DataPath({k_ImageGeometryName});
const DataPath k_InputMaskPath = DataPath({k_ImageGeometryName, k_CellDataName, k_MaskName});
const DataPath k_InputAttrMatPath = DataPath({k_ImageGeometryName, k_CellDataName});

const DataPath k_ComputedVertexDataPath = DataPath({"Computed Vertex Geometry"});
const std::string k_SharedVertexListName = "Shared Vertex List";
const std::string k_VertexAttrMatName = "Vertex Data";

std::vector<std::string> k_CopyMoveArrayNames = {"Confidence Index", "EulerAngles", "Image Quality", "Mask", "Mask (IQ)", "Phases"};

namespace ExtractVertexGeometryTest
{
// -----------------------------------------------------------------------------
DataStructure CreateDataStructure()
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
  auto cellAttrMatrixDims = std::vector<usize>(dims.rbegin(), dims.rend());
  AttributeMatrix* cellAttrMat = AttributeMatrix::Create(dataStructure, k_CellAttrMatName, cellAttrMatrixDims, imageGeom->getId());

  // Generate a "mask"
  BoolArray* maskData = BoolArray::CreateWithStore<BoolDataStore>(dataStructure, k_MaskArrayName, cellAttrMatrixDims, {1}, cellAttrMat->getId());
  maskData->fill(true);
  (*maskData)[1] = false;
  (*maskData)[4] = false;
  (*maskData)[9] = false;
  (*maskData)[13] = false;
  (*maskData)[14] = false;

  AttributeMatrix* cellAttrMat2 = AttributeMatrix::Create(dataStructure, k_CellAttrMat2Name, cellAttrMatrixDims, imageGeom->getId());

  // Create a cell attribute array
  Float32Array* f32Data = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, k_FloatArrayName, cellAttrMatrixDims, {1}, cellAttrMat->getId());
  f32Data->fill(45.243f);

  Float32Array* f32Data2 = Float32Array::CreateWithStore<Float32DataStore>(dataStructure, k_FloatArrayName, cellAttrMatrixDims, {1}, cellAttrMat2->getId());
  f32Data2->fill(45.243f);

  AttributeMatrix* wrongTuplesAttrMatrix = AttributeMatrix::Create(dataStructure, k_WrongAttrMatName, {3}, imageGeom->getId());

  Float32Array::CreateWithStore<Float32DataStore>(dataStructure, k_FloatArrayName, {3}, {1}, wrongTuplesAttrMatrix->getId());

  BoolArray::CreateWithStore<BoolDataStore>(dataStructure, k_MaskArrayName, {3}, {1}, wrongTuplesAttrMatrix->getId());

  Float32Array::CreateWithStore<Float32DataStore>(dataStructure, k_FloatArrayName, {cellCount}, {1});

  Float32Array::CreateWithStore<Float32DataStore>(dataStructure, k_FloatArrayName, {cellCount}, {1}, imageGeom->getId());

  return dataStructure;
}
} // namespace ExtractVertexGeometryTest
} // namespace

TEST_CASE("SimplnxCore::ExtractVertexGeometry: Data Array With Wrong Tuple Count", "[SimplnxCore][ExtractVertexGeometry]")
{
  UnitTest::LoadPlugins();

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ExtractVertexGeometryFilter filter;
  DataStructure dataStructure = ExtractVertexGeometryTest::CreateDataStructure();
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ExtractVertexGeometryFilter::k_ArrayHandling_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ArrayHandlingType::Move)));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_InputGeometryPath_Key, std::make_any<DataPath>(DataPath{{k_ImageGeometryName}}));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_IncludedDataArrayPaths_Key,
                      std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{DataPath{{k_ImageGeometryName, k_WrongAttrMatName, k_FloatArrayName}}}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.invalid());
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -2006);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ExtractVertexGeometry: Mask Array With Wrong Tuple Count", "[SimplnxCore][ExtractVertexGeometry]")
{
  UnitTest::LoadPlugins();

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ExtractVertexGeometryFilter filter;
  DataStructure dataStructure = ExtractVertexGeometryTest::CreateDataStructure();
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ExtractVertexGeometryFilter::k_ArrayHandling_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ArrayHandlingType::Move)));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_InputGeometryPath_Key, std::make_any<DataPath>(DataPath{{k_ImageGeometryName}}));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_IncludedDataArrayPaths_Key,
                      std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{DataPath{{k_ImageGeometryName, k_CellAttrMatName, k_FloatArrayName}}}));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_UseMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath{{k_ImageGeometryName, k_WrongAttrMatName, k_MaskArrayName}}));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  REQUIRE(preflightResult.outputActions.invalid());
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -651);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ExtractVertexGeometry: Copy cell data arrays", "[SimplnxCore][ExtractVertexGeometry]")
{
  UnitTest::LoadPlugins();
  //  Read Exemplar DREAM3D File Filter
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "extract_vertex_geometry.tar.gz", "extract_vertex_geometry");
  auto baseDataFilePath = fs::path(fmt::format("{}/extract_vertex_geometry/extract_vertex_geometry.dream3d", unit_test::k_TestFilesDir));

  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ExtractVertexGeometryFilter filter;
  Arguments args;

  MultiArraySelectionParameter::ValueType arrayPaths;
  for(const auto& name : k_CopyMoveArrayNames)
  {
    arrayPaths.push_back(k_InputAttrMatPath.createChildPath(name));
  }

  // Create default Parameters for the filter.
  args.insertOrAssign(ExtractVertexGeometryFilter::k_InputGeometryPath_Key, std::make_any<DataPath>(k_InputImageGeometryPath));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_UseMask_Key, std::make_any<bool>(false));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(k_InputMaskPath));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_ArrayHandling_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ArrayHandlingType::Copy)));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_IncludedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(arrayPaths));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_VertexGeometryPath_Key, std::make_any<DataPath>(k_ComputedVertexDataPath));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_SharedVertexListName_Key, std::make_any<std::string>(k_SharedVertexListName));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_VertexAttrMatrixName_Key, std::make_any<std::string>(k_VertexAttrMatName));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // Write out the .dream3d file now
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/extract_vertex_geometry_copy.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  {
    DataPath exemplarAttrMat = DataPath({"Exemplar_Copy", "Vertex Data"});
    UnitTest::CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarAttrMat, dataStructure, k_ComputedVertexDataPath.createChildPath(k_VertexAttrMatName), true);
  }

  {
    DataPath computedAttrMat = k_InputImageGeometryPath.createChildPath("Cell Data");
    DataPath exemplarAttrMat = DataPath({"ImageGeometry_Copy_Input", "Cell Data"});
    UnitTest::CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarAttrMat, dataStructure, computedAttrMat, true);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ExtractVertexGeometry: Copy cell data arrays with mask", "[SimplnxCore][ExtractVertexGeometry]")
{
  UnitTest::LoadPlugins();
  //  Read Exemplar DREAM3D File Filter
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "extract_vertex_geometry.tar.gz", "extract_vertex_geometry");
  auto baseDataFilePath = fs::path(fmt::format("{}/extract_vertex_geometry/extract_vertex_geometry.dream3d", unit_test::k_TestFilesDir));

  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ExtractVertexGeometryFilter filter;
  Arguments args;

  MultiArraySelectionParameter::ValueType arrayPaths;
  for(const auto& name : k_CopyMoveArrayNames)
  {
    arrayPaths.push_back(k_InputAttrMatPath.createChildPath(name));
  }

  // Create default Parameters for the filter.
  args.insertOrAssign(ExtractVertexGeometryFilter::k_InputGeometryPath_Key, std::make_any<DataPath>(k_InputImageGeometryPath));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_UseMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(k_InputMaskPath));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_ArrayHandling_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ArrayHandlingType::Copy)));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_IncludedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(arrayPaths));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_VertexGeometryPath_Key, std::make_any<DataPath>(k_ComputedVertexDataPath));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_SharedVertexListName_Key, std::make_any<std::string>(k_SharedVertexListName));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_VertexAttrMatrixName_Key, std::make_any<std::string>(k_VertexAttrMatName));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // Write out the .dream3d file now
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/extract_vertex_geometry_copy_mask.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  {
    DataPath exemplarAttrMat = DataPath({"Exemplar_Copy_Mask", "Vertex Data"});
    UnitTest::CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarAttrMat, dataStructure, k_ComputedVertexDataPath.createChildPath(k_VertexAttrMatName), true);
  }

  {
    DataPath computedAttrMat = k_InputImageGeometryPath.createChildPath("Cell Data");
    DataPath exemplarAttrMat = DataPath({"ImageGeometry_Copy_Mask_Input", "Cell Data"});
    UnitTest::CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarAttrMat, dataStructure, computedAttrMat, true);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ExtractVertexGeometry: Move cell data arrays", "[SimplnxCore][ExtractVertexGeometry]")
{
  UnitTest::LoadPlugins();
  //  Read Exemplar DREAM3D File Filter
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "extract_vertex_geometry.tar.gz", "extract_vertex_geometry");
  auto baseDataFilePath = fs::path(fmt::format("{}/extract_vertex_geometry/extract_vertex_geometry.dream3d", unit_test::k_TestFilesDir));

  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ExtractVertexGeometryFilter filter;
  Arguments args;

  MultiArraySelectionParameter::ValueType arrayPaths;
  for(const auto& name : k_CopyMoveArrayNames)
  {
    arrayPaths.push_back(k_InputAttrMatPath.createChildPath(name));
  }

  // Create default Parameters for the filter.
  args.insertOrAssign(ExtractVertexGeometryFilter::k_InputGeometryPath_Key, std::make_any<DataPath>(k_InputImageGeometryPath));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_UseMask_Key, std::make_any<bool>(false));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(k_InputMaskPath));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_ArrayHandling_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ArrayHandlingType::Move)));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_IncludedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(arrayPaths));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_VertexGeometryPath_Key, std::make_any<DataPath>(k_ComputedVertexDataPath));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_SharedVertexListName_Key, std::make_any<std::string>(k_SharedVertexListName));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_VertexAttrMatrixName_Key, std::make_any<std::string>(k_VertexAttrMatName));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // Write out the .dream3d file now
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/extract_vertex_geometry_move.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  {
    DataPath exemplarAttrMat = DataPath({"Exemplar_Move", "Vertex Data"});
    UnitTest::CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarAttrMat, dataStructure, k_ComputedVertexDataPath.createChildPath(k_VertexAttrMatName), true);
  }

  {
    DataPath computedAttrMat = k_InputImageGeometryPath.createChildPath("Cell Data");
    DataPath exemplarAttrMat = DataPath({"ImageGeometry_Move_Input", "Cell Data"});
    UnitTest::CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarAttrMat, dataStructure, computedAttrMat, true);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ExtractVertexGeometry: Move cell data arrays with mask", "[SimplnxCore][ExtractVertexGeometry]")
{
  UnitTest::LoadPlugins();
  //  Read Exemplar DREAM3D File Filter
  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "extract_vertex_geometry.tar.gz", "extract_vertex_geometry");
  auto baseDataFilePath = fs::path(fmt::format("{}/extract_vertex_geometry/extract_vertex_geometry.dream3d", unit_test::k_TestFilesDir));

  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);

  // Instantiate the filter, a DataStructure object and an Arguments Object
  ExtractVertexGeometryFilter filter;
  Arguments args;

  MultiArraySelectionParameter::ValueType arrayPaths;
  for(const auto& name : k_CopyMoveArrayNames)
  {
    arrayPaths.push_back(k_InputAttrMatPath.createChildPath(name));
  }

  // Create default Parameters for the filter.
  args.insertOrAssign(ExtractVertexGeometryFilter::k_InputGeometryPath_Key, std::make_any<DataPath>(k_InputImageGeometryPath));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_UseMask_Key, std::make_any<bool>(true));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(k_InputMaskPath));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_ArrayHandling_Key, std::make_any<ChoicesParameter::ValueType>(to_underlying(ArrayHandlingType::Move)));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_IncludedDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(arrayPaths));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_VertexGeometryPath_Key, std::make_any<DataPath>(k_ComputedVertexDataPath));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_SharedVertexListName_Key, std::make_any<std::string>(k_SharedVertexListName));
  args.insertOrAssign(ExtractVertexGeometryFilter::k_VertexAttrMatrixName_Key, std::make_any<std::string>(k_VertexAttrMatName));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  // Write out the .dream3d file now
#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fmt::format("{}/extract_vertex_geometry_move_mask.dream3d", unit_test::k_BinaryTestOutputDir));
#endif

  {
    DataPath exemplarAttrMat = DataPath({"Exemplar_Move_Mask", "Vertex Data"});
    UnitTest::CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarAttrMat, dataStructure, k_ComputedVertexDataPath.createChildPath(k_VertexAttrMatName), true);
  }

  {
    DataPath computedAttrMat = k_InputImageGeometryPath.createChildPath("Cell Data");
    DataPath exemplarAttrMat = DataPath({"ImageGeometry_Move_Mask_Input", "Cell Data"});
    UnitTest::CompareExemplarToGenerateAttributeMatrix(dataStructure, exemplarAttrMat, dataStructure, computedAttrMat, true);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("SimplnxCore::ExtractVertexGeometryFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ExtractVertexGeometryFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ExtractVertexGeometryFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ExtractVertexGeometryFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilter != nullptr);

      const IFilter* filter = pipelineFilter->getFilter();
      REQUIRE(filter != nullptr);
      REQUIRE(filter->uuid() == FilterTraits<ExtractVertexGeometryFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      if(label == "SIMPL 6.5 (UUID)")
      {
        CHECK(args.value<bool>(ExtractVertexGeometryFilter::k_UseMask_Key) == true);
        CHECK(args.value<DataPath>(ExtractVertexGeometryFilter::k_MaskArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      }
      // Complex type (MultiDataArraySelectionFilterParameterConverter) - verified by successful pipeline loading
      CHECK(args.value<DataPath>(ExtractVertexGeometryFilter::k_InputGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(ExtractVertexGeometryFilter::k_VertexGeometryPath_Key) == DataPath({"DataContainer"}));
    }
  }
}
