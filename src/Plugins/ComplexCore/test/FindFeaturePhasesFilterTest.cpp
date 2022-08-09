#include <catch2/catch.hpp>

#include "ComplexCore/Filters/FindFeaturePhasesFilter.hpp"
#include "ComplexCore/Filters/RawBinaryReaderFilter.hpp"
#include "complex/Parameters/DynamicTableParameter.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"
#include "complex/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "complex/Utilities/Parsing/HDF5/H5FileWriter.hpp"

#include "ComplexCore/ComplexCore_test_dirs.hpp"

using namespace complex;
namespace fs = std::filesystem;

namespace
{

inline DataStructure LoadDataStructure(const fs::path& filepath)
{
  DataStructure exemplarDataStructure;
  REQUIRE(fs::exists(filepath));
  auto result = DREAM3D::ImportDataStructureFromFile(filepath);
  REQUIRE(result.valid());
  return result.value();
}

} // namespace

TEST_CASE("ComplexCore::FindFeaturePhasesFilter(Valid Parameters)", "[ComplexCore][FindFeaturePhasesFilter]")
{

  // Read the Small IN100 Data set
  auto baseDataFilePath = fs::path(fmt::format("{}/6_5_test_data_1.dream3d", unit_test::k_TestDataSourceDir));
  DataStructure dataStructure = LoadDataStructure(baseDataFilePath);
  DataPath smallIn100Group({complex::Constants::k_DataContainer});
  DataPath cellDataPath = smallIn100Group.createChildPath(complex::Constants::k_CellData);
  DataPath cellPhasesPath = cellDataPath.createChildPath(complex::Constants::k_Phases);
  DataPath featureIdsPath = cellDataPath.createChildPath(complex::Constants::k_FeatureIds);
  DataPath featurePhasesPath({complex::Constants::k_Phases});

  {
    FindFeaturePhasesFilter ffpFilter;
    Arguments args;
    args.insert(FindFeaturePhasesFilter::k_CellPhasesArrayPath_Key, std::make_any<DataPath>(cellPhasesPath));
    args.insert(FindFeaturePhasesFilter::k_FeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsPath));
    args.insert(FindFeaturePhasesFilter::k_FeaturePhasesArrayPath_Key, std::make_any<DataPath>(featurePhasesPath));

    auto preflightResult = ffpFilter.preflight(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

    auto result = ffpFilter.execute(dataStructure, args);
    COMPLEX_RESULT_REQUIRE_VALID(result.result);

    DataPath exemplaryDataPath = smallIn100Group.createChildPath("CellFeatureData").createChildPath(complex::Constants::k_Phases);
    const Int32Array& featureArrayExemplary = dataStructure.getDataRefAs<Int32Array>(exemplaryDataPath);

    const Int32Array& createdFeatureArray = dataStructure.getDataRefAs<Int32Array>(featurePhasesPath);
    REQUIRE(createdFeatureArray.getNumberOfTuples() == featureArrayExemplary.getNumberOfTuples());

    for(usize i = 0; i < featureArrayExemplary.getSize(); i++)
    {
      REQUIRE(featureArrayExemplary[i] == createdFeatureArray[i]);
    }
  }

  {
    // Write out the DataStructure for later viewing/debugging
    Result<H5::FileWriter> result = H5::FileWriter::CreateFile(fmt::format("{}/find_feature_phases_filter.dream3d", unit_test::k_BinaryDir));
    H5::FileWriter fileWriter = std::move(result.value());
    herr_t err = dataStructure.writeHdf5(fileWriter);
    REQUIRE(err >= 0);
  }
}
