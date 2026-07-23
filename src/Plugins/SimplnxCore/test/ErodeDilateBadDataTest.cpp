#include "SimplnxCore/SimplnxCore_test_dirs.hpp"
#include <catch2/catch.hpp>

#include "SimplnxCore/Filters/Algorithms/ErodeDilateBadData.hpp"
#include "SimplnxCore/Filters/ErodeDilateBadDataFilter.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/FileIO.hpp"

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::Constants;
using namespace nx::core::UnitTest;

namespace
{
constexpr ChoicesParameter::ValueType k_Dilate = 0ULL;
constexpr ChoicesParameter::ValueType k_Erode = 1ULL;

const std::string k_EbsdScanDataName("EBSD Scan Data");

const DataPath k_InputData({"Input Data"});
const DataPath k_EbsdScanDataDataPath = k_InputData.createChildPath(k_EbsdScanDataName);
const DataPath k_FeatureIdsDataPath = k_EbsdScanDataDataPath.createChildPath("FeatureIds");
const StringLiteral k_MiscData = "Misc";

const ShapeType k_TupleShape{4, 4, 2};
const usize k_NumTuples = 32;
const DataPath k_DataPath({::k_ImageGeometry, ::k_CellData, k_MiscData});
const DataPath k_ImageFeatureIdsPath({::k_ImageGeometry, ::k_CellData, k_FeatureIds});
} // namespace

DataStructure CreateTestData()
{
  DataStructure dataStructure;
  auto* geom = ImageGeom::Create(dataStructure, ::k_ImageGeometry);
  geom->setDimensions(SizeVec3{k_TupleShape[0], k_TupleShape[1], k_TupleShape[2]});

  auto* cellData = AttributeMatrix::Create(dataStructure, ::k_CellData, k_TupleShape, geom->getId());

  // Feature IDs
  auto featureIdsPtr = std::make_shared<Int32DataStore>(k_NumTuples, 0);
  auto* featureIdsArray = Int32Array::Create(dataStructure, ::k_FeatureIds, featureIdsPtr, cellData->getId());

  // Index 0, 14, 31
  auto& featureIds = featureIdsArray->getDataStoreRef();
  featureIds[0] = 0;
  featureIds[1] = 1;
  featureIds[2] = 1;
  featureIds[3] = 2;

  featureIds[4] = 2;
  featureIds[5] = 1;
  featureIds[6] = 2;
  featureIds[7] = 2;

  featureIds[8] = 1;
  featureIds[9] = 1;
  featureIds[10] = 0;
  featureIds[11] = 2;

  featureIds[12] = 2;
  featureIds[13] = 0;
  featureIds[14] = 0;
  featureIds[15] = 3;
  // Z
  featureIds[16] = 4;
  featureIds[17] = 4;
  featureIds[18] = 4;
  featureIds[19] = 4;

  featureIds[20] = 3;
  featureIds[21] = 3;
  featureIds[22] = 3;
  featureIds[23] = 3;

  featureIds[24] = 5;
  featureIds[25] = 5;
  featureIds[26] = 5;
  featureIds[27] = 5;

  featureIds[28] = 5;
  featureIds[29] = 6;
  featureIds[30] = 6;
  featureIds[31] = 0;

  // Misc DataArray
  auto dataStorePtr = std::make_shared<Int32DataStore>(k_NumTuples, 0);
  auto* miscArray = Int32Array::Create(dataStructure, k_MiscData, dataStorePtr, cellData->getId());

  auto& dataStore = miscArray->getDataStoreRef();
  for(usize i = 0; i < dataStore.size(); i++)
  {
    dataStore[i] = i;
  }

  return dataStructure;
}

// Erode 1
void CheckDataErode1XYZ(const Int32AbstractDataStore& featureIds, const Int32AbstractDataStore& dataStore)
{
  std::vector<int32> exemplarData{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 6, 11, 12, 9, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 15};
  std::vector<int32> exemplarFeatures{0, 1, 1, 2, 2, 1, 2, 2, 1, 1, 2, 2, 2, 1, 0, 3, 4, 4, 4, 4, 3, 3, 3, 3, 5, 5, 5, 5, 5, 6, 6, 3};

  for(usize i = 0; i < dataStore.size(); i++)
  {
    REQUIRE(dataStore[i] == exemplarData[i]);
    REQUIRE(featureIds[i] == exemplarFeatures[i]);
  }
}
void CheckDataErode1YZ(const Int32AbstractDataStore& featureIds, const Int32AbstractDataStore& dataStore)
{
  std::vector<int32> exemplarData{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 6, 11, 12, 9, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 15};
  std::vector<int32> exemplarFeatures{0, 1, 1, 2, 2, 1, 2, 2, 1, 1, 2, 2, 2, 1, 0, 3, 4, 4, 4, 4, 3, 3, 3, 3, 5, 5, 5, 5, 5, 6, 6, 3};

  for(usize i = 0; i < dataStore.size(); i++)
  {
    REQUIRE(dataStore[i] == exemplarData[i]);
    REQUIRE(featureIds[i] == exemplarFeatures[i]);
  }
}
void CheckDataErode1Z(const Int32AbstractDataStore& featureIds, const Int32AbstractDataStore& dataStore)
{
  std::vector<int32> exemplarData{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 6, 11, 12, 9, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 15};
  std::vector<int32> exemplarFeatures{0, 1, 1, 2, 2, 1, 2, 2, 1, 1, 2, 2, 2, 1, 0, 3, 4, 4, 4, 4, 3, 3, 3, 3, 5, 5, 5, 5, 5, 6, 6, 3};

  for(usize i = 0; i < dataStore.size(); i++)
  {
    REQUIRE(dataStore[i] == exemplarData[i]);
    REQUIRE(featureIds[i] == exemplarFeatures[i]);
  }
}
void CheckDataErode1XZ(const Int32AbstractDataStore& featureIds, const Int32AbstractDataStore& dataStore)
{
  std::vector<int32> exemplarData{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 6, 11, 12, 9, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 15};
  std::vector<int32> exemplarFeatures{0, 1, 1, 2, 2, 1, 2, 2, 1, 1, 2, 2, 2, 1, 0, 3, 4, 4, 4, 4, 3, 3, 3, 3, 5, 5, 5, 5, 5, 6, 6, 3};

  for(usize i = 0; i < dataStore.size(); i++)
  {
    REQUIRE(dataStore[i] == exemplarData[i]);
    REQUIRE(featureIds[i] == exemplarFeatures[i]);
  }
}
void CheckDataErode1X(const Int32AbstractDataStore& featureIds, const Int32AbstractDataStore& dataStore)
{
  std::vector<int32> exemplarData{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 6, 11, 12, 9, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 15};
  std::vector<int32> exemplarFeatures{0, 1, 1, 2, 2, 1, 2, 2, 1, 1, 2, 2, 2, 1, 0, 3, 4, 4, 4, 4, 3, 3, 3, 3, 5, 5, 5, 5, 5, 6, 6, 3};

  for(usize i = 0; i < dataStore.size(); i++)
  {
    REQUIRE(dataStore[i] == exemplarData[i]);
    REQUIRE(featureIds[i] == exemplarFeatures[i]);
  }
}
void CheckDataErode1XY(const Int32AbstractDataStore& featureIds, const Int32AbstractDataStore& dataStore)
{
  std::vector<int32> exemplarData{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 6, 11, 12, 9, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 15};
  std::vector<int32> exemplarFeatures{0, 1, 1, 2, 2, 1, 2, 2, 1, 1, 2, 2, 2, 1, 0, 3, 4, 4, 4, 4, 3, 3, 3, 3, 5, 5, 5, 5, 5, 6, 6, 3};

  for(usize i = 0; i < dataStore.size(); i++)
  {
    REQUIRE(dataStore[i] == exemplarData[i]);
    REQUIRE(featureIds[i] == exemplarFeatures[i]);
  }
}
void CheckDataErode1Y(const Int32AbstractDataStore& featureIds, const Int32AbstractDataStore& dataStore)
{
  std::vector<int32> exemplarData{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 6, 11, 12, 9, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 15};
  std::vector<int32> exemplarFeatures{0, 1, 1, 2, 2, 1, 2, 2, 1, 1, 2, 2, 2, 1, 0, 3, 4, 4, 4, 4, 3, 3, 3, 3, 5, 5, 5, 5, 5, 6, 6, 3};

  for(usize i = 0; i < dataStore.size(); i++)
  {
    REQUIRE(dataStore[i] == exemplarData[i]);
    REQUIRE(featureIds[i] == exemplarFeatures[i]);
  }
}

void CheckDataErode1(const Int32AbstractDataStore& featureIds, const Int32AbstractDataStore& dataStore, const std::array<bool, 3>& dir)
{
  if(dir[0])
  {
    if(dir[1])
    {
      if(dir[2])
      {
        CheckDataErode1XYZ(featureIds, dataStore);
      }
      else
      {
        CheckDataErode1XY(featureIds, dataStore);
      }
    }
    // Not Y
    else
    {
      if(dir[2])
      {
        CheckDataErode1XZ(featureIds, dataStore);
      }
      else
      {
        CheckDataErode1X(featureIds, dataStore);
      }
    }
  }
  // Not X
  else
  {
    if(dir[1])
    {
      if(dir[2])
      {
        CheckDataErode1YZ(featureIds, dataStore);
      }
      else
      {
        CheckDataErode1Y(featureIds, dataStore);
      }
    }
    // Not Y
    else
    {
      if(dir[2])
      {
        CheckDataErode1Z(featureIds, dataStore);
      }
      else
      {
        REQUIRE(false);
      }
    }
  }
}

// Erode 2
void CheckDataErode2XYZ(const Int32AbstractDataStore& featureIds, const Int32AbstractDataStore& dataStore)
{
  std::vector<int32> exemplarData{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 6, 11, 12, 9, 6, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 15};
  std::vector<int32> exemplarFeatures{0, 1, 1, 2, 2, 1, 2, 2, 1, 1, 2, 2, 2, 1, 2, 3, 4, 4, 4, 4, 3, 3, 3, 3, 5, 5, 5, 5, 5, 6, 6, 3};

  for(usize i = 0; i < dataStore.size(); i++)
  {
    REQUIRE(dataStore[i] == exemplarData[i]);
    REQUIRE(featureIds[i] == exemplarFeatures[i]);
  }
}
void CheckDataErode2YZ(const Int32AbstractDataStore& featureIds, const Int32AbstractDataStore& dataStore)
{
  std::vector<int32> exemplarData{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 6, 11, 12, 9, 6, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 15};
  std::vector<int32> exemplarFeatures{0, 1, 1, 2, 2, 1, 2, 2, 1, 1, 2, 2, 2, 1, 2, 3, 4, 4, 4, 4, 3, 3, 3, 3, 5, 5, 5, 5, 5, 6, 6, 3};

  for(usize i = 0; i < dataStore.size(); i++)
  {
    REQUIRE(dataStore[i] == exemplarData[i]);
    REQUIRE(featureIds[i] == exemplarFeatures[i]);
  }
}
void CheckDataErode2Z(const Int32AbstractDataStore& featureIds, const Int32AbstractDataStore& dataStore)
{
  std::vector<int32> exemplarData{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 6, 11, 12, 9, 6, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 15};
  std::vector<int32> exemplarFeatures{0, 1, 1, 2, 2, 1, 2, 2, 1, 1, 2, 2, 2, 1, 2, 3, 4, 4, 4, 4, 3, 3, 3, 3, 5, 5, 5, 5, 5, 6, 6, 3};

  for(usize i = 0; i < dataStore.size(); i++)
  {
    REQUIRE(dataStore[i] == exemplarData[i]);
    REQUIRE(featureIds[i] == exemplarFeatures[i]);
  }
}
void CheckDataErode2XY(const Int32AbstractDataStore& featureIds, const Int32AbstractDataStore& dataStore)
{
  std::vector<int32> exemplarData{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 6, 11, 12, 9, 6, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 15};
  std::vector<int32> exemplarFeatures{0, 1, 1, 2, 2, 1, 2, 2, 1, 1, 2, 2, 2, 1, 2, 3, 4, 4, 4, 4, 3, 3, 3, 3, 5, 5, 5, 5, 5, 6, 6, 3};

  for(usize i = 0; i < dataStore.size(); i++)
  {
    REQUIRE(dataStore[i] == exemplarData[i]);
    REQUIRE(featureIds[i] == exemplarFeatures[i]);
  }
}
void CheckDataErode2X(const Int32AbstractDataStore& featureIds, const Int32AbstractDataStore& dataStore)
{
  std::vector<int32> exemplarData{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 6, 11, 12, 9, 6, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 15};
  std::vector<int32> exemplarFeatures{0, 1, 1, 2, 2, 1, 2, 2, 1, 1, 2, 2, 2, 1, 2, 3, 4, 4, 4, 4, 3, 3, 3, 3, 5, 5, 5, 5, 5, 6, 6, 3};

  for(usize i = 0; i < dataStore.size(); i++)
  {
    REQUIRE(dataStore[i] == exemplarData[i]);
    REQUIRE(featureIds[i] == exemplarFeatures[i]);
  }
}
void CheckDataErode2XZ(const Int32AbstractDataStore& featureIds, const Int32AbstractDataStore& dataStore)
{
  std::vector<int32> exemplarData{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 6, 11, 12, 9, 6, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 15};
  std::vector<int32> exemplarFeatures{0, 1, 1, 2, 2, 1, 2, 2, 1, 1, 2, 2, 2, 1, 2, 3, 4, 4, 4, 4, 3, 3, 3, 3, 5, 5, 5, 5, 5, 6, 6, 3};

  for(usize i = 0; i < dataStore.size(); i++)
  {
    REQUIRE(dataStore[i] == exemplarData[i]);
    REQUIRE(featureIds[i] == exemplarFeatures[i]);
  }
}
void CheckDataErode2Y(const Int32AbstractDataStore& featureIds, const Int32AbstractDataStore& dataStore)
{
  std::vector<int32> exemplarData{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 6, 11, 12, 9, 6, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 15};
  std::vector<int32> exemplarFeatures{0, 1, 1, 2, 2, 1, 2, 2, 1, 1, 2, 2, 2, 1, 2, 3, 4, 4, 4, 4, 3, 3, 3, 3, 5, 5, 5, 5, 5, 6, 6, 3};

  for(usize i = 0; i < dataStore.size(); i++)
  {
    REQUIRE(dataStore[i] == exemplarData[i]);
    REQUIRE(featureIds[i] == exemplarFeatures[i]);
  }
}

void CheckDataErode2(const Int32AbstractDataStore& featureIds, const Int32AbstractDataStore& dataStore, const std::array<bool, 3>& dir)
{
  if(dir[0])
  {
    if(dir[1])
    {
      if(dir[2])
      {
        CheckDataErode2XYZ(featureIds, dataStore);
      }
      else
      {
        CheckDataErode2XY(featureIds, dataStore);
      }
    }
    // Not Y
    else
    {
      if(dir[2])
      {
        CheckDataErode2XZ(featureIds, dataStore);
      }
      else
      {
        CheckDataErode2X(featureIds, dataStore);
      }
    }
  }
  // Not X
  else
  {
    if(dir[1])
    {
      if(dir[2])
      {
        CheckDataErode2YZ(featureIds, dataStore);
      }
      else
      {
        CheckDataErode2Y(featureIds, dataStore);
      }
    }
    // Not Y
    else
    {
      if(dir[2])
      {
        CheckDataErode2Z(featureIds, dataStore);
      }
      else
      {
        REQUIRE(false);
      }
    }
  }
}

void CheckDataErode(Int32Array& featureIdsArray, Int32Array& dataArray, int32 numIterations, const std::array<bool, 3>& directions)
{
  const auto& featureIds = featureIdsArray.getDataStoreRef();
  const auto& dataStore = dataArray.getDataStoreRef();

  // Close up 0 features
  switch(numIterations)
  {
  case 1:
    CheckDataErode1(featureIds, dataStore, directions);
    break;
  case 2:
    CheckDataErode2(featureIds, dataStore, directions);
    break;
  default:
    REQUIRE(false);
  }
}

// Dilate
void CheckDataDilate1XYZ(const Int32AbstractDataStore& featureIds, const Int32AbstractDataStore& dataStore)
{
  std::vector<int32> exemplarData{0, 1, 2, 3, 4, 5, 10, 7, 8, 13, 10, 11, 12, 13, 14, 31, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 31, 28, 29, 30, 31};
  std::vector<int32> exemplarFeatures{0, 1, 1, 2, 2, 1, 0, 2, 1, 0, 0, 2, 2, 0, 0, 0, 4, 4, 4, 4, 3, 3, 3, 3, 5, 5, 5, 0, 5, 6, 6, 0};

  for(usize i = 0; i < dataStore.size(); i++)
  {
    REQUIRE(dataStore[i] == exemplarData[i]);
    REQUIRE(featureIds[i] == exemplarFeatures[i]);
  }
}
void CheckDataDilate1XY(const Int32AbstractDataStore& featureIds, const Int32AbstractDataStore& dataStore)
{
  std::vector<int32> exemplarData{0, 1, 2, 3, 4, 5, 10, 7, 8, 13, 10, 11, 12, 13, 14, 31, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 31, 28, 29, 30, 31};
  std::vector<int32> exemplarFeatures{0, 1, 1, 2, 2, 1, 0, 2, 1, 0, 0, 2, 2, 0, 0, 0, 4, 4, 4, 4, 3, 3, 3, 3, 5, 5, 5, 0, 5, 6, 6, 0};

  for(usize i = 0; i < dataStore.size(); i++)
  {
    REQUIRE(dataStore[i] == exemplarData[i]);
    REQUIRE(featureIds[i] == exemplarFeatures[i]);
  }
}
void CheckDataDilate1XZ(const Int32AbstractDataStore& featureIds, const Int32AbstractDataStore& dataStore)
{
  std::vector<int32> exemplarData{0, 1, 2, 3, 4, 5, 10, 7, 8, 13, 10, 11, 12, 13, 14, 31, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 31, 28, 29, 30, 31};
  std::vector<int32> exemplarFeatures{0, 1, 1, 2, 2, 1, 0, 2, 1, 0, 0, 2, 2, 0, 0, 0, 4, 4, 4, 4, 3, 3, 3, 3, 5, 5, 5, 0, 5, 6, 6, 0};

  for(usize i = 0; i < dataStore.size(); i++)
  {
    REQUIRE(dataStore[i] == exemplarData[i]);
    REQUIRE(featureIds[i] == exemplarFeatures[i]);
  }
}
void CheckDataDilate1X(const Int32AbstractDataStore& featureIds, const Int32AbstractDataStore& dataStore)
{
  std::vector<int32> exemplarData{0, 1, 2, 3, 4, 5, 10, 7, 8, 13, 10, 11, 12, 13, 14, 31, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 31, 28, 29, 30, 31};
  std::vector<int32> exemplarFeatures{0, 1, 1, 2, 2, 1, 0, 2, 1, 0, 0, 2, 2, 0, 0, 0, 4, 4, 4, 4, 3, 3, 3, 3, 5, 5, 5, 0, 5, 6, 6, 0};

  for(usize i = 0; i < dataStore.size(); i++)
  {
    REQUIRE(dataStore[i] == exemplarData[i]);
    REQUIRE(featureIds[i] == exemplarFeatures[i]);
  }
}
void CheckDataDilate1YZ(const Int32AbstractDataStore& featureIds, const Int32AbstractDataStore& dataStore)
{
  std::vector<int32> exemplarData{0, 1, 2, 3, 4, 5, 10, 7, 8, 13, 10, 11, 12, 13, 14, 31, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 31, 28, 29, 30, 31};
  std::vector<int32> exemplarFeatures{0, 1, 1, 2, 2, 1, 0, 2, 1, 0, 0, 2, 2, 0, 0, 0, 4, 4, 4, 4, 3, 3, 3, 3, 5, 5, 5, 0, 5, 6, 6, 0};

  for(usize i = 0; i < dataStore.size(); i++)
  {
    REQUIRE(dataStore[i] == exemplarData[i]);
    REQUIRE(featureIds[i] == exemplarFeatures[i]);
  }
}
void CheckDataDilate1Y(const Int32AbstractDataStore& featureIds, const Int32AbstractDataStore& dataStore)
{
  std::vector<int32> exemplarData{0, 1, 2, 3, 4, 5, 10, 7, 8, 13, 10, 11, 12, 13, 14, 31, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 31, 28, 29, 30, 31};
  std::vector<int32> exemplarFeatures{0, 1, 1, 2, 2, 1, 0, 2, 1, 0, 0, 2, 2, 0, 0, 0, 4, 4, 4, 4, 3, 3, 3, 3, 5, 5, 5, 0, 5, 6, 6, 0};

  for(usize i = 0; i < dataStore.size(); i++)
  {
    REQUIRE(dataStore[i] == exemplarData[i]);
    REQUIRE(featureIds[i] == exemplarFeatures[i]);
  }
}
void CheckDataDilate1Z(const Int32AbstractDataStore& featureIds, const Int32AbstractDataStore& dataStore)
{
  std::vector<int32> exemplarData{0, 1, 2, 3, 4, 5, 10, 7, 8, 13, 10, 11, 12, 13, 14, 31, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 31, 28, 29, 30, 31};
  std::vector<int32> exemplarFeatures{0, 1, 1, 2, 2, 1, 0, 2, 1, 0, 0, 2, 2, 0, 0, 0, 4, 4, 4, 4, 3, 3, 3, 3, 5, 5, 5, 0, 5, 6, 6, 0};

  for(usize i = 0; i < dataStore.size(); i++)
  {
    REQUIRE(dataStore[i] == exemplarData[i]);
    REQUIRE(featureIds[i] == exemplarFeatures[i]);
  }
}

void CheckDataDilate1(const Int32AbstractDataStore& featureIds, const Int32AbstractDataStore& dataStore, const std::array<bool, 3>& dir)
{
  if(dir[0])
  {
    if(dir[1])
    {
      if(dir[2])
      {
        CheckDataDilate1XYZ(featureIds, dataStore);
      }
      else
      {
        CheckDataDilate1XY(featureIds, dataStore);
      }
    }
    // Not Y
    else
    {
      if(dir[2])
      {
        CheckDataDilate1XZ(featureIds, dataStore);
      }
      else
      {
        CheckDataDilate1X(featureIds, dataStore);
      }
    }
  }
  // Not X
  else
  {
    if(dir[1])
    {
      if(dir[2])
      {
        CheckDataDilate1YZ(featureIds, dataStore);
      }
      else
      {
        CheckDataDilate1Y(featureIds, dataStore);
      }
    }
    // Not Y
    else
    {
      if(dir[2])
      {
        CheckDataDilate1Z(featureIds, dataStore);
      }
      else
      {
        REQUIRE(false);
      }
    }
  }
}

void CheckDataDilate2XYZ(const Int32AbstractDataStore& featureIds, const Int32AbstractDataStore& dataStore)
{
  std::vector<int32> exemplarData{0, 1, 10, 3, 4, 13, 10, 7, 8, 13, 10, 31, 12, 13, 14, 31, 16, 17, 18, 19, 20, 21, 22, 31, 24, 25, 26, 31, 28, 29, 30, 31};
  std::vector<int32> exemplarFeatures{0, 1, 0, 2, 2, 0, 0, 2, 1, 0, 0, 0, 2, 0, 0, 0, 4, 4, 4, 4, 3, 3, 3, 0, 5, 5, 5, 0, 5, 6, 6, 0};

  for(usize i = 0; i < dataStore.size(); i++)
  {
    REQUIRE(dataStore[i] == exemplarData[i]);
    REQUIRE(featureIds[i] == exemplarFeatures[i]);
  }
}
void CheckDataDilate2XY(const Int32AbstractDataStore& featureIds, const Int32AbstractDataStore& dataStore)
{
  std::vector<int32> exemplarData{0, 1, 10, 3, 4, 13, 10, 7, 8, 13, 10, 31, 12, 13, 14, 31, 16, 17, 18, 19, 20, 21, 22, 31, 24, 25, 26, 31, 28, 29, 30, 31};
  std::vector<int32> exemplarFeatures{0, 1, 0, 2, 2, 0, 0, 2, 1, 0, 0, 0, 2, 0, 0, 0, 4, 4, 4, 4, 3, 3, 3, 0, 5, 5, 5, 0, 5, 6, 6, 0};

  for(usize i = 0; i < dataStore.size(); i++)
  {
    REQUIRE(dataStore[i] == exemplarData[i]);
    REQUIRE(featureIds[i] == exemplarFeatures[i]);
  }
}
void CheckDataDilate2XZ(const Int32AbstractDataStore& featureIds, const Int32AbstractDataStore& dataStore)
{
  std::vector<int32> exemplarData{0, 1, 10, 3, 4, 13, 10, 7, 8, 13, 10, 31, 12, 13, 14, 31, 16, 17, 18, 19, 20, 21, 22, 31, 24, 25, 26, 31, 28, 29, 30, 31};
  std::vector<int32> exemplarFeatures{0, 1, 0, 2, 2, 0, 0, 2, 1, 0, 0, 0, 2, 0, 0, 0, 4, 4, 4, 4, 3, 3, 3, 0, 5, 5, 5, 0, 5, 6, 6, 0};

  for(usize i = 0; i < dataStore.size(); i++)
  {
    REQUIRE(dataStore[i] == exemplarData[i]);
    REQUIRE(featureIds[i] == exemplarFeatures[i]);
  }
}
void CheckDataDilate2X(const Int32AbstractDataStore& featureIds, const Int32AbstractDataStore& dataStore)
{
  std::vector<int32> exemplarData{0, 1, 10, 3, 4, 13, 10, 7, 8, 13, 10, 31, 12, 13, 14, 31, 16, 17, 18, 19, 20, 21, 22, 31, 24, 25, 26, 31, 28, 29, 30, 31};
  std::vector<int32> exemplarFeatures{0, 1, 0, 2, 2, 0, 0, 2, 1, 0, 0, 0, 2, 0, 0, 0, 4, 4, 4, 4, 3, 3, 3, 0, 5, 5, 5, 0, 5, 6, 6, 0};

  for(usize i = 0; i < dataStore.size(); i++)
  {
    REQUIRE(dataStore[i] == exemplarData[i]);
    REQUIRE(featureIds[i] == exemplarFeatures[i]);
  }
}
void CheckDataDilate2YZ(const Int32AbstractDataStore& featureIds, const Int32AbstractDataStore& dataStore)
{
  std::vector<int32> exemplarData{0, 1, 10, 3, 4, 13, 10, 7, 8, 13, 10, 31, 12, 13, 14, 31, 16, 17, 18, 19, 20, 21, 22, 31, 24, 25, 26, 31, 28, 29, 30, 31};
  std::vector<int32> exemplarFeatures{0, 1, 0, 2, 2, 0, 0, 2, 1, 0, 0, 0, 2, 0, 0, 0, 4, 4, 4, 4, 3, 3, 3, 0, 5, 5, 5, 0, 5, 6, 6, 0};

  for(usize i = 0; i < dataStore.size(); i++)
  {
    REQUIRE(dataStore[i] == exemplarData[i]);
    REQUIRE(featureIds[i] == exemplarFeatures[i]);
  }
}
void CheckDataDilate2Y(const Int32AbstractDataStore& featureIds, const Int32AbstractDataStore& dataStore)
{
  std::vector<int32> exemplarData{0, 1, 10, 3, 4, 13, 10, 7, 8, 13, 10, 31, 12, 13, 14, 31, 16, 17, 18, 19, 20, 21, 22, 31, 24, 25, 26, 31, 28, 29, 30, 31};
  std::vector<int32> exemplarFeatures{0, 1, 0, 2, 2, 0, 0, 2, 1, 0, 0, 0, 2, 0, 0, 0, 4, 4, 4, 4, 3, 3, 3, 0, 5, 5, 5, 0, 5, 6, 6, 0};

  for(usize i = 0; i < dataStore.size(); i++)
  {
    REQUIRE(dataStore[i] == exemplarData[i]);
    REQUIRE(featureIds[i] == exemplarFeatures[i]);
  }
}
void CheckDataDilate2Z(const Int32AbstractDataStore& featureIds, const Int32AbstractDataStore& dataStore)
{
  std::vector<int32> exemplarData{0, 1, 10, 3, 4, 13, 10, 7, 8, 13, 10, 31, 12, 13, 14, 31, 16, 17, 18, 19, 20, 21, 22, 31, 24, 25, 26, 31, 28, 29, 30, 31};
  std::vector<int32> exemplarFeatures{0, 1, 0, 2, 2, 0, 0, 2, 1, 0, 0, 0, 2, 0, 0, 0, 4, 4, 4, 4, 3, 3, 3, 0, 5, 5, 5, 0, 5, 6, 6, 0};

  for(usize i = 0; i < dataStore.size(); i++)
  {
    REQUIRE(dataStore[i] == exemplarData[i]);
    REQUIRE(featureIds[i] == exemplarFeatures[i]);
  }
}

void CheckDataDilate2(const Int32AbstractDataStore& featureIds, const Int32AbstractDataStore& dataStore, const std::array<bool, 3>& dir)
{
  if(dir[0])
  {
    if(dir[1])
    {
      if(dir[2])
      {
        CheckDataDilate2XYZ(featureIds, dataStore);
      }
      else
      {
        CheckDataDilate2XY(featureIds, dataStore);
      }
    }
    // Not Y
    else
    {
      if(dir[2])
      {
        CheckDataDilate2XZ(featureIds, dataStore);
      }
      else
      {
        CheckDataDilate2X(featureIds, dataStore);
      }
    }
  }
  // Not X
  else
  {
    if(dir[1])
    {
      if(dir[2])
      {
        CheckDataDilate2YZ(featureIds, dataStore);
      }
      else
      {
        CheckDataDilate2Y(featureIds, dataStore);
      }
    }
    // Not Y
    else
    {
      if(dir[2])
      {
        CheckDataDilate2Z(featureIds, dataStore);
      }
      else
      {
        REQUIRE(false);
      }
    }
  }
}

void CheckDataDilate(const Int32Array& featureIdsArray, const Int32Array& dataArray, usize numIterations, const std::array<bool, 3>& dir)
{
  const auto& featureIds = featureIdsArray.getDataStoreRef();
  const auto& dataStore = dataArray.getDataStoreRef();

  // Expand 0 features
  switch(numIterations)
  {
  case 1:
    CheckDataDilate1(featureIds, dataStore, dir);
    break;
  case 2:
    CheckDataDilate2(featureIds, dataStore, dir);
    break;
  default:
    REQUIRE(false);
  }
}

void RunFilter(DataStructure& dataStructure, ChoicesParameter::ValueType operation, int32 numIterations, const std::array<bool, 3>& directions, const DataPath& geometryPath,
               const DataPath& featureIdsPath)
{
  const ErodeDilateBadDataFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ErodeDilateBadDataFilter::k_Operation_Key, std::make_any<ChoicesParameter::ValueType>(operation));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_NumIterations_Key, std::make_any<int32>(numIterations));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_XDirOn_Key, std::make_any<bool>(directions[0]));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_YDirOn_Key, std::make_any<bool>(directions[1]));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_ZDirOn_Key, std::make_any<bool>(directions[2]));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(featureIdsPath));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(geometryPath));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  // Execute the filter and check the result
  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)
}

TEST_CASE("SimplnxCore::ErodeDilateBadDataFilter(Erode) Expanded", "[SimplnxCore][ErodeDilateBadDataFilter]")
{
  UnitTest::LoadPlugins();

  bool dirX = GENERATE(true, false);
  bool dirY = GENERATE(true, false);
  bool dirZ = GENERATE(true, false);

  DataStructure dataStructure = CreateTestData();
  std::array<bool, 3> directions = {dirX, dirY, dirZ};
  uint64 operation = nx::core::detail::k_ErodeIndex;
  int32 numIterations = GENERATE(1, 2);

  // At least one direction is required.
  if(!dirX && !dirY && !dirZ)
  {
    return;
  }

  RunFilter(dataStructure, operation, numIterations, directions, DataPath({k_ImageGeometry}), k_ImageFeatureIdsPath);
  auto& dataArray = dataStructure.getDataRefAs<Int32Array>(k_DataPath);
  auto& featureIdsArray = dataStructure.getDataRefAs<Int32Array>(k_ImageFeatureIdsPath);

  CheckDataErode(featureIdsArray, dataArray, numIterations, directions);
}

TEST_CASE("SimplnxCore::ErodeDilateBadDataFilter(Dilate) Expanded", "[SimplnxCore][ErodeDilateBadDataFilter]")
{
  UnitTest::LoadPlugins();

  bool dirX = GENERATE(true, false);
  bool dirY = GENERATE(true, false);
  bool dirZ = GENERATE(true, false);

  DataStructure dataStructure = CreateTestData();
  std::array<bool, 3> directions = {dirX, dirY, dirZ};
  uint64 operation = nx::core::detail::k_DilateIndex;
  int32 numIterations = GENERATE(1, 2);

  // At least one direction is required.
  if(!dirX && !dirY && !dirZ)
  {
    return;
  }

  RunFilter(dataStructure, operation, numIterations, directions, DataPath({k_ImageGeometry}), k_ImageFeatureIdsPath);
  auto& dataArray = dataStructure.getDataRefAs<Int32Array>(k_DataPath);
  auto& featureIdsArray = dataStructure.getDataRefAs<Int32Array>(k_ImageFeatureIdsPath);

  CheckDataDilate(featureIdsArray, dataArray, numIterations, directions);
}

TEST_CASE("SimplnxCore::ErodeDilateBadDataFilter(Dilate) No Dimensions", "[SimplnxCore][ErodeDilateBadDataFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = CreateTestData();
  std::array<bool, 3> directions = {false, false, false};
  int32 operation = GENERATE(0, 1);
  int32 numIterations = GENERATE(1, 2);

  DataPath imageGeomPath({k_ImageGeometry});

  const ErodeDilateBadDataFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ErodeDilateBadDataFilter::k_Operation_Key, std::make_any<ChoicesParameter::ValueType>(operation));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_NumIterations_Key, std::make_any<int32>(numIterations));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_XDirOn_Key, std::make_any<bool>(directions[0]));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_YDirOn_Key, std::make_any<bool>(directions[1]));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_ZDirOn_Key, std::make_any<bool>(directions[2]));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_ImageFeatureIdsPath));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(imageGeomPath));

  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  imageGeom.setDimensions(SizeVec3{0, 0, 0});

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
}

TEST_CASE("SimplnxCore::ErodeDilateBadDataFilter(Dilate) No Direction", "[SimplnxCore][ErodeDilateBadDataFilter]")
{
  UnitTest::LoadPlugins();

  DataStructure dataStructure = CreateTestData();
  std::array<bool, 3> directions = {false, false, false};
  int32 operation = GENERATE(0, 1);
  int32 numIterations = GENERATE(1, 2);

  const ErodeDilateBadDataFilter filter;
  Arguments args;

  // Create default Parameters for the filter.
  args.insertOrAssign(ErodeDilateBadDataFilter::k_Operation_Key, std::make_any<ChoicesParameter::ValueType>(operation));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_NumIterations_Key, std::make_any<int32>(numIterations));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_XDirOn_Key, std::make_any<bool>(directions[0]));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_YDirOn_Key, std::make_any<bool>(directions[1]));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_ZDirOn_Key, std::make_any<bool>(directions[2]));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(k_ImageFeatureIdsPath));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_IgnoredDataArrayPaths_Key, std::make_any<MultiArraySelectionParameter::ValueType>(MultiArraySelectionParameter::ValueType{}));
  args.insertOrAssign(ErodeDilateBadDataFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(DataPath({k_ImageGeometry})));

  // Preflight the filter and check result
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions)
}

TEST_CASE("SimplnxCore::ErodeDilateBadDataFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ErodeDilateBadDataFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ErodeDilateBadDataFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ErodeDilateBadDataFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ErodeDilateBadDataFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<ChoicesParameter::ValueType>(ErodeDilateBadDataFilter::k_Operation_Key) == k_Dilate);
      CHECK(args.value<int32>(ErodeDilateBadDataFilter::k_NumIterations_Key) == 5);
      CHECK(args.value<bool>(ErodeDilateBadDataFilter::k_XDirOn_Key) == true);
      CHECK(args.value<bool>(ErodeDilateBadDataFilter::k_YDirOn_Key) == true);
      CHECK(args.value<bool>(ErodeDilateBadDataFilter::k_ZDirOn_Key) == true);
      CHECK(args.value<DataPath>(ErodeDilateBadDataFilter::k_SelectedImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(ErodeDilateBadDataFilter::k_CellFeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      // Complex type (MultiDataArraySelectionFilterParameterConverter) - verified by successful pipeline loading
    }
  }
}