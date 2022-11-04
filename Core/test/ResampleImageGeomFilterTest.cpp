/**
 * This file is auto generated from the original Sampling/ResampleImageGeom
 * runtime information. These are the steps that need to be taken to utilize this
 * unit test in the proper way.
 *
 * 1: Validate each of the default parameters that gets created.
 * 2: Inspect the actual filter to determine if the filter in its default state
 * would pass or fail BOTH the preflight() and execute() methods
 * 3: UPDATE the ```REQUIRE(result.result.valid());``` code to have the proper
 *
 * 4: Add additional unit tests to actually test each code path within the filter
 *
 * There are some example Catch2 ```TEST_CASE``` sections for your inspiration.
 *
 * NOTE the format of the ```TEST_CASE``` macro. Please stick to this format to
 * allow easier parsing of the unit tests.
 *
 * When you start working on this unit test remove "[ResampleImageGeom][.][UNIMPLEMENTED]"
 * from the TEST_CASE macro. This will enable this unit test to be run by default
 * and report errors.
 */

#include <catch2/catch.hpp>

#include "complex/Parameters/VectorParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/DataGroupUtilities.hpp"
#include "complex/Utilities/FilterUtilities.hpp"

#include "Core/Core_test_dirs.hpp"
#include "Core/Filters/ResampleImageGeomFilter.hpp"
#include "complex_plugins/Utilities/TestUtilities.hpp"

#include <filesystem>

namespace fs = std::filesystem;
using namespace complex;

struct CompareDataArrayFunctor
{
  template <typename T>
  void operator()(const IDataArray& left, const IDataArray& right, usize start = 0)
  {
    UnitTest::CompareDataArrays<T>(left, right, start);
  }
};

TEST_CASE("Core::ResampleImageGeom: Instantiation, Parameter Check and valid filter execution", "[Core][ResampleImageGeom]")
{
  // 3D image geom
  {
    ResampleImageGeomFilter filter;
    Arguments args;

    DataStructure ds = LoadDataStructure(fs::path(fmt::format("{}/TestFiles/ResampleImageGeom_Exemplar.dream3d", unit_test::k_DREAM3DDataDir)));
    DataPath srcGeomPath({Constants::k_SmallIN100});
    DataPath cellDataPath = srcGeomPath.createChildPath(Constants::k_EbsdScanData);
    DataPath cellFeatureDataPath = srcGeomPath.createChildPath(Constants::k_FeatureGroupName);
    DataPath featureIdsPath = cellDataPath.createChildPath(Constants::k_FeatureIds);
    DataPath destGeomPath({Constants::k_SmallIN100.str() + "_RESAMPLED"});
    DataPath destCellDataPath = destGeomPath.createChildPath(Constants::k_EbsdScanData);
    DataPath destCellFeatureDataPath = destGeomPath.createChildPath(Constants::k_FeatureGroupName);
    DataPath destNewGrainDataPath = destGeomPath.createChildPath("NewGrain Data");
    DataPath destPhaseDataPath = destGeomPath.createChildPath("Phase Data");
    DataPath exemplarGeoPath({"Resampled_3D_ImageGeom"});
    DataPath exemplarCellDataPath = exemplarGeoPath.createChildPath(Constants::k_EbsdScanData);
    DataPath exemplarCellFeatureDataPath = exemplarGeoPath.createChildPath(Constants::k_FeatureGroupName);

    // Create default Parameters for the filter.
    args.insertOrAssign(ResampleImageGeomFilter::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{1, 1, 1}));
    args.insertOrAssign(ResampleImageGeomFilter::k_RenumberFeatures_Key, std::make_any<bool>(true));
    args.insertOrAssign(ResampleImageGeomFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
    args.insertOrAssign(ResampleImageGeomFilter::k_SelectedImageGeometry_Key, std::make_any<DataPath>(srcGeomPath));
    args.insertOrAssign(ResampleImageGeomFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsPath));
    args.insertOrAssign(ResampleImageGeomFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(cellFeatureDataPath));
    args.insertOrAssign(ResampleImageGeomFilter::k_NewDataContainerPath_Key, std::make_any<DataPath>(destGeomPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(ds, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(ds, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)

    // check that the geometry(s) and the attribute matrixes are all there
    const auto srcGeo = ds.getDataAs<ImageGeom>(srcGeomPath);
    const auto destGeo = ds.getDataAs<ImageGeom>(destGeomPath);
    const auto destCellData = ds.getDataAs<AttributeMatrix>(destCellDataPath);
    const auto destFeatureData = ds.getDataAs<AttributeMatrix>(destCellFeatureDataPath);
    const auto destNewGrainData = ds.getDataAs<AttributeMatrix>(destNewGrainDataPath);
    const auto destPhaseData = ds.getDataAs<AttributeMatrix>(destPhaseDataPath);
    const auto exemplarGeo = ds.getDataAs<ImageGeom>(exemplarGeoPath);
    const auto exemplarCellData = ds.getDataAs<AttributeMatrix>(exemplarCellDataPath);
    const auto exemplarFeatureData = ds.getDataAs<AttributeMatrix>(exemplarCellFeatureDataPath);
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
    const auto exemplarCellDataArrays = GetAllChildArrayDataPaths(ds, exemplarCellDataPath).value();
    const auto calculatedCellDataArrays = GetAllChildArrayDataPaths(ds, destCellDataPath).value();
    for(usize i = 0; i < exemplarCellDataArrays.size(); ++i)
    {
      const IDataArray& exemplarArray = ds.getDataRefAs<IDataArray>(exemplarCellDataArrays[i]);
      const IDataArray& calculatedArray = ds.getDataRefAs<IDataArray>(calculatedCellDataArrays[i]);
      ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
    }

    const auto exemplarFeatureDataArrays = GetAllChildArrayDataPaths(ds, exemplarCellFeatureDataPath).value();
    const auto calculatedFeatureDataArrays = GetAllChildArrayDataPaths(ds, destCellFeatureDataPath).value();
    for(usize i = 0; i < exemplarFeatureDataArrays.size(); ++i)
    {
      const IDataArray& exemplarArray = ds.getDataRefAs<IDataArray>(exemplarFeatureDataArrays[i]);
      const IDataArray& calculatedArray = ds.getDataRefAs<IDataArray>(calculatedFeatureDataArrays[i]);
      ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
    }

    const auto srcNewGrainDataArrays = GetAllChildArrayDataPaths(ds, srcGeomPath.createChildPath(destNewGrainDataPath.getTargetName())).value();
    const auto calculatedNewGrainDataArrays = GetAllChildArrayDataPaths(ds, destNewGrainDataPath).value();
    for(usize i = 0; i < srcNewGrainDataArrays.size(); ++i)
    {
      const IDataArray& exemplarArray = ds.getDataRefAs<IDataArray>(srcNewGrainDataArrays[i]);
      const IDataArray& calculatedArray = ds.getDataRefAs<IDataArray>(calculatedNewGrainDataArrays[i]);
      ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
    }

    const auto srcPhaseDataArrays = GetAllChildArrayDataPaths(ds, srcGeomPath.createChildPath(destPhaseDataPath.getTargetName())).value();
    const auto calculatedPhaseDataArrays = GetAllChildArrayDataPaths(ds, destPhaseDataPath).value();
    for(usize i = 0; i < srcPhaseDataArrays.size(); ++i)
    {
      const IDataArray& exemplarArray = ds.getDataRefAs<IDataArray>(srcPhaseDataArrays[i]);
      const IDataArray& calculatedArray = ds.getDataRefAs<IDataArray>(calculatedPhaseDataArrays[i]);
      ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
    }
  }

  // 2D image geom
  {
    ResampleImageGeomFilter filter;
    Arguments args;

    DataStructure ds = LoadDataStructure(fs::path(fmt::format("{}/TestFiles/ResampleImageGeom_Exemplar.dream3d", unit_test::k_DREAM3DDataDir)));
    DataPath srcGeomPath({"Image2dDataContainer"});
    DataPath cellDataPath = srcGeomPath.createChildPath("Cell Data");
    DataPath destGeomPath({srcGeomPath.getTargetName() + "_RESAMPLED"});
    DataPath destCellDataPath = destGeomPath.createChildPath(cellDataPath.getTargetName());
    DataPath exemplarGeoPath({"Resampled_2D_ImageGeom"});
    DataPath exemplarCellDataPath = exemplarGeoPath.createChildPath(cellDataPath.getTargetName());

    // Create default Parameters for the filter.
    args.insertOrAssign(ResampleImageGeomFilter::k_Spacing_Key, std::make_any<VectorFloat32Parameter::ValueType>(std::vector<float32>{1, 1, 1}));
    args.insertOrAssign(ResampleImageGeomFilter::k_RenumberFeatures_Key, std::make_any<bool>(false));
    args.insertOrAssign(ResampleImageGeomFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(false));
    args.insertOrAssign(ResampleImageGeomFilter::k_SelectedImageGeometry_Key, std::make_any<DataPath>(srcGeomPath));
    args.insertOrAssign(ResampleImageGeomFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(ResampleImageGeomFilter::k_CellFeatureAttributeMatrixPath_Key, std::make_any<DataPath>(DataPath{}));
    args.insertOrAssign(ResampleImageGeomFilter::k_NewDataContainerPath_Key, std::make_any<DataPath>(destGeomPath));

    // Preflight the filter and check result
    auto preflightResult = filter.preflight(ds, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

    // Execute the filter and check the result
    auto executeResult = filter.execute(ds, args);
    COMPLEX_RESULT_REQUIRE_VALID(executeResult.result)

    // check that the geometry(s) and the attribute matrixes are all there
    const auto srcGeo = ds.getDataAs<ImageGeom>(srcGeomPath);
    const auto destGeo = ds.getDataAs<ImageGeom>(destGeomPath);
    const auto destCellData = ds.getDataAs<AttributeMatrix>(destCellDataPath);
    const auto exemplarGeo = ds.getDataAs<ImageGeom>(exemplarGeoPath);
    const auto exemplarCellData = ds.getDataAs<AttributeMatrix>(exemplarCellDataPath);
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
    const auto exemplarCellDataArrays = GetAllChildArrayDataPaths(ds, exemplarCellDataPath).value();
    const auto calculatedCellDataArrays = GetAllChildArrayDataPaths(ds, destCellDataPath).value();
    for(usize i = 0; i < exemplarCellDataArrays.size(); ++i)
    {
      const IDataArray& exemplarArray = ds.getDataRefAs<IDataArray>(exemplarCellDataArrays[i]);
      const IDataArray& calculatedArray = ds.getDataRefAs<IDataArray>(calculatedCellDataArrays[i]);
      ExecuteDataFunction(CompareDataArrayFunctor{}, exemplarArray.getDataType(), exemplarArray, calculatedArray);
    }
  }
}
