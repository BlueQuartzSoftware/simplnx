
#include "SimplnxCore/Filters/IdentifySampleFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

#include <catch2/catch.hpp>

#include <cmath>

using namespace nx::core;
using namespace nx::core::UnitTest;

namespace
{
const DataPath k_ExemplarArrayPath = Constants::k_DataContainerPath.createChildPath(Constants::k_CellData).createChildPath("Mask Exemplar");
}
TEST_CASE("SimplnxCore::IdentifySampleFilter", "[SimplnxCore][IdentifySampleFilter]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = GENERATE(false, true);
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  // 25x25x25 dataset, Mask (uint8, 1-comp) => 25*25*1 = 625 bytes/slice
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 625, true);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, "identify_sample_v2.tar.gz", "identify_sample_v2");
  using TestArgType = std::tuple<std::string, std::string, std::string>;
  /* clang-format off */
  std::vector<TestArgType> allTestParams = {
    {"sliced", "xy", "fill"},
    {"sliced", "xy", "nofill"},
    {"sliced", "xz", "fill"},
    {"sliced", "xz", "nofill"},
    {"sliced", "yz", "fill"},
    {"sliced", "yz", "nofill"},

    {"whole", "xy", "fill"},
    {"whole", "xy", "nofill"},
    {"whole", "xz", "fill"},
    {"whole", "xz", "nofill"},
    {"whole", "yz", "fill"},
    {"whole", "yz", "nofill"},
  };
  /* clang-format on */
  for(const auto& testParam : allTestParams)
  {
    std::string slice_by_slice = std::get<0>(testParam);
    bool sliceBySlice = slice_by_slice == "sliced";

    std::string slice_plane = std::get<1>(testParam);

    ChoicesParameter::ValueType sliceBySlicePlane = 0;
    if(slice_plane == "xz")
      sliceBySlicePlane = 1;
    else if(slice_plane == "yz")
      sliceBySlicePlane = 2;

    std::string fill_holes = std::get<2>(testParam);
    bool fillHoles = fill_holes == "fill";

    SECTION(fmt::format("{}_{}_{}", slice_by_slice, slice_plane, fill_holes))
    {
      fs::path inputFilePath = fs::path(fmt::format("{}/identify_sample_v2/{}_{}_{}.dream3d", unit_test::k_TestFilesDir, slice_by_slice, slice_plane, fill_holes));
      std::cout << inputFilePath.string() << std::endl;

      DataStructure dataStructure = LoadDataStructure(inputFilePath);
      IdentifySampleFilter filter;
      Arguments args;
      args.insert(IdentifySampleFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(Constants::k_DataContainerPath));
      args.insert(IdentifySampleFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(Constants::k_MaskArrayPath));
      args.insert(IdentifySampleFilter::k_FillHoles_Key, std::make_any<bool>(fillHoles));
      args.insert(IdentifySampleFilter::k_SliceBySlice_Key, std::make_any<bool>(sliceBySlice));
      args.insert(IdentifySampleFilter::k_SliceBySlicePlane_Key, std::make_any<ChoicesParameter::ValueType>(sliceBySlicePlane));

      // Preflight the filter and check result
      auto preflightResult = filter.preflight(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

      // Execute the filter and check the result
      auto executeResult = filter.execute(dataStructure, args);
      SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

#ifdef SIMPLNX_WRITE_TEST_OUTPUT
      WriteTestDataStructure(dataStructure, fmt::format("{}/identify_sample_output_{}_{}_{}.dream3d", unit_test::k_BinaryTestOutputDir, fillHoles, sliceBySlice, sliceBySlicePlane));
#endif

      const IDataArray& computedArray = dataStructure.getDataRefAs<IDataArray>(Constants::k_MaskArrayPath);
      const IDataArray& exemplarArray = dataStructure.getDataRefAs<IDataArray>(k_ExemplarArrayPath);
      CompareDataArrays<uint8>(computedArray, exemplarArray);

      UnitTest::CheckArraysInheritTupleDims(dataStructure);
    }
  }
}

TEST_CASE("SimplnxCore::IdentifySampleFilter: Benchmark 200x200x200", "[SimplnxCore][IdentifySampleFilter][Benchmark]")
{
  UnitTest::LoadPlugins();
  bool forceOocAlgo = GENERATE(false, true);
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  // 200*200 * 1 byte = 40000 bytes per Z-slice for uint8 mask
  const UnitTest::PreferencesSentinel prefsSentinel("Zarr", 40000, true);

  constexpr usize kDimX = 200;
  constexpr usize kDimY = 200;
  constexpr usize kDimZ = 200;
  constexpr usize kTotalVoxels = kDimX * kDimY * kDimZ;
  const ShapeType cellTupleShape = {kDimZ, kDimY, kDimX};
  const auto benchmarkFile = fs::path(fmt::format("{}/identify_sample_benchmark.dream3d", unit_test::k_BinaryTestOutputDir));

  // Stage 1: Build data programmatically and write to .dream3d
  {
    DataStructure buildDS;
    auto* imageGeom = ImageGeom::Create(buildDS, "DataContainer");
    imageGeom->setDimensions({kDimX, kDimY, kDimZ});
    imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
    imageGeom->setOrigin({0.0f, 0.0f, 0.0f});

    auto* cellAM = AttributeMatrix::Create(buildDS, "Cell Data", cellTupleShape, imageGeom->getId());
    imageGeom->setCellData(*cellAM);

    // Create mask array: a sphere of "good" voxels with interior holes and exterior noise
    auto* maskArray = CreateTestDataArray<uint8>(buildDS, "Mask", cellTupleShape, {1}, cellAM->getId());
    auto& maskStore = maskArray->getDataStoreRef();

    const float cx = kDimX / 2.0f;
    const float cy = kDimY / 2.0f;
    const float cz = kDimZ / 2.0f;
    const float radius = 80.0f;

    for(usize z = 0; z < kDimZ; z++)
    {
      for(usize y = 0; y < kDimY; y++)
      {
        for(usize x = 0; x < kDimX; x++)
        {
          const usize idx = z * kDimX * kDimY + y * kDimX + x;
          const float dx = static_cast<float>(x) - cx;
          const float dy = static_cast<float>(y) - cy;
          const float dz = static_cast<float>(z) - cz;
          const float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
          bool good = dist < radius;

          // Create interior holes (small sphere cavities)
          if(good)
          {
            const float h1 = std::sqrt((static_cast<float>(x) - 120.0f) * (static_cast<float>(x) - 120.0f) + (static_cast<float>(y) - 120.0f) * (static_cast<float>(y) - 120.0f) +
                                       (static_cast<float>(z) - 120.0f) * (static_cast<float>(z) - 120.0f));
            if(h1 < 10.0f)
            {
              good = false;
            }
            const float h2 = std::sqrt((static_cast<float>(x) - 80.0f) * (static_cast<float>(x) - 80.0f) + (static_cast<float>(y) - 80.0f) * (static_cast<float>(y) - 80.0f) +
                                       (static_cast<float>(z) - 80.0f) * (static_cast<float>(z) - 80.0f));
            if(h2 < 8.0f)
            {
              good = false;
            }
          }

          // Add some isolated small clusters outside the main sphere
          if(!good && dist < radius + 5.0f && dist > radius)
          {
            if((x + y + z) % 7 == 0)
            {
              good = true;
            }
          }

          maskStore[idx] = good ? 1 : 0;
        }
      }
    }

    UnitTest::WriteTestDataStructure(buildDS, benchmarkFile);
  }

  // Stage 2: Reload (arrays become ZarrStore in OOC) and run filter
  DataStructure dataStructure = UnitTest::LoadDataStructure(benchmarkFile);

  {
    IdentifySampleFilter filter;
    Arguments args;
    args.insert(IdentifySampleFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({"DataContainer"})));
    args.insert(IdentifySampleFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(DataPath({"DataContainer", "Cell Data", "Mask"})));
    args.insert(IdentifySampleFilter::k_FillHoles_Key, std::make_any<bool>(true));
    args.insert(IdentifySampleFilter::k_SliceBySlice_Key, std::make_any<bool>(false));
    args.insert(IdentifySampleFilter::k_SliceBySlicePlane_Key, std::make_any<ChoicesParameter::ValueType>(0));

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);
  }

  fs::remove(benchmarkFile);
}
