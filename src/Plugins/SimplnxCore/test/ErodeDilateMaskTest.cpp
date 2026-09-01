#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/ErodeDilateMaskFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

#include <filesystem>
#include <memory>

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
const DataPath k_MaskPath = k_CellDataPath.createChildPath("Mask");

void BuildTestData(DataStructure& dataStructure, usize dimX, usize dimY, usize dimZ)
{
  const ShapeType cellTupleShape = {dimZ, dimY, dimX};
  const usize sliceSize = dimX * dimY;

  auto* imageGeom = ImageGeom::Create(dataStructure, k_GeomName);
  imageGeom->setDimensions({dimX, dimY, dimZ});
  imageGeom->setSpacing({1.0f, 1.0f, 1.0f});
  imageGeom->setOrigin({0.0f, 0.0f, 0.0f});

  auto* cellAM = AttributeMatrix::Create(dataStructure, k_CellDataName, cellTupleShape, imageGeom->getId());
  imageGeom->setCellData(*cellAM);

  auto maskDataStore = DataStoreUtilities::CreateDataStore<bool>(dataStructure, k_MaskPath, cellTupleShape, {1}, IDataAction::Mode::Execute);
  auto* maskArray = DataArray<bool>::Create(dataStructure, "Mask", maskDataStore, cellAM->getId());
  auto& maskStore = maskArray->getDataStoreRef();

  // Use Z-slice buffered writes. Bool data requires a raw array instead of vector<bool>.
  auto maskBuf = std::make_unique<bool[]>(sliceSize);

  for(usize z = 0; z < dimZ; z++)
  {
    for(usize y = 0; y < dimY; y++)
    {
      for(usize x = 0; x < dimX; x++)
      {
        const usize inSlice = y * dimX + x;
        maskBuf[inSlice] = ((x * 7 + y * 13 + z * 29) % 3 != 0);
      }
    }
    maskStore.copyFromBuffer(z * sliceSize, nonstd::span<const bool>(maskBuf.get(), sliceSize));
  }
}

usize CountTrueVoxels(const DataStructure& dataStructure, usize dimX, usize dimY, usize dimZ)
{
  const auto& mask = dataStructure.getDataRefAs<BoolArray>(k_MaskPath).getDataStoreRef();
  const usize sliceSize = dimX * dimY;
  auto buf = std::make_unique<bool[]>(sliceSize);
  usize count = 0;
  for(usize z = 0; z < dimZ; z++)
  {
    mask.copyIntoBuffer(z * sliceSize, nonstd::span<bool>(buf.get(), sliceSize));
    for(usize i = 0; i < sliceSize; i++)
    {
      if(buf[i])
      {
        count++;
      }
    }
  }
  return count;
}
} // namespace

TEST_CASE("SimplnxCore::ErodeDilateMaskFilter: Generate Test Data", "[SimplnxCore][ErodeDilateMaskFilter][.GenerateTestData]")
{
  const auto outputDir = fs::path(unit_test::k_BinaryTestOutputDir.view()) / "generated_test_data" / "erode_dilate_mask";
  fs::create_directories(outputDir);

  // The small fixture uses a 20-cubed volume.
  {
    DataStructure buildDS;
    BuildTestData(buildDS, 20, 20, 20);
    UnitTest::WriteTestDataStructure(buildDS, outputDir / "small_input.dream3d");
  }

  // The large fixture uses a 200-cubed volume.
  {
    DataStructure buildDS;
    BuildTestData(buildDS, 200, 200, 200);
    UnitTest::WriteTestDataStructure(buildDS, outputDir / "large_input.dream3d");
  }
}
