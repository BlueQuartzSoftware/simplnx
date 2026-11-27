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

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::unit_test;

namespace
{
constexpr StringLiteralType k_FaceEnsembleDataPath("FaceEnsembleData [NX]");

constexpr StringLiteralType k_PoleFigureSigma3("GBCD Pole Figure [Sigma 3]");
constexpr StringLiteralType k_PoleFigureSigma9("GBCD Pole Figure [Sigma 9]");
constexpr StringLiteralType k_PoleFigureSigma11("GBCD Pole Figure [Sigma 11]");
constexpr StringLiteralType k_ExemplarCellData("Cell Data");
constexpr StringLiteralType k_Sigma3("Sigma 3");
constexpr StringLiteralType k_Sigma9("Sigma 9");
constexpr StringLiteralType k_Sigma11("Sigma 11");
constexpr StringLiteralType k_MRD("MRD");

constexpr int32 k_Dimension = 300;

} // namespace

TEST_CASE("OrientationAnalysis::ComputeGBCDPoleFigureFilter", "[OrientationAnalysis][ComputeGBCDPoleFigureFilter]")
{
  unit_test::LoadPlugins();

  // AlgorithmTestScope forces the selected path and records its target-call
  // witness.
  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  const nx::core::unit_test::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_Small_IN100_GBCD.tar.gz", "6_6_Small_IN100_GBCD");

  auto baseDataFilePath = fs::path(fmt::format("{}/6_6_Small_IN100_GBCD/6_6_Small_IN100_GBCD.dream3d", unit_test::k_TestFilesDir));
  DataStructure dataStructure = unit_test::LoadDataStructure(baseDataFilePath);
  DataPath smallIn100Group({nx::core::constants::k_SmallIN100});
  DataPath featureDataPath = smallIn100Group.createChildPath(constants::k_GrainDataWithSpaces);
  DataPath avgEulerAnglesPath = featureDataPath.createChildPath(constants::k_AvgEulerAngles);
  DataPath featurePhasesPath = featureDataPath.createChildPath(constants::k_Phases);

  DataPath ensembleDataPath = smallIn100Group.createChildPath(constants::k_PhaseData);
  DataPath crystalStructurePath = ensembleDataPath.createChildPath(constants::k_CrystalStructures);

  DataPath triangleDataContainerPath({constants::k_TriangleDataContainerName});
  DataPath faceDataGroup = triangleDataContainerPath.createChildPath(constants::k_FaceData);
  DataPath faceEnsemblePath = triangleDataContainerPath.createChildPath(k_FaceEnsembleDataPath);

  DataPath faceLabels = faceDataGroup.createChildPath(constants::k_FaceLabels);
  DataPath faceNormals = faceDataGroup.createChildPath(constants::k_FaceNormals);
  DataPath faceAreas = faceDataGroup.createChildPath(constants::k_FaceAreas);

  DataPath gbcdArrayPath = triangleDataContainerPath.createChildPath("FaceEnsembleData").createChildPath(constants::k_GbcdName);

  DataPath nxPoleFigureSigma3({"NX GBCD Pole Figure [Sigma 3]"});
  DataPath nxPoleFigureSigma9({"NX GBCD Pole Figure [Sigma 9]"});
  DataPath nxPoleFigureSigma11({"NX GBCD Pole Figure [Sigma 11]"});

  // Sigma 3 uses a 60-degree rotation about [111].
  {
    ComputeGBCDPoleFigureFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_PhaseOfInterest_Key, std::make_any<Int32ParameterType::ValueType>(1));
    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_MisorientationRotation_Key, std::make_any<VectorFloat32ParameterType::ValueType>({60.0F, 1.0F, 1.0F, 1.0F}));
    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_OutputImageDimension_Key, std::make_any<int32>(k_Dimension));
    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_GBCDArrayPath_Key, std::make_any<ArraySelectionParameter::ValueType>(gbcdArrayPath));
    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_CrystalStructuresArrayPath_Key, std::make_any<ArraySelectionParameter::ValueType>(crystalStructurePath));
    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_ImageGeometryName_Key, std::make_any<DataGroupCreationParameter::ValueType>(nxPoleFigureSigma3));
    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_CellAttributeMatrixName_Key, std::make_any<DataObjectNameParameter::ValueType>(constants::k_CellData));
    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_CellIntensityArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_Sigma3));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(nxPoleFigureSigma3));
    CHECK(dataStructure.getDataRefAs<ImageGeom>(nxPoleFigureSigma3).getUnits() == IGeometry::LengthUnit::Unspecified);
  }

  {
    const DataPath kGeneratedDataPath = nxPoleFigureSigma3.createChildPath(constants::k_CellData).createChildPath(k_Sigma3);
    const DataPath kExemplarArrayPath({k_PoleFigureSigma3, k_ExemplarCellData, k_MRD});

    unit_test::CompareFloatArraysWithNans<float64>(dataStructure, kExemplarArrayPath, kGeneratedDataPath);
  }

  // Sigma 9 uses a 39-degree rotation about [110].
  {
    ComputeGBCDPoleFigureFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_PhaseOfInterest_Key, std::make_any<Int32ParameterType::ValueType>(1));
    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_MisorientationRotation_Key, std::make_any<VectorFloat32ParameterType::ValueType>({39.0F, 1.0F, 1.0F, 0.0F}));
    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_OutputImageDimension_Key, std::make_any<int32>(k_Dimension));
    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_GBCDArrayPath_Key, std::make_any<ArraySelectionParameter::ValueType>(gbcdArrayPath));
    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_CrystalStructuresArrayPath_Key, std::make_any<ArraySelectionParameter::ValueType>(crystalStructurePath));
    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_ImageGeometryName_Key, std::make_any<DataGroupCreationParameter::ValueType>(nxPoleFigureSigma9));
    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_CellAttributeMatrixName_Key, std::make_any<DataObjectNameParameter::ValueType>(constants::k_CellData));
    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_CellIntensityArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_Sigma9));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(nxPoleFigureSigma9));
    CHECK(dataStructure.getDataRefAs<ImageGeom>(nxPoleFigureSigma9).getUnits() == IGeometry::LengthUnit::Unspecified);
  }

  {
    const DataPath kGeneratedDataPath = nxPoleFigureSigma9.createChildPath(constants::k_CellData).createChildPath(k_Sigma9);
    const DataPath kExemplarArrayPath({k_PoleFigureSigma9, k_ExemplarCellData, k_MRD});

    unit_test::CompareFloatArraysWithNans<float64>(dataStructure, kExemplarArrayPath, kGeneratedDataPath);
  }

  // Sigma 11 uses a 50.5-degree rotation about [110].
  {
    ComputeGBCDPoleFigureFilter filter;
    Arguments args;

    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_PhaseOfInterest_Key, std::make_any<Int32ParameterType::ValueType>(1));
    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_MisorientationRotation_Key, std::make_any<VectorFloat32ParameterType::ValueType>({50.5F, 1.0F, 1.0F, 0.0F}));
    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_OutputImageDimension_Key, std::make_any<int32>(k_Dimension));
    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_GBCDArrayPath_Key, std::make_any<ArraySelectionParameter::ValueType>(gbcdArrayPath));
    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_CrystalStructuresArrayPath_Key, std::make_any<ArraySelectionParameter::ValueType>(crystalStructurePath));
    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_ImageGeometryName_Key, std::make_any<DataGroupCreationParameter::ValueType>(nxPoleFigureSigma11));
    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_CellAttributeMatrixName_Key, std::make_any<DataObjectNameParameter::ValueType>(constants::k_CellData));
    args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_CellIntensityArrayName_Key, std::make_any<DataObjectNameParameter::ValueType>(k_Sigma11));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.output_actions);

    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<ImageGeom>(nxPoleFigureSigma11));
    CHECK(dataStructure.getDataRefAs<ImageGeom>(nxPoleFigureSigma11).getUnits() == IGeometry::LengthUnit::Unspecified);
  }

  {
    const DataPath kGeneratedDataPath = nxPoleFigureSigma11.createChildPath(constants::k_CellData).createChildPath(k_Sigma11);
    const DataPath kExemplarArrayPath({k_PoleFigureSigma11, k_ExemplarCellData, k_MRD});

    unit_test::CompareFloatArraysWithNans<float64>(dataStructure, kExemplarArrayPath, kGeneratedDataPath);
  }

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
  WriteTestDataStructure(dataStructure, fs::path(fmt::format("{}/gbcd_pole_figure.dream3d", unit_test::k_BinaryTestOutputDir)));
#endif

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeGBCDPoleFigureFilter: SIMPL Backwards Compatibility", "[OrientationAnalysis][ComputeGBCDPoleFigureFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  unit_test::LoadPlugins();
  auto* filterListPtr = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeGBCDPoleFigureFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeGBCDPoleFigureFilter.json"},
  };

  for(const auto& [label, fixturePath] : fixtures)
  {
    DYNAMIC_SECTION(label)
    {
      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterListPtr);
      REQUIRE(pipelineResult.valid());

      auto& pipeline = pipelineResult.value();
      REQUIRE(pipeline.size() == 1);

      auto* pipelineFilterPtr = dynamic_cast<PipelineFilter*>(pipeline.at(0));
      REQUIRE(pipelineFilterPtr != nullptr);

      const IFilter* filterPtr = pipelineFilterPtr->getFilter();
      REQUIRE(filterPtr != nullptr);
      REQUIRE(filterPtr->uuid() == FilterTraits<ComputeGBCDPoleFigureFilter>::uuid);

      CHECK(pipelineFilterPtr->getComments().empty());

      const Arguments args = pipelineFilterPtr->getArguments();
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
  unit_test::LoadPlugins();

  const auto scenario = GENERATE(from_range(unit_test::SelectAlgorithmTestScenariosForInMemoryStores()));
  CAPTURE(scenario);
  unit_test::AlgorithmTestScope scope(scenario);

  const unit_test::TestFileSentinel testDataSentinel(unit_test::k_TestFilesDir, "6_6_Small_IN100_GBCD.tar.gz", "6_6_Small_IN100_GBCD");
  const fs::path inputFile = fs::path(unit_test::k_TestFilesDir.view()) / "6_6_Small_IN100_GBCD" / "6_6_Small_IN100_GBCD.dream3d";
  DataStructure dataStructure = unit_test::LoadDataStructure(inputFile);

  const DataPath triangleDataContainerPath({constants::k_TriangleDataContainerName});
  const DataPath gbcdArrayPath = triangleDataContainerPath.createChildPath("FaceEnsembleData").createChildPath(constants::k_GbcdName);
  const DataPath crystalStructuresPath = DataPath({constants::k_SmallIN100}).createChildPath(constants::k_PhaseData).createChildPath(constants::k_CrystalStructures);
  const DataPath shortCrystalStructuresPath({"Short Crystal Structures"});
  auto* shortCrystalStructuresArrayPtr = unit_test::CreateTestDataArray<uint32>(dataStructure, shortCrystalStructuresPath.getTargetName(), {1}, {1});
  (*shortCrystalStructuresArrayPtr)[0] = 999U;

  ComputeGBCDPoleFigureFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_PhaseOfInterest_Key, std::make_any<int32>(1));
  args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_MisorientationRotation_Key, std::make_any<VectorFloat32ParameterType::ValueType>({60.0F, 1.0F, 1.0F, 1.0F}));
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
    REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt32ArrayType>(crystalStructuresPath));
    auto& crystalStructuresArrayRef = dataStructure.getDataRefAs<UInt32ArrayType>(crystalStructuresPath);
    crystalStructuresArrayRef.getDataStoreRef()[1] = 999U;
    auto executeResult = scope.executeFilter(filter, dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result);
    REQUIRE(executeResult.result.errors()[0].code == -34644);
  }

  unit_test::CheckArraysInheritTupleDims(dataStructure);
}

TEST_CASE("OrientationAnalysis::ComputeGBCDPoleFigureFilter: Invalid GBCD Component Shape", "[OrientationAnalysis][ComputeGBCDPoleFigureFilter][ComponentShape]")
{
  const auto componentShape = GENERATE(ShapeType{1}, ShapeType{2, 2, 2, 2, 2, 1}, ShapeType{0, 2, 2, 2, 2, 2}, ShapeType{2, 0, 2, 2, 2, 2}, ShapeType{2, 2, 0, 2, 2, 2}, ShapeType{2, 2, 2, 0, 2, 2},
                                       ShapeType{2, 2, 2, 2, 0, 2});
  CAPTURE(componentShape);

  DataStructure dataStructure;
  const DataPath gbcdPath({"Malformed GBCD"});
  const DataPath crystalStructuresPath({"Crystal Structures"});
  auto* gbcdPtr = unit_test::CreateTestDataArray<float64>(dataStructure, gbcdPath.getTargetName(), {2}, componentShape);
  REQUIRE(gbcdPtr != nullptr);
  auto* crystalStructuresPtr = unit_test::CreateTestDataArray<uint32>(dataStructure, crystalStructuresPath.getTargetName(), {2}, {1});
  REQUIRE(crystalStructuresPtr != nullptr);

  ComputeGBCDPoleFigureFilter filter;
  Arguments args = filter.getDefaultArguments();
  args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_PhaseOfInterest_Key, std::make_any<int32>(1));
  args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_GBCDArrayPath_Key, std::make_any<DataPath>(gbcdPath));
  args.insertOrAssign(ComputeGBCDPoleFigureFilter::k_CrystalStructuresArrayPath_Key, std::make_any<DataPath>(crystalStructuresPath));

  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.output_actions);
  REQUIRE(preflightResult.output_actions.errors().size() == 1);
  REQUIRE(preflightResult.output_actions.errors()[0].code == -34645);
}
