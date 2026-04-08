#include "OrientationAnalysis/Filters/ComputeGBCDPoleFigureFilter.hpp"
#include "OrientationAnalysis/OrientationAnalysis_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
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

  bool forceOocAlgo = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "6_6_Small_IN100_GBCD.tar.gz", "6_6_Small_IN100_GBCD");

  // Read the Small IN100 Data set
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

  // Create the Pole Figures for Sigma 3  60@[111]
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

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Compare the Output Pole Figure for Sigma 3
  {
    const DataPath k_GeneratedDataPath = nxPoleFigureSigma3.createChildPath(Constants::k_CellData).createChildPath(k_Sigma3);
    const DataPath k_ExemplarArrayPath({k_PoleFigureSigma3, k_ExemplarCellData, k_MRD});

    UnitTest::CompareFloatArraysWithNans<float64>(dataStructure, k_ExemplarArrayPath, k_GeneratedDataPath);
  }

  // Create the Pole Figures for Sigma 9  39@[110]
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

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Compare the Output Pole Figure for Sigma 9
  {
    const DataPath k_GeneratedDataPath = nxPoleFigureSigma9.createChildPath(Constants::k_CellData).createChildPath(k_Sigma9);
    const DataPath k_ExemplarArrayPath({k_PoleFigureSigma9, k_ExemplarCellData, k_MRD});

    UnitTest::CompareFloatArraysWithNans<float64>(dataStructure, k_ExemplarArrayPath, k_GeneratedDataPath);
  }

  // Create the Pole Figures for Sigma 11  50.5@[110]
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

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    // Execute the filter and check the result
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  // Compare the Output Pole Figure for Sigma 11
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
      // Complex type (AxisAngleFilterParameterConverter<float32>) - verified by successful pipeline loading
      CHECK(args.value<DataPath>(ComputeGBCDPoleFigureFilter::k_GBCDArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeGBCDPoleFigureFilter::k_CrystalStructuresArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      CHECK(args.value<DataPath>(ComputeGBCDPoleFigureFilter::k_ImageGeometryName_Key) == DataPath({"DataContainer"}));
    }
  }
}
