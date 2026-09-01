#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/ErodeDilateBadDataFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <filesystem>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

namespace
{
constexpr ChoicesParameter::ValueType k_Dilate = 0ULL;
constexpr ChoicesParameter::ValueType k_Erode = 1ULL;

const std::string k_GeomName("ImageGeom");
const std::string k_CellDataName("CellData");

const DataPath k_GeomPath({k_GeomName});
const DataPath k_CellDataPath = k_GeomPath.createChildPath(k_CellDataName);
const DataPath k_FeatureIdsPath = k_CellDataPath.createChildPath("FeatureIds");

void BuildTestData(DataStructure& dataStructure, usize dimX, usize dimY, usize dimZ, usize blockSize)
{
  const ShapeType cellTupleShape = {dimZ, dimY, dimX};
  const usize sliceSize = dimX * dimY;

  auto* imageGeom = ImageGeom::Create(dataStructure, k_GeomName);
  imageGeom->setDimensions({dimX, dimY, dimZ});
  imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});

  auto* cellAM = AttributeMatrix::Create(dataStructure, k_CellDataName, cellTupleShape, imageGeom->getId());
  imageGeom->setCellData(*cellAM);

  auto featureIdsDataStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, k_FeatureIdsPath, cellTupleShape, {1}, IDataAction::Mode::Execute);
  auto* featureIdsArray = DataArray<int32>::Create(dataStructure, "FeatureIds", featureIdsDataStore, cellAM->getId());
  auto& featureIdsStore = featureIdsArray->getDataStoreRef();

  auto eulerDataStore = DataStoreUtilities::CreateDataStore<float32>(dataStructure, k_CellDataPath.createChildPath("EulerAngles"), cellTupleShape, {3}, IDataAction::Mode::Execute);
  auto* eulerArray = DataArray<float32>::Create(dataStructure, "EulerAngles", eulerDataStore, cellAM->getId());
  auto& eulerStore = eulerArray->getDataStoreRef();

  auto phasesDataStore = DataStoreUtilities::CreateDataStore<int32>(dataStructure, k_CellDataPath.createChildPath("Phases"), cellTupleShape, {1}, IDataAction::Mode::Execute);
  auto* phasesArray = DataArray<int32>::Create(dataStructure, "Phases", phasesDataStore, cellAM->getId());
  auto& phasesStore = phasesArray->getDataStoreRef();

  const usize blocksPerDimX = dimX / blockSize;
  const usize blocksPerDimY = dimY / blockSize;

  // Use Z-slice buffered writes to limit OOC memory use.
  std::vector<int32> featureIdsBuf(sliceSize);
  std::vector<float32> eulerBuf(sliceSize * 3);
  std::vector<int32> phasesBuf(sliceSize);

  for(usize z = 0; z < dimZ; z++)
  {
    for(usize y = 0; y < dimY; y++)
    {
      for(usize x = 0; x < dimX; x++)
      {
        const usize inSlice = y * dimX + x;
        phasesBuf[inSlice] = 1;

        usize bx = x / blockSize;
        usize by = y / blockSize;
        usize bz = z / blockSize;
        int32 blockFeatureId = static_cast<int32>(bz * blocksPerDimY * blocksPerDimX + by * blocksPerDimX + bx + 1);

        bool isBad = ((x * 7 + y * 13 + z * 29) % 10 == 0);
        featureIdsBuf[inSlice] = isBad ? 0 : blockFeatureId;

        const usize eIdx = inSlice * 3;
        eulerBuf[eIdx] = static_cast<float32>(x) / static_cast<float32>(dimX);
        eulerBuf[eIdx + 1] = static_cast<float32>(y) / static_cast<float32>(dimY);
        eulerBuf[eIdx + 2] = static_cast<float32>(z) / static_cast<float32>(dimZ);
      }
    }
    const usize zOffset = z * sliceSize;
    featureIdsStore.copyFromBuffer(zOffset, nonstd::span<const int32>(featureIdsBuf.data(), sliceSize));
    eulerStore.copyFromBuffer(zOffset * 3, nonstd::span<const float32>(eulerBuf.data(), sliceSize * 3));
    phasesStore.copyFromBuffer(zOffset, nonstd::span<const int32>(phasesBuf.data(), sliceSize));
  }
}

usize CountBadVoxels(const DataStructure& dataStructure, usize dimX, usize dimY, usize dimZ)
{
  const auto& featureIds = dataStructure.getDataRefAs<Int32Array>(k_FeatureIdsPath).getDataStoreRef();
  const usize sliceSize = dimX * dimY;
  std::vector<int32> buf(sliceSize);
  usize count = 0;
  for(usize z = 0; z < dimZ; z++)
  {
    featureIds.copyIntoBuffer(z * sliceSize, nonstd::span<int32>(buf.data(), sliceSize));
    for(usize i = 0; i < sliceSize; i++)
    {
      if(buf[i] == 0)
      {
        count++;
      }
    }
  }
  return count;
}
} // namespace

TEST_CASE("SimplnxCore::ErodeDilateBadDataFilter: Generate Test Data", "[SimplnxCore][ErodeDilateBadDataFilter][.GenerateTestData]")
{
  const auto outputDir = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "generated_test_data" / "erode_dilate_bad_data";
  fs::create_directories(outputDir);

  // The small fixture uses a 20-cubed volume and block size 5.
  {
    DataStructure buildDS;
    BuildTestData(buildDS, 20, 20, 20, 5);
    UnitTest::WriteTestDataStructure(buildDS, outputDir / "small_input.dream3d");
  }

  // The large fixture uses a 200-cubed volume and block size 25.
  {
    DataStructure buildDS;
    BuildTestData(buildDS, 200, 200, 200, 25);
    UnitTest::WriteTestDataStructure(buildDS, outputDir / "large_input.dream3d");
  }
}
