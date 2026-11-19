#include "OrientationAnalysis/Filters/Algorithms/ComputeGBCDPoleFigureDirect.hpp"
#include "OrientationAnalysis/Filters/Algorithms/ComputeGBCDPoleFigureScanline.hpp"
#include "OrientationAnalysis/Filters/ComputeGBCDPoleFigureFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

#include <catch2/catch.hpp>

#include <EbsdLib/Core/EbsdLibConstants.h>

#include <atomic>
#include <filesystem>
#include <fstream>
#include <limits>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::UnitTest;

namespace
{
constexpr StringLiteral k_FaceEnsembleDataPath("FaceEnsembleData [NX]");

constexpr StringLiteral k_PoleFigureSigma3("GBCD Pole Figure [Sigma 3]");
constexpr StringLiteral k_PoleFigureSigma9("GBCD Pole Figure [Sigma 9]");
constexpr StringLiteral k_PoleFigureSigma11("GBCD Pole Figure [Sigma 11]");
constexpr StringLiteral k_ExemplarCellData("Cell Data");
constexpr StringLiteral k_Sigma3("Sigma 3");
constexpr StringLiteral k_Sigma9("Sigma 9");
constexpr StringLiteral k_Sigma11("Sigma 11");
constexpr StringLiteral k_MRD("MRD");

constexpr int32 k_Dimension = 300;

} // namespace

TEST_CASE("OrientationAnalysis::ComputeGBCDPoleFigureFilter", "[OrientationAnalysis][ComputeGBCDPoleFigureFilter]")
{
  UnitTest::LoadPlugins();

  // AlgorithmTestScope forces the selected path and records its target-call
  // witness.
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_Small_IN100_GBCD.tar.gz", "6_6_Small_IN100_GBCD");

  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_Small_IN100_GBCD/6_6_Small_IN100_GBCD.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = UnitTest::LoadDataStructure(baseDataFilePath);
  DataPath smallIn100Group({nx::core::Constants::k_SmallIN100});
  DataPath featureDataPath = smallIn100Group.createChildPath(Constants::k_Grain_Data);
  DataPath avgEulerAnglesPath = featureDataPath.createChildPath(Constants::k_AvgEulerAngles);
  DataPath featurePhasesPath = featureDataPath.createChildPath(Constants::k_Phases);

  DataPath ensembleDataPath = smallIn100Group.createChildPath(Constants::k_Phase_Data);
  DataPath crystalStructurePath = ensembleDataPath.createChildPath(Constants::k_CrystalStructures);

  DataPath triangleDataContainerPath({Constants::k_TriangleDataContainerName});
  DataPath faceDataGroup = triangleDataContainerPath.createChildPath(Constants::k_FaceData);
  DataPath faceEnsemblePath = triangleDataContainerPath.createChildPath(k_FaceEnsembleDataPath);

  DataPath faceLabels = faceDataGroup.createChildPath(Constants::k_FaceLabels);
  DataPath faceNormals = faceDataGroup.createChildPath(Constants::k_FaceNormals);
  DataPath faceAreas = faceDataGroup.createChildPath(Constants::k_FaceAreas);

  DataPath gbcdArrayPath = triangleDataContainerPath.createChildPath("FaceEnsembleData").createChildPath(Constants::k_GBCD_Name);

  DataPath nxPoleFigureSigma3({"NX GBCD Pole Figure [Sigma 3]"});
  DataPath nxPoleFigureSigma9({"NX GBCD Pole Figure [Sigma 9]"});
  DataPath nxPoleFigureSigma11({"NX GBCD Pole Figure [Sigma 11]"});

  // Sigma 3 uses a 60-degree rotation about [111].
  {
    ComputeGBCDPoleFigureFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_PhaseOfInterest_Key, std::make_any<Int32Parameter::ValueType>(1));
    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_MisorientationRotation_Key, std::make_any<VectorFloat32Parameter::ValueType>({60.0F, 1.0F, 1.0F, 1.0F}));
    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_OutputImageDimension_Key, std::make_any<int32>(k_Dimension));
    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_GBCDArrayPath_Key, std::make_any<ArraySelectionParameter::ValueType>(gbcdArrayPath));
    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_CrystalStructuresArrayPath_Key, std::make_any<ArraySelectionParameter::ValueType>(crystalStructurePath));
    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_ImageGeometryName_Key, std::make_any<DataGroupCreationParameter::ValueType>(nxPoleFigureSigma3));
    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_CellAttributeMatrixName_Key, std::make_any<DataObjectNameParameter::ValueType>(Constants::k_CellData));
    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_CellIntensityArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_Sigma3));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(nxPoleFigureSigma3));
    CHECK(dataStructure.getDataRefAs<ImageGeom>(nxPoleFigureSigma3).getUnits() == IGeometry::LengthUnit::Unspecified);
  }

  {
    const DataPath k_GeneratedDataPath = nxPoleFigureSigma3.createChildPath(Constants::k_CellData).createChildPath(k_Sigma3);
    const DataPath k_ExemplarArrayPath({k_PoleFigureSigma3, k_ExemplarCellData, k_MRD});

    UnitTest::CompareFloatArraysWithNans<float64>(dataStructure, k_ExemplarArrayPath, k_GeneratedDataPath);
  }

  // Sigma 9 uses a 39-degree rotation about [110].
  {
    ComputeGBCDPoleFigureFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_PhaseOfInterest_Key, std::make_any<Int32Parameter::ValueType>(1));
    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_MisorientationRotation_Key, std::make_any<VectorFloat32Parameter::ValueType>({39.0F, 1.0F, 1.0F, 0.0F}));
    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_OutputImageDimension_Key, std::make_any<int32>(k_Dimension));
    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_GBCDArrayPath_Key, std::make_any<ArraySelectionParameter::ValueType>(gbcdArrayPath));
    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_CrystalStructuresArrayPath_Key, std::make_any<ArraySelectionParameter::ValueType>(crystalStructurePath));
    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_ImageGeometryName_Key, std::make_any<DataGroupCreationParameter::ValueType>(nxPoleFigureSigma9));
    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_CellAttributeMatrixName_Key, std::make_any<DataObjectNameParameter::ValueType>(Constants::k_CellData));
    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_CellIntensityArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_Sigma9));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(nxPoleFigureSigma9));
    CHECK(dataStructure.getDataRefAs<ImageGeom>(nxPoleFigureSigma9).getUnits() == IGeometry::LengthUnit::Unspecified);
  }

  {
    const DataPath k_GeneratedDataPath = nxPoleFigureSigma9.createChildPath(Constants::k_CellData).createChildPath(k_Sigma9);
    const DataPath k_ExemplarArrayPath({k_PoleFigureSigma9, k_ExemplarCellData, k_MRD});

    UnitTest::CompareFloatArraysWithNans<float64>(dataStructure, k_ExemplarArrayPath, k_GeneratedDataPath);
  }

  // Sigma 11 uses a 50.5-degree rotation about [110].
  {
    ComputeGBCDPoleFigureFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_PhaseOfInterest_Key, std::make_any<Int32Parameter::ValueType>(1));
    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_MisorientationRotation_Key, std::make_any<VectorFloat32Parameter::ValueType>({50.5F, 1.0F, 1.0F, 0.0F}));
    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_OutputImageDimension_Key, std::make_any<int32>(k_Dimension));
    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_GBCDArrayPath_Key, std::make_any<ArraySelectionParameter::ValueType>(gbcdArrayPath));
    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_CrystalStructuresArrayPath_Key, std::make_any<ArraySelectionParameter::ValueType>(crystalStructurePath));
    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_ImageGeometryName_Key, std::make_any<DataGroupCreationParameter::ValueType>(nxPoleFigureSigma11));
    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_CellAttributeMatrixName_Key, std::make_any<DataObjectNameParameter::ValueType>(Constants::k_CellData));
    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_CellIntensityArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_Sigma11));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(nxPoleFigureSigma11));
    CHECK(dataStructure.getDataRefAs<ImageGeom>(nxPoleFigureSigma11).getUnits() == IGeometry::LengthUnit::Unspecified);
  }

  {
    const DataPath k_GeneratedDataPath = nxPoleFigureSigma11.createChildPath(Constants::k_CellData).createChildPath(k_Sigma11);
    const DataPath k_ExemplarArrayPath({k_PoleFigureSigma11, k_ExemplarCellData, k_MRD});

    UnitTest::CompareFloatArraysWithNans<float64>(dataStructure, k_ExemplarArrayPath, k_GeneratedDataPath);
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/gbcd_pole_figure.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeGBCDPoleFigureFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][ComputeGBCDPoleFigureFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeGBCDPoleFigureFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeGBCDPoleFigureFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeGBCDPoleFigureFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      if(label == "SIMPL 6.5 (UUID)")
      {
        CHECK(args.value<int32>(ComputeGBCDPoleFigureFilter::k_OutputImageDimension_Key) == 5);
        CHECK(args.value<std::string>(ComputeGBCDPoleFigureFilter::k_CellAttributeMatrixName_Key) == "TestName");
        CHECK(args.value<std::string>(ComputeGBCDPoleFigureFilter::k_CellIntensityArrayName_Key) == "TestName");
      }
      CHECK(args.value<int32>(ComputeGBCDPoleFigureFilter::k_PhaseOfInterest_Key) == 5);
      // Pipeline loading verifies AxisAngleFilterParameterConverter<float32>.
      CHECK(args.value<DataPath>(ComputeGBCDPoleFigureFilter::k_GBCDArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeGBCDPoleFigureFilter::k_CrystalStructuresArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeGBCDPoleFigureFilter::k_ImageGeometryName_Key) == DataPath({"DataContainer"}));
    }
  }
}

TEST_CASE("OrientationAnalysis::ComputeGBCDPoleFigureFilter: Phase and Laue Index Bounds", "[OrientationAnalysis][ComputeGBCDPoleFigureFilter]")
{
  UnitTest::LoadPlugins();

  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  const UnitTest::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "6_6_Small_IN100_GBCD.tar.gz", "6_6_Small_IN100_GBCD");
  const fs::path inputFile = fs::path(unit_test::k_TestFilesDir.view()) / "6_6_Small_IN100_GBCD" / "6_6_Small_IN100_GBCD.dream3d";
  DataStructure dataStructure = UnitTest::LoadDataStructure(inputFile);

  const DataPath triangleDataContainerPath({Constants::k_TriangleDataContainerName});
  const DataPath gbcdArrayPath = triangleDataContainerPath.createChildPath("FaceEnsembleData").createChildPath(Constants::k_GBCD_Name);
  const DataPath crystalStructuresPath = DataPath({Constants::k_SmallIN100}).createChildPath(Constants::k_Phase_Data).createChildPath(Constants::k_CrystalStructures);
  const DataPath shortCrystalStructuresPath({"Short Crystal Structures"});
  auto* shortCrystalStructuresArrayPtr = UnitTest::CreateTestDataArray<uint32>(dataStructure, shortCrystalStructuresPath.getTargetName(), {1}, {1});
  (*shortCrystalStructuresArrayPtr)[0] = 999U;

  ComputeGBCDPoleFigureFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_PhaseOfInterest_Key, std::make_any<int32>(1));
  args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_MisorientationRotation_Key, std::make_any<VectorFloat32Parameter::ValueType>({60.0F, 1.0F, 1.0F, 1.0F}));
  args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_OutputImageDimension_Key, std::make_any<int32>(4));
  args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_GBCDArrayPath_Key, std::make_any<DataPath>(gbcdArrayPath));
  args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(crystalStructuresPath));
  args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_ImageGeometryName_Key, std::make_any<DataPath>(DataPath({"Bounds Pole Figure"})));
  args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>("Cell Data"));
  args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_CellIntensityArrayName_Key, std::make_any<std::string>("Intensity"));

  SECTION("Phase missing from Crystal Structures returns an error")
  {
    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(shortCrystalStructuresPath));
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
    REQUIRE(executeResult.result.errors()[0].code == -34643);
  }

  SECTION("Invalid Laue index returns an error")
  {
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt32Array>(crystalStructuresPath));
    auto& crystalStructuresArrayRef = dataStructure.getDataRefAs<UInt32Array>(crystalStructuresPath);
    crystalStructuresArrayRef.getDataStoreRef()[1] = 999U;
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
    REQUIRE(executeResult.result.errors()[0].code == -34644);
  }

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeGBCDPoleFigureFilter: Invalid GBCD Component Shape", "[OrientationAnalysis][ComputeGBCDPoleFigureFilter][ComponentShape]")
{
  const auto componentShape = GENERATE(ShapeType{1}, ShapeType{2, 2, 2, 2, 2, 1}, ShapeType{0, 2, 2, 2, 2, 2}, ShapeType{2, 0, 2, 2, 2, 2}, ShapeType{2, 2, 0, 2, 2, 2}, ShapeType{2, 2, 2, 0, 2, 2},
                                       ShapeType{2, 2, 2, 2, 0, 2});
  CAPTURE(componentShape);

  DataStructure dataStructure;
  const DataPath gbcdPath({"Malformed GBCD"});
  const DataPath crystalStructuresPath({"Crystal Structures"});
  auto* gbcd = UnitTest::CreateTestDataArray<float64>(dataStructure, gbcdPath.getTargetName(), {2}, componentShape);
  REQUIRE(gbcd != nullptr);
  auto* crystalStructures = UnitTest::CreateTestDataArray<uint32>(dataStructure, crystalStructuresPath.getTargetName(), {2}, {1});
  REQUIRE(crystalStructures != nullptr);

  ComputeGBCDPoleFigureFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_PhaseOfInterest_Key, std::make_any<int32>(1));
  args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_GBCDArrayPath_Key, std::make_any<DataPath>(gbcdPath));
  args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(crystalStructuresPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors().size() == 1);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -34645);
}

namespace
{
DataStructure CreateSyntheticGBCD()
{
  DataStructure dataStructure;
  auto* gbcd = UnitTest::CreateTestDataArray<float64>(dataStructure, "GBCD", {2}, {3, 3, 3, 4, 4, 2});
  for(usize index = 0; index < gbcd->getSize(); index++)
  {
    (*gbcd)[index] = static_cast<float64>(index % 97) + 1.0;
  }
  auto* crystalStructures = UnitTest::CreateTestDataArray<uint32>(dataStructure, "Crystal Structures", {2}, {1});
  (*crystalStructures)[0] = ebsdlib::CrystalStructure::UnknownCrystalStructure;
  (*crystalStructures)[1] = ebsdlib::CrystalStructure::Cubic_High;
  return dataStructure;
}

Arguments CreateSyntheticGBCDArguments(const std::vector<float32>& misorientationRotation, const std::string& geometryName)
{
  Arguments args = ComputeGBCDPoleFigureFilter().getDefaultArguments();
  args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_PhaseOfInterest_Key, std::make_any<int32>(1));
  args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_MisorientationRotation_Key, std::make_any<VectorFloat32Parameter::ValueType>(misorientationRotation));
  args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_OutputImageDimension_Key, std::make_any<int32>(16));
  args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_GBCDArrayPath_Key, std::make_any<DataPath>(DataPath({"GBCD"})));
  args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(DataPath({"Crystal Structures"})));
  args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_ImageGeometryName_Key, std::make_any<DataPath>(DataPath({geometryName})));
  args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_CellAttributeMatrixName_Key, std::make_any<std::string>("Cell Data"));
  args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_CellIntensityArrayName_Key, std::make_any<std::string>("MRD"));
  return args;
}
} // namespace

TEST_CASE("OrientationAnalysis::ComputeGBCDPoleFigureFilter: Invalid Misorientation Axis", "[OrientationAnalysis][ComputeGBCDPoleFigureFilter]")
{
  const std::vector<float32> misorientationRotation =
      GENERATE(std::vector<float32>{60.0F, 0.0F, 0.0F, 0.0F}, std::vector<float32>{60.0F, std::numeric_limits<float32>::quiet_NaN(), 1.0F, 1.0F},
               std::vector<float32>{60.0F, 1.0F, std::numeric_limits<float32>::infinity(), 1.0F}, std::vector<float32>{60.0F, 1.0F, 1.0F, -std::numeric_limits<float32>::infinity()});
  CAPTURE(misorientationRotation);

  DataStructure dataStructure = CreateSyntheticGBCD();
  ComputeGBCDPoleFigureFilter filter;
  const Arguments args = CreateSyntheticGBCDArguments(misorientationRotation, "Pole Figure");

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);
  REQUIRE(preflightResult.outputActions.errors()[0].code == -34646);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
  REQUIRE(executeResult.result.errors()[0].code == -34646);

  // Each executor rejects the axis when it runs without the filter.
  ComputeGBCDPoleFigureInputValues inputValues;
  inputValues.PhaseOfInterest = 1;
  inputValues.MisorientationRotation = misorientationRotation;
  inputValues.GBCDArrayPath = DataPath({"GBCD"});
  inputValues.CrystalStructuresArrayPath = DataPath({"Crystal Structures"});
  inputValues.OutputImageDimension = 16;
  inputValues.ImageGeometryPath = DataPath({"Pole Figure"});
  inputValues.CellAttributeMatrixName = "Cell Data";
  inputValues.CellIntensityArrayName = "MRD";
  const std::atomic_bool shouldCancel = false;
  Result<> directResult = ComputeGBCDPoleFigureDirect(dataStructure, {}, shouldCancel, &inputValues)();
  SIMPLNX_RESULT_REQUIRE_INVALID(directResult);
  REQUIRE(directResult.errors()[0].code == -34646);
  Result<> scanlineResult = ComputeGBCDPoleFigureScanline(dataStructure, {}, shouldCancel, &inputValues)();
  SIMPLNX_RESULT_REQUIRE_INVALID(scanlineResult);
  REQUIRE(scanlineResult.errors()[0].code == -34646);
}

TEST_CASE("OrientationAnalysis::ComputeGBCDPoleFigureFilter: Misorientation Axis Scale", "[OrientationAnalysis][ComputeGBCDPoleFigureFilter]")
{
  const auto scenario = GENERATE(from_range(UnitTest::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  UnitTest::AlgorithmTestScope scope(scenario);

  // The axis [1e20, 1e20, 1e20] is the axis [1, 1, 1]. Its squared length overflows float32.
  DataStructure dataStructure = CreateSyntheticGBCD();
  ComputeGBCDPoleFigureFilter filter;
  auto unitResult = scope.executeFilter(filter, dataStructure, CreateSyntheticGBCDArguments({60.0F, 1.0F, 1.0F, 1.0F}, "Unit Axis"));
  SIMPLNX_RESULT_REQUIRE_VALID(unitResult.result);
  auto scaledResult = scope.executeFilter(filter, dataStructure, CreateSyntheticGBCDArguments({60.0F, 1.0e20F, 1.0e20F, 1.0e20F}, "Scaled Axis"));
  SIMPLNX_RESULT_REQUIRE_VALID(scaledResult.result);

  const auto& unitMrd = dataStructure.getDataRefAs<Float64Array>(DataPath({"Unit Axis", "Cell Data", "MRD"}));
  const auto& scaledMrd = dataStructure.getDataRefAs<Float64Array>(DataPath({"Scaled Axis", "Cell Data", "MRD"}));
  REQUIRE(unitMrd.getSize() == scaledMrd.getSize());
  usize nonzeroCount = 0;
  for(usize index = 0; index < unitMrd.getSize(); index++)
  {
    REQUIRE(scaledMrd[index] == Approx(unitMrd[index]).margin(1.0e-9));
    nonzeroCount += unitMrd[index] != 0.0 ? 1 : 0;
  }
  REQUIRE(nonzeroCount > 0);
}
