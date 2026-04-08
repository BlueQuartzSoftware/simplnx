
#include "SimplnxCore/Filters/IdentifySampleFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <catch2/catch.hpp>
#include <nonstd/span.hpp>

#include <cmath>

using namespace nx::core;
using namespace nx::core::UnitTest;

namespace
{
// Exemplar archive
const std::string k_ArchiveName = "identify_sample_exemplars.tar.gz";
const std::string k_DataDirName = "identify_sample_exemplars";
const fs::path k_DataDir = fs::path(unit_test::k_TestFilesDir.view()) / k_DataDirName;
const fs::path k_ExemplarFile = k_DataDir / "identify_sample.dream3d";

// Geometry names
constexpr StringLiteral k_GeomName = "DataContainer";
constexpr StringLiteral k_CellDataName = "CellData";

// Output array paths
const DataPath k_GeomPath({k_GeomName});
const DataPath k_MaskPath({k_GeomName, k_CellDataName, "Mask"});

// Test dimensions
constexpr usize k_Dim = 200;

/**
 * @brief Builds an IdentifySample test dataset: a sphere of "good" voxels
 * with interior holes and exterior noise.
 */
void BuildIdentifySampleTestData(DataStructure& ds, usize dimX, usize dimY, usize dimZ, const std::string& geomName = "DataContainer")
{
  const ShapeType cellShape = {dimZ, dimY, dimX};
  auto* imageGeom = ImageGeom::Create(ds, geomName);
  imageGeom->setDimensions({dimX, dimY, dimZ});
  imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});

  auto* cellAM = AttributeMatrix::Create(ds, "CellData", cellShape, imageGeom->getId());
  imageGeom->setCellData(*cellAM);

  auto maskDataStore = DataStoreUtilities::CreateDataStore<uint8>(cellShape, {1}, IDataAction::Mode::Execute);
  auto* maskArray = DataArray<uint8>::Create(ds, "Mask", maskDataStore, cellAM->getId());
  auto& maskStore = maskArray->getDataStoreRef();

  const float32 cx = dimX / 2.0f;
  const float32 cy = dimY / 2.0f;
  const float32 cz = dimZ / 2.0f;
  const float32 radius = dimX * 0.4f;

  const usize sliceSize = dimY * dimX;
  std::vector<uint8> sliceBuffer(sliceSize);

  for(usize z = 0; z < dimZ; z++)
  {
    for(usize y = 0; y < dimY; y++)
    {
      for(usize x = 0; x < dimX; x++)
      {
        const float32 dx = static_cast<float32>(x) - cx;
        const float32 dy = static_cast<float32>(y) - cy;
        const float32 dz = static_cast<float32>(z) - cz;
        const float32 dist = std::sqrt(dx * dx + dy * dy + dz * dz);
        bool good = dist < radius;

        // Interior holes (positions relative to geometry size so they work at any dim)
        const float32 h1cx = cx + radius * 0.3f;
        const float32 h1cy = cy + radius * 0.3f;
        const float32 h1cz = cz + radius * 0.3f;
        const float32 h1r = dimX * 0.053f; // ~4 at 75, ~10.6 at 200
        const float32 h2cx = cx - radius * 0.3f;
        const float32 h2cy = cy - radius * 0.3f;
        const float32 h2cz = cz - radius * 0.3f;
        const float32 h2r = dimX * 0.04f; // ~3 at 75, ~8 at 200

        if(good)
        {
          const float32 h1 = std::sqrt((static_cast<float32>(x) - h1cx) * (static_cast<float32>(x) - h1cx) + (static_cast<float32>(y) - h1cy) * (static_cast<float32>(y) - h1cy) +
                                     (static_cast<float32>(z) - h1cz) * (static_cast<float32>(z) - h1cz));
          if(h1 < h1r)
          {
            good = false;
          }
          const float32 h2 = std::sqrt((static_cast<float32>(x) - h2cx) * (static_cast<float32>(x) - h2cx) + (static_cast<float32>(y) - h2cy) * (static_cast<float32>(y) - h2cy) +
                                     (static_cast<float32>(z) - h2cz) * (static_cast<float32>(z) - h2cz));
          if(h2 < h2r)
          {
            good = false;
          }
        }

        // Isolated noise outside the sphere
        if(!good && dist < radius + 5.0f && dist > radius)
        {
          if((x + y + z) % 7 == 0)
          {
            good = true;
          }
        }

        sliceBuffer[y * dimX + x] = good ? 1 : 0;
      }
    }
    maskStore.copyFromBuffer(z * sliceSize, nonstd::span<const uint8>(sliceBuffer.data(), sliceSize));
  }
}

/**
 * @brief Populates IdentifySampleFilter arguments from a test variant name.
 *
 * Name convention: "whole_fill", "sliced_xy_nofill", etc.
 */
void SetupArgs(Arguments& args, const std::string& testName, const DataPath& geomPath, const DataPath& maskPath)
{
  const bool fillHoles = (testName.find("nofill") == std::string::npos);
  const bool sliceBySlice = (testName.find("sliced") != std::string::npos);
  ChoicesParameter::ValueType slicePlane = 0;
  if(testName.find("xz") != std::string::npos)
  {
    slicePlane = 1;
  }
  else if(testName.find("yz") != std::string::npos)
  {
    slicePlane = 2;
  }

  args.insertOrAssign(IdentifySampleFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(geomPath));
  args.insertOrAssign(IdentifySampleFilter::k_MaskArrayPath_Key, std::make_any<DataPath>(maskPath));
  args.insertOrAssign(IdentifySampleFilter::k_FillHoles_Key, std::make_any<bool>(fillHoles));
  args.insertOrAssign(IdentifySampleFilter::k_SliceBySlice_Key, std::make_any<bool>(sliceBySlice));
  args.insertOrAssign(IdentifySampleFilter::k_SliceBySlicePlane_Key, std::make_any<ChoicesParameter::ValueType>(slicePlane));
}
} // namespace

TEST_CASE("SimplnxCore::IdentifySampleFilter: 200x200x200 Exemplar Comparison", "[SimplnxCore][IdentifySampleFilter]")
{
  UnitTest::LoadPlugins();
  // Test both algorithm paths (in-core + OOC) by default; controlled by CMake SIMPLNX_TEST_ALGORITHM_PATH
  bool forceOocAlgo = static_cast<bool>(GENERATE(from_range(nx::core::k_ForceOocTestValues)));
  const nx::core::ForceOocAlgorithmGuard guard(forceOocAlgo);
  // uint8 1-comp => 200*200*1 = 40,000 bytes/slice
  const UnitTest::PreferencesSentinel prefsSentinel("HDF5-OOC", 40000, true);

  const nx::core::UnitTest::TestFileSentinel testDataSentinel(nx::core::unit_test::k_TestFilesDir, k_ArchiveName, k_DataDirName);
  DataStructure exemplarDS = UnitTest::LoadDataStructure(k_ExemplarFile);

  std::string testName = GENERATE("whole_fill", "whole_nofill", "sliced_xy_fill", "sliced_xy_nofill", "sliced_xz_fill", "sliced_xz_nofill", "sliced_yz_fill", "sliced_yz_nofill");
  DYNAMIC_SECTION("Variant: " << testName)
  {
    DataStructure dataStructure;
    BuildIdentifySampleTestData(dataStructure, k_Dim, k_Dim, k_Dim);

    UnitTest::RequireExpectedStoreType(dataStructure.getDataRefAs<IDataArray>(k_MaskPath));

    IdentifySampleFilter filter;
    Arguments args;
    SetupArgs(args, testName, k_GeomPath, k_MaskPath);

    auto preflightResult = filter.preflight(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);
    auto executeResult = filter.execute(dataStructure, args);
    SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

    // Compare against exemplar
    const std::string exemplarGeomName = testName + "_Exemplar";
    const DataPath exemplarMaskPath({exemplarGeomName, std::string(k_CellDataName), "Mask"});

    REQUIRE_NOTHROW(dataStructure.getDataRefAs<UInt8Array>(k_MaskPath));
    REQUIRE_NOTHROW(exemplarDS.getDataRefAs<UInt8Array>(exemplarMaskPath));
    CompareDataArrays<uint8>(exemplarDS.getDataRefAs<UInt8Array>(exemplarMaskPath), dataStructure.getDataRefAs<UInt8Array>(k_MaskPath));

    UnitTest::CheckArraysInheritTupleDims(dataStructure);
  }
}

TEST_CASE("SimplnxCore::IdentifySampleFilter: Generate Test Data", "[SimplnxCore][IdentifySampleFilter][.GenerateTestData]")
{
  UnitTest::LoadPlugins();

  const auto outputDir = fs::path(fmt::format("{}/generated_test_data/identify_sample", unit_test::k_BinaryTestOutputDir));
  fs::create_directories(outputDir);

  DataStructure ds;
  for(const auto& name : {"whole_fill", "whole_nofill", "sliced_xy_fill", "sliced_xy_nofill", "sliced_xz_fill", "sliced_xz_nofill", "sliced_yz_fill", "sliced_yz_nofill"})
  {
    BuildIdentifySampleTestData(ds, k_Dim, k_Dim, k_Dim, name);
  }
  UnitTest::WriteTestDataStructure(ds, outputDir / "input.dream3d");
}
