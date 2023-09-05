#include "ComplexCore/ComplexCore_test_dirs.hpp"
#include "ComplexCore/Filters/ResampleImageGeomFilter.hpp"

#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/DataGroupUtilities.hpp"
#include "complex/Utilities/FilterUtilities.hpp"

#include <catch2/catch.hpp>

#include <filesystem>

namespace fs = std::filesystem;
using namespace complex;
using namespace complex::UnitTest;

namespace
{
const ChoicesParameter::ValueType k_SpacingModeIndex = 0;
const ChoicesParameter::ValueType k_ScalingModeIndex = 1;
const ChoicesParameter::ValueType k_ExactDimensionsModeIndex = 2;
} // namespace

struct CompareDataArrayFunctor
{
  template <typename T>
  void operator()(const IDataArray& left, const IDataArray& right, usize start = 0)
  {
    UnitTest::CompareDataArrays<T>(left, right, start);
  }
};

TEST_CASE("ComplexCore::ResampleImageGeom: Invalid Parameters", "[ComplexCore][ResampleImageGeom]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "ResampleImageGeom_Exemplar.tar.gz",
                                                             "ResampleImageGeom_Exemplar.dream3d");

  ResampleImageGeomFilter filter;
  Arguments args;

  DataStructure dataStructure = LoadDataStructure(fs::path(fmt::format("{}/ResampleImageGeom_Exemplar.dream3d", unit_test::k_TestFilesDir)));
  DataPath srcGeomPath({Constants::k_SmallIN100});
  DataPath cellDataPath = srcGeomPath.createChildPath(Constants::k_EbsdScanData);
  DataPath cellFeatureDataPath = srcGeomPath.createChildPath(Constants::k_FeatureGroupName);
  DataPath featureIdsPath = cellDataPath.createChildPath(Constants::k_FeatureIds);
  DataPath destGeomPath({Constants::k_SmallIN100});
  DataPath destCellDataPath = destGeomPath.createChildPath(Constants::k_EbsdScanData);
  DataPath destCellFeatureDataPath = destGeomPath.createChildPath(Constants::k_FeatureGroupName);
  DataPath destNewGrainDataPath = destGeomPath.createChildPath("NewGrain Data");
  DataPath destPhaseDataPath = destGeomPath.createChildPath("Phase Data");

  int errCode;
  SECTION("Spacing")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_SpacingModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{-1.0F, -1.0F, -1.0F}));
    errCode = -11502;
  }
  SECTION("Scaling")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_ScalingModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_Scaling_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{-1.0F, -1.0F, -1.0F}));
    errCode = -11500;
  }

  // We do not need to test the "Exact Dimensions" resampling mode because the parameter type "VectorUInt64Parameter" automatically keeps the input values in range

  // Assign values to the filter parameters
  args.insertOrAssign(ResampleImageGeomFilter::k_RenumberFeatures_Key, std::make_any<bool>(true));
  args.insertOrAssign(ResampleImageGeomFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(true));
  args.insertOrAssign(ResampleImageGeomFilter::k_SelectedImageGeometry_Key, std::make_any<DataPath>(srcGeomPath));
  args.insertOrAssign(ResampleImageGeomFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsPath));
  args.insertOrAssign(ResampleImageGeomFilter::k_FeatureAttributeMatrix_Key, std::make_any<DataPath>(cellFeatureDataPath));
  args.insertOrAssign(ResampleImageGeomFilter::k_CreatedImageGeometry_Key, std::make_any<DataPath>(destGeomPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  auto preflightErrors = preflightResult.outputActions.errors();
  REQUIRE(preflightErrors.size() == 1);
  REQUIRE(preflightErrors[0].code == errCode);
}

TEST_CASE("ComplexCore::ResampleImageGeom: 3D In Place", "[ComplexCore][ResampleImageGeom]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "ResampleImageGeom_Exemplar.tar.gz",
                                                             "ResampleImageGeom_Exemplar.dream3d");

  ResampleImageGeomFilter filter;
  Arguments args;

  DataStructure dataStructure = LoadDataStructure(fs::path(fmt::format("{}/ResampleImageGeom_Exemplar.dream3d", unit_test::k_TestFilesDir)));
  DataPath srcGeomPath({Constants::k_SmallIN100});
  DataPath cellDataPath = srcGeomPath.createChildPath(Constants::k_EbsdScanData);
  DataPath cellFeatureDataPath = srcGeomPath.createChildPath(Constants::k_FeatureGroupName);
  DataPath featureIdsPath = cellDataPath.createChildPath(Constants::k_FeatureIds);
  DataPath destGeomPath({Constants::k_SmallIN100});
  DataPath destCellDataPath = destGeomPath.createChildPath(Constants::k_EbsdScanData);
  DataPath destCellFeatureDataPath = destGeomPath.createChildPath(Constants::k_FeatureGroupName);
  DataPath destNewGrainDataPath = destGeomPath.createChildPath("NewGrain Data");
  DataPath destPhaseDataPath = destGeomPath.createChildPath("Phase Data");

  // Create default Parameters for the filter.
  DataPath exemplarGeoPath;
  SECTION("Spacing")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_SpacingModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{1, 1, 1}));
    exemplarGeoPath = DataPath({"Resampled_3D_ImageGeom"});
  }
  SECTION("Scaling")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_ScalingModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_Scaling_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{25, 25, 25}));
    exemplarGeoPath = DataPath({"Resampled_3D_ImageGeom"});
  }
  SECTION("Exact Dimensions")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_ExactDimensionsModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_ExactDimensions_Key, std::make_any<VectorUInt64Parameter::ValueType>(std::vector<uint64>{47, 50, 1}));
    exemplarGeoPath = DataPath({"Resampled_3D_ImageGeom_ExactDimensions"});
  }
  args.insertOrAssign(ResampleImageGeomFilter::k_RenumberFeatures_Key, std::make_any<bool>(true));
  args.insertOrAssign(ResampleImageGeomFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(true));
  args.insertOrAssign(ResampleImageGeomFilter::k_SelectedImageGeometry_Key, std::make_any<DataPath>(srcGeomPath));
  args.insertOrAssign(ResampleImageGeomFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsPath));
  args.insertOrAssign(ResampleImageGeomFilter::k_FeatureAttributeMatrix_Key, std::make_any<DataPath>(cellFeatureDataPath));
  args.insertOrAssign(ResampleImageGeomFilter::k_CreatedImageGeometry_Key, std::make_any<DataPath>(destGeomPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)

  DataPath exemplarCellDataPath = exemplarGeoPath.createChildPath(Constants::k_EbsdScanData);
  DataPath exemplarCellFeatureDataPath = exemplarGeoPath.createChildPath(Constants::k_FeatureGroupName);

  // check that the geometry(s) and the attribute matrix are all there
  const auto srcGeo = dataStructure.getDataAs<ImageGeom>(srcGeomPath);
  const auto destGeo = dataStructure.getDataAs<ImageGeom>(destGeomPath);
  const auto destCellData = dataStructure.getDataAs<AttributeMatrix>(destCellDataPath);
  const auto destFeatureData = dataStructure.getDataAs<AttributeMatrix>(destCellFeatureDataPath);
  const auto destNewGrainData = dataStructure.getDataAs<AttributeMatrix>(destNewGrainDataPath);
  const auto destPhaseData = dataStructure.getDataAs<AttributeMatrix>(destPhaseDataPath);
  REQUIRE(srcGeo != nullptr);
  REQUIRE(destGeo != nullptr);
  REQUIRE(destCellData != nullptr);
  REQUIRE(destFeatureData != nullptr);
  REQUIRE(destNewGrainData != nullptr);
  REQUIRE(destPhaseData != nullptr);

  // check the resampled geometry dimensions
  const auto destSpacing = destGeo->getSpacing();
  const auto destDims = destGeo->getDimensions();
  const auto destCellDataDims = destCellData->getShape();
  const auto destFeatDataDims = destFeatureData->getShape();
  const auto destNewGrainDataDims = destNewGrainData->getShape();
  const auto destPhaseDataDims = destPhaseData->getShape();
  // Get the exemplar data
  const auto exemplarGeo = dataStructure.getDataAs<ImageGeom>(exemplarGeoPath);
  const auto exemplarCellData = dataStructure.getDataAs<AttributeMatrix>(exemplarCellDataPath);
  const auto exemplarFeatureData = dataStructure.getDataAs<AttributeMatrix>(exemplarCellFeatureDataPath);
  const auto exemplarSpacing = exemplarGeo->getSpacing();
  const auto exemplarDims = exemplarGeo->getDimensions();
  const auto exemplarCellDataDims = exemplarCellData->getShape();
  const auto exemplarFeatDataDims = exemplarFeatureData->getShape();
  REQUIRE(exemplarGeo->getOrigin() == destGeo->getOrigin());
  REQUIRE(std::fabs(destSpacing[0] - exemplarSpacing[0]) < UnitTest::EPSILON);
  REQUIRE(std::fabs(destSpacing[1] - exemplarSpacing[1]) < UnitTest::EPSILON);
  REQUIRE(std::fabs(destSpacing[2] - exemplarSpacing[2]) < UnitTest::EPSILON);
  REQUIRE(destDims[0] == exemplarDims[0]);
  REQUIRE(destDims[1] == exemplarDims[1]);
  REQUIRE(destDims[2] == exemplarDims[2]);
  REQUIRE(destCellDataDims[0] == exemplarCellDataDims[0]);
  REQUIRE(destCellDataDims[1] == exemplarCellDataDims[1]);
  REQUIRE(destCellDataDims[2] == exemplarCellDataDims[2]);
  REQUIRE(destFeatDataDims[0] == 214);
  REQUIRE(destNewGrainDataDims[0] == 5120);
  REQUIRE(destPhaseDataDims[0] == 2);
  REQUIRE(destCellData->getSize() == exemplarCellData->getSize());
  REQUIRE(destFeatureData->getSize() == exemplarFeatureData->getSize());
  REQUIRE(destNewGrainData->getSize() == 1);
  REQUIRE(destPhaseData->getSize() == 2);

  // check the data arrays
  const auto exemplarCellDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellDataPath).value();
  const auto calculatedCellDataArrays = GetAllChildArrayDataPaths(dataStructure, destCellDataPath).value();
  for(usize i = 0; i < exemplarCellDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarCellDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedCellDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  const auto exemplarFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellFeatureDataPath).value();
  const auto calculatedFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, destCellFeatureDataPath).value();
  for(usize i = 0; i < exemplarFeatureDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarFeatureDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedFeatureDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  const auto srcNewGrainDataArrays = GetAllChildArrayDataPaths(dataStructure, srcGeomPath.createChildPath(destNewGrainDataPath.getTargetName())).value();
  const auto calculatedNewGrainDataArrays = GetAllChildArrayDataPaths(dataStructure, destNewGrainDataPath).value();
  for(usize i = 0; i < srcNewGrainDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(srcNewGrainDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedNewGrainDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  const auto srcPhaseDataArrays = GetAllChildArrayDataPaths(dataStructure, srcGeomPath.createChildPath(destPhaseDataPath.getTargetName())).value();
  const auto calculatedPhaseDataArrays = GetAllChildArrayDataPaths(dataStructure, destPhaseDataPath).value();
  for(usize i = 0; i < srcPhaseDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(srcPhaseDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedPhaseDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }
}

TEST_CASE("ComplexCore::ResampleImageGeom: 3D Save Geometry", "[ComplexCore][ResampleImageGeom]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "ResampleImageGeom_Exemplar.tar.gz",
                                                             "ResampleImageGeom_Exemplar.dream3d");

  ResampleImageGeomFilter filter;
  Arguments args;

  DataStructure dataStructure = LoadDataStructure(fs::path(fmt::format("{}/ResampleImageGeom_Exemplar.dream3d", unit_test::k_TestFilesDir)));
  DataPath srcGeomPath({Constants::k_SmallIN100});
  DataPath cellDataPath = srcGeomPath.createChildPath(Constants::k_EbsdScanData);
  DataPath cellFeatureDataPath = srcGeomPath.createChildPath(Constants::k_FeatureGroupName);
  DataPath featureIdsPath = cellDataPath.createChildPath(Constants::k_FeatureIds);
  DataPath destGeomPath({Constants::k_SmallIN100.str() + "_RESAMPLED"});
  DataPath destCellDataPath = destGeomPath.createChildPath(Constants::k_EbsdScanData);
  DataPath destCellFeatureDataPath = destGeomPath.createChildPath(Constants::k_FeatureGroupName);
  DataPath destNewGrainDataPath = destGeomPath.createChildPath("NewGrain Data");
  DataPath destPhaseDataPath = destGeomPath.createChildPath("Phase Data");

  // Create default Parameters for the filter.
  DataPath exemplarGeoPath;
  SECTION("Spacing")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_SpacingModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{1, 1, 1}));
    exemplarGeoPath = DataPath({"Resampled_3D_ImageGeom"});
  }
  SECTION("Scaling")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_ScalingModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_Scaling_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{25, 25, 25}));
    exemplarGeoPath = DataPath({"Resampled_3D_ImageGeom"});
  }
  SECTION("Exact Dimensions")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_ExactDimensionsModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_ExactDimensions_Key, std::make_any<VectorUInt64Parameter::ValueType>(std::vector<uint64>{47, 50, 1}));
    exemplarGeoPath = DataPath({"Resampled_3D_ImageGeom_ExactDimensions"});
  }
  args.insertOrAssign(ResampleImageGeomFilter::k_RenumberFeatures_Key, std::make_any<bool>(true));
  args.insertOrAssign(ResampleImageGeomFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insertOrAssign(ResampleImageGeomFilter::k_SelectedImageGeometry_Key, std::make_any<DataPath>(srcGeomPath));
  args.insertOrAssign(ResampleImageGeomFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsPath));
  args.insertOrAssign(ResampleImageGeomFilter::k_FeatureAttributeMatrix_Key, std::make_any<DataPath>(cellFeatureDataPath));
  args.insertOrAssign(ResampleImageGeomFilter::k_CreatedImageGeometry_Key, std::make_any<DataPath>(destGeomPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)

  DataPath exemplarCellDataPath = exemplarGeoPath.createChildPath(Constants::k_EbsdScanData);
  DataPath exemplarCellFeatureDataPath = exemplarGeoPath.createChildPath(Constants::k_FeatureGroupName);

  // check that the geometry(s) and the attribute matrix are all there
  const auto srcGeo = dataStructure.getDataAs<ImageGeom>(srcGeomPath);
  const auto destGeo = dataStructure.getDataAs<ImageGeom>(destGeomPath);
  const auto destCellData = dataStructure.getDataAs<AttributeMatrix>(destCellDataPath);
  const auto destFeatureData = dataStructure.getDataAs<AttributeMatrix>(destCellFeatureDataPath);
  const auto destNewGrainData = dataStructure.getDataAs<AttributeMatrix>(destNewGrainDataPath);
  const auto destPhaseData = dataStructure.getDataAs<AttributeMatrix>(destPhaseDataPath);
  const auto exemplarGeo = dataStructure.getDataAs<ImageGeom>(exemplarGeoPath);
  const auto exemplarCellData = dataStructure.getDataAs<AttributeMatrix>(exemplarCellDataPath);
  const auto exemplarFeatureData = dataStructure.getDataAs<AttributeMatrix>(exemplarCellFeatureDataPath);
  REQUIRE(srcGeo != nullptr);
  REQUIRE(destGeo != nullptr);
  REQUIRE(destCellData != nullptr);
  REQUIRE(destFeatureData != nullptr);
  REQUIRE(destNewGrainData != nullptr);
  REQUIRE(destPhaseData != nullptr);

  // check the resampled geometry dimensions
  const auto destSpacing = destGeo->getSpacing();
  const auto destDims = destGeo->getDimensions();
  const auto destCellDataDims = destCellData->getShape();
  const auto destFeatDataDims = destFeatureData->getShape();
  const auto destNewGrainDataDims = destNewGrainData->getShape();
  const auto destPhaseDataDims = destPhaseData->getShape();

  const auto exemplarSpacing = exemplarGeo->getSpacing();
  const auto exemplarDims = exemplarGeo->getDimensions();
  const auto exemplarCellDataDims = exemplarCellData->getShape();
  const auto exemplarFeatDataDims = exemplarFeatureData->getShape();
  REQUIRE(exemplarGeo->getOrigin() == destGeo->getOrigin());
  REQUIRE(std::fabs(destSpacing[0] - exemplarSpacing[0]) < UnitTest::EPSILON);
  REQUIRE(std::fabs(destSpacing[1] - exemplarSpacing[1]) < UnitTest::EPSILON);
  REQUIRE(std::fabs(destSpacing[2] - exemplarSpacing[2]) < UnitTest::EPSILON);
  REQUIRE(destDims[0] == exemplarDims[0]);
  REQUIRE(destDims[1] == exemplarDims[1]);
  REQUIRE(destDims[2] == exemplarDims[2]);
  REQUIRE(destCellDataDims[0] == exemplarCellDataDims[0]);
  REQUIRE(destCellDataDims[1] == exemplarCellDataDims[1]);
  REQUIRE(destCellDataDims[2] == exemplarCellDataDims[2]);
  REQUIRE(destFeatDataDims[0] == 214);
  REQUIRE(destNewGrainDataDims[0] == 5120);
  REQUIRE(destPhaseDataDims[0] == 2);
  REQUIRE(destCellData->getSize() == exemplarCellData->getSize());
  REQUIRE(destFeatureData->getSize() == exemplarFeatureData->getSize());
  REQUIRE(destNewGrainData->getSize() == 1);
  REQUIRE(destPhaseData->getSize() == 2);

  // check the data arrays
  const auto exemplarCellDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellDataPath).value();
  const auto calculatedCellDataArrays = GetAllChildArrayDataPaths(dataStructure, destCellDataPath).value();
  for(usize i = 0; i < exemplarCellDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarCellDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedCellDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  const auto exemplarFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellFeatureDataPath).value();
  const auto calculatedFeatureDataArrays = GetAllChildArrayDataPaths(dataStructure, destCellFeatureDataPath).value();
  for(usize i = 0; i < exemplarFeatureDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarFeatureDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedFeatureDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  const auto srcNewGrainDataArrays = GetAllChildArrayDataPaths(dataStructure, srcGeomPath.createChildPath(destNewGrainDataPath.getTargetName())).value();
  const auto calculatedNewGrainDataArrays = GetAllChildArrayDataPaths(dataStructure, destNewGrainDataPath).value();
  for(usize i = 0; i < srcNewGrainDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(srcNewGrainDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedNewGrainDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }

  const auto srcPhaseDataArrays = GetAllChildArrayDataPaths(dataStructure, srcGeomPath.createChildPath(destPhaseDataPath.getTargetName())).value();
  const auto calculatedPhaseDataArrays = GetAllChildArrayDataPaths(dataStructure, destPhaseDataPath).value();
  for(usize i = 0; i < srcPhaseDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(srcPhaseDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedPhaseDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }
}

TEST_CASE("ComplexCore::ResampleImageGeom: 2D In Place", "[ComplexCore][ResampleImageGeom]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "ResampleImageGeom_Exemplar.tar.gz",
                                                             "ResampleImageGeom_Exemplar.dream3d");

  ResampleImageGeomFilter filter;
  Arguments args;

  DataStructure dataStructure = LoadDataStructure(fs::path(fmt::format("{}/ResampleImageGeom_Exemplar.dream3d", unit_test::k_TestFilesDir)));
  DataPath srcGeomPath({"Image2dDataContainer"});
  DataPath cellDataPath = srcGeomPath.createChildPath("Cell Data");
  DataPath destGeomPath({srcGeomPath.getTargetName()});
  DataPath destCellDataPath = destGeomPath.createChildPath(cellDataPath.getTargetName());

  // Create default Parameters for the filter.
  DataPath exemplarGeoPath;
  SECTION("Spacing")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_SpacingModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{1, 1, 1}));
    exemplarGeoPath = DataPath({"Resampled_2D_ImageGeom"});
  }
  SECTION("Scaling")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_ScalingModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_Scaling_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{150, 150, 50}));
    exemplarGeoPath = DataPath({"Resampled_2D_ImageGeom"});
  }
  SECTION("Exact Dimensions")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_ExactDimensionsModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_ExactDimensions_Key, std::make_any<VectorUInt64Parameter::ValueType>(std::vector<uint64>{15, 15, 1}));
    exemplarGeoPath = DataPath({"Resampled_2D_ImageGeom_ExactDimensions"});
  }
  args.insertOrAssign(ResampleImageGeomFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
  args.insertOrAssign(ResampleImageGeomFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(true));
  args.insertOrAssign(ResampleImageGeomFilter::k_SelectedImageGeometry_Key, std::make_any<DataPath>(srcGeomPath));
  args.insertOrAssign(ResampleImageGeomFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(ResampleImageGeomFilter::k_FeatureAttributeMatrix_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(ResampleImageGeomFilter::k_CreatedImageGeometry_Key, std::make_any<DataPath>(destGeomPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)

  DataPath exemplarCellDataPath = exemplarGeoPath.createChildPath(cellDataPath.getTargetName());

  // check that the geometry(s) and the attribute matrices are all there
  const auto srcGeo = dataStructure.getDataAs<ImageGeom>(srcGeomPath);
  const auto destGeo = dataStructure.getDataAs<ImageGeom>(destGeomPath);
  const auto destCellData = dataStructure.getDataAs<AttributeMatrix>(destCellDataPath);
  const auto exemplarGeo = dataStructure.getDataAs<ImageGeom>(exemplarGeoPath);
  const auto exemplarCellData = dataStructure.getDataAs<AttributeMatrix>(exemplarCellDataPath);
  REQUIRE(srcGeo != nullptr);
  REQUIRE(destGeo != nullptr);
  REQUIRE(destCellData != nullptr);

  // check the resampled geometry dimensions
  const auto destSpacing = destGeo->getSpacing();
  const auto destDims = destGeo->getDimensions();
  const auto destCellDataDims = destCellData->getShape();

  const auto exemplarSpacing = exemplarGeo->getSpacing();
  const auto exemplarDims = exemplarGeo->getDimensions();
  const auto exemplarCellDataDims = exemplarCellData->getShape();
  REQUIRE(exemplarGeo->getOrigin() == destGeo->getOrigin());
  REQUIRE(std::fabs(destSpacing[0] - exemplarSpacing[0]) < UnitTest::EPSILON);
  REQUIRE(std::fabs(destSpacing[1] - exemplarSpacing[1]) < UnitTest::EPSILON);
  REQUIRE(std::fabs(destSpacing[2] - exemplarSpacing[2]) < UnitTest::EPSILON);
  REQUIRE(destDims[0] == exemplarDims[0]);
  REQUIRE(destDims[1] == exemplarDims[1]);
  REQUIRE(destDims[2] == exemplarDims[2]);
  REQUIRE(destCellDataDims[0] == exemplarCellDataDims[0]);
  REQUIRE(destCellDataDims[1] == exemplarCellDataDims[1]);
  REQUIRE(destCellDataDims[2] == exemplarCellDataDims[2]);
  REQUIRE(destCellData->getSize() == exemplarCellData->getSize());

  // check the data arrays
  const auto exemplarCellDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellDataPath).value();
  const auto calculatedCellDataArrays = GetAllChildArrayDataPaths(dataStructure, destCellDataPath).value();
  for(usize i = 0; i < exemplarCellDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarCellDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedCellDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }
}

TEST_CASE("ComplexCore::ResampleImageGeom: 2D Save Geometry", "[ComplexCore][ResampleImageGeom]")
{
  const complex::UnitTest::TestFileSentinel testDataSentinel(complex::unit_test::k_CMakeExecutable, complex::unit_test::k_TestFilesDir, "ResampleImageGeom_Exemplar.tar.gz",
                                                             "ResampleImageGeom_Exemplar.dream3d");

  ResampleImageGeomFilter filter;
  Arguments args;

  DataStructure dataStructure = LoadDataStructure(fs::path(fmt::format("{}/ResampleImageGeom_Exemplar_test.dream3d", unit_test::k_TestFilesDir)));
  DataPath srcGeomPath({"Image2dDataContainer"});
  DataPath cellDataPath = srcGeomPath.createChildPath("Cell Data");
  DataPath destGeomPath({srcGeomPath.getTargetName() + "_RESAMPLED"});
  DataPath destCellDataPath = destGeomPath.createChildPath(cellDataPath.getTargetName());
  DataPath exemplarGeoPath;

  // Create default Parameters for the filter.
  SECTION("Spacing")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_SpacingModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{1, 1, 1}));
    exemplarGeoPath = DataPath({"Resampled_2D_ImageGeom"});
  }
  SECTION("Scaling")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_ScalingModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_Scaling_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{150, 150, 50}));
    exemplarGeoPath = DataPath({"Resampled_2D_ImageGeom"});
  }
  SECTION("Exact Dimensions")
  {
    args.insertOrAssign(ResampleImageGeomFilter::k_ResamplingMode_Key, std::make_any<ChoicesParameter::ValueType>(k_ExactDimensionsModeIndex));
    args.insertOrAssign(ResampleImageGeomFilter::k_ExactDimensions_Key, std::make_any<VectorUInt64Parameter::ValueType>(std::vector<uint64>{15, 15, 1}));
    exemplarGeoPath = DataPath({"Resampled_2D_ImageGeom_ExactDimensions"});
  }
  args.insertOrAssign(ResampleImageGeomFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
  args.insertOrAssign(ResampleImageGeomFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
  args.insertOrAssign(ResampleImageGeomFilter::k_SelectedImageGeometry_Key, std::make_any<DataPath>(srcGeomPath));
  args.insertOrAssign(ResampleImageGeomFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(ResampleImageGeomFilter::k_FeatureAttributeMatrix_Key, std::make_any<DataPath>(DataPath{}));
  args.insertOrAssign(ResampleImageGeomFilter::k_CreatedImageGeometry_Key, std::make_any<DataPath>(destGeomPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)

  // check that the geometry(s) and the attribute matrices are all there
  DataPath exemplarCellDataPath = exemplarGeoPath.createChildPath(cellDataPath.getTargetName());
  const auto srcGeo = dataStructure.getDataAs<ImageGeom>(srcGeomPath);
  const auto destGeo = dataStructure.getDataAs<ImageGeom>(destGeomPath);
  const auto destCellData = dataStructure.getDataAs<AttributeMatrix>(destCellDataPath);
  const auto exemplarGeo = dataStructure.getDataAs<ImageGeom>(exemplarGeoPath);
  const auto exemplarCellData = dataStructure.getDataAs<AttributeMatrix>(exemplarCellDataPath);
  REQUIRE(srcGeo != nullptr);
  REQUIRE(destGeo != nullptr);
  REQUIRE(destCellData != nullptr);

  // check the resampled geometry dimensions
  const auto destSpacing = destGeo->getSpacing();
  const auto destDims = destGeo->getDimensions();
  const auto destCellDataDims = destCellData->getShape();

  const auto exemplarSpacing = exemplarGeo->getSpacing();
  const auto exemplarDims = exemplarGeo->getDimensions();
  const auto exemplarCellDataDims = exemplarCellData->getShape();
  REQUIRE(exemplarGeo->getOrigin() == destGeo->getOrigin());
  REQUIRE(std::fabs(destSpacing[0] - exemplarSpacing[0]) < UnitTest::EPSILON);
  REQUIRE(std::fabs(destSpacing[1] - exemplarSpacing[1]) < UnitTest::EPSILON);
  REQUIRE(std::fabs(destSpacing[2] - exemplarSpacing[2]) < UnitTest::EPSILON);
  REQUIRE(destDims[0] == exemplarDims[0]);
  REQUIRE(destDims[1] == exemplarDims[1]);
  REQUIRE(destDims[2] == exemplarDims[2]);
  REQUIRE(destCellDataDims[0] == exemplarCellDataDims[0]);
  REQUIRE(destCellDataDims[1] == exemplarCellDataDims[1]);
  REQUIRE(destCellDataDims[2] == exemplarCellDataDims[2]);
  REQUIRE(destCellData->getSize() == exemplarCellData->getSize());

  // check the data arrays
  const auto exemplarCellDataArrays = GetAllChildArrayDataPaths(dataStructure, exemplarCellDataPath).value();
  const auto calculatedCellDataArrays = GetAllChildArrayDataPaths(dataStructure, destCellDataPath).value();
  for(usize i = 0; i < exemplarCellDataArrays.size(); ++i)
  {
    const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(exemplarCellDataArrays[i]);
    const IDataArray& calculatedArray = dataStructure.getDataRefAs<IDataArray>(calculatedCellDataArrays[i]);
    ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
  }
}
