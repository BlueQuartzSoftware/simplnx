#include <catch2/catch.hpp>

#include "complex/Core/Application.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/Filter/FilterHandle.hpp"
#include "complex/Parameters/DynamicTableParameter.hpp"
#include "complex/Pipeline/Pipeline.hpp"

#include "complex/unit_test/complex_test_dirs.hpp"

using namespace complex;

namespace
{
const Uuid k_CreateDataArrayId = *Uuid::FromString("67041f9b-bdc6-4122-acc6-c9fe9280e90d");
const Uuid k_CorePluginId = *Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f");
const FilterHandle k_CreateDataArrayHandle(k_CreateDataArrayId, k_CorePluginId);

const Uuid k_CreateImageGeomId = *Uuid::FromString("c4320659-1a84-561d-939e-c7c10229a504");
const FilterHandle k_CreateImageGeomHandle(k_CreateImageGeomId, k_CorePluginId);

const Uuid k_InitializeDataId = *Uuid::FromString("dfab9921-fea3-521c-99ba-48db98e43ff8");
const FilterHandle k_InitializeDataHandle(k_InitializeDataId, k_CorePluginId);

const Uuid k_CropImageGeomId = *Uuid::FromString("baa4b7fe-31e5-5e63-a2cb-0bb9d844cfaf");
const FilterHandle k_CropImageGeomHandle(k_CropImageGeomId, k_CorePluginId);

const DataPath k_ImageGeomPath({"ImageGeom"});
const DataPath k_NewImageGeomPath({"NewImageGeom"});
const DataPath k_DataArrayPath({"ImageGeom", "DataArray"});
} // namespace

Pipeline createPipeline()
{
  Pipeline pipeline;

  // Create ImageGeom
  {
    std::vector<uint64> dimensions{5, 10, 1};
    std::vector<float32> origin{0.0f, 0.0f, 0.0f};
    std::vector<float32> spacing{1.0f, 1.0f, 1.0f};

    Arguments args;
    args.insert("GeometryDataPath", k_ImageGeomPath);
    args.insert("Dimensions", dimensions);
    args.insert("Origin", origin);
    args.insert("Spacing", spacing);
    pipeline.push_back(k_CreateImageGeomHandle, args);
  }

  // Create DataArray
  {
    DynamicTableData::TableDataType tableData{{50}};
    auto dynamicTableData = DynamicTableData::Create(tableData, {"Row"}, {"Col"});

    uint64_t numComponents = 1;
    std::string initValue = "4";

    Arguments args;
    args.insert("numeric_type", NumericType::int32);
    args.insert("component_count", numComponents);
    args.insert("tuple_dimensions", std::move(dynamicTableData));
    args.insert("output_data_array", k_DataArrayPath);
    args.insert("initialization_value", initValue);
    args.insert("in_memory", false);
    pipeline.push_back(k_CreateDataArrayHandle, args);
  }

  if(false)
  {
    std::vector<DataPath> voxelArrays{k_DataArrayPath};
    std::vector<uint64> minVoxel{0, 1, 0};
    std::vector<uint64> maxVoxel{1, 2, 0};
    std::string newGroupName = "Cropped Arrays";

    Arguments args;
    args.insert("min_voxel", minVoxel);
    args.insert("max_voxel", maxVoxel);
    args.insert("update_origin", false);
    args.insert("image_geom", k_ImageGeomPath);
    args.insert("new_image_geom", k_NewImageGeomPath);
    args.insert("voxel_arrays", maxVoxel);
    args.insert("renumber_features", false);
    args.insert("feature_ids", DataPath{});
    args.insert("new_features_group_name", newGroupName);
    pipeline.push_back(k_CropImageGeomHandle, args);
  }

  // Assign new value(s)
  if(false)
  {
    std::vector<DataPath> cellArrays = {k_DataArrayPath};
    std::vector<uint64> minPoint{0, 0, 0};
    std::vector<uint64> maxPoint{1, 2, 0};
    float64 target_value = 8.0;
    std::vector<float64> value_range{0.0, 10.0};
    uint64 init_type = 0;

    Arguments args;
    args.insert("cell_arrays", cellArrays);
    args.insert("image_geom_path", k_ImageGeomPath);
    args.insert("min_point", minPoint);
    args.insert("max_point", maxPoint);
    args.insert("init_type", init_type);
    args.insert("init_value", target_value);
    args.insert("init_range", value_range);
    pipeline.push_back(k_InitializeDataHandle, args);
  }

  return pipeline;
}

TEST_CASE("Out-of-Core Pipeline", "Out-of-Core")
{
  Application app;
  app.loadPlugins(unit_test::k_BuildDir.view());
  auto filterList = app.getFilterList();

  auto pipeline = createPipeline();
  REQUIRE(pipeline.execute());

  auto dataStructure = pipeline.getDataStructure();
  auto* dataArrayPtr = dataStructure.getDataAs<Int32Array>(k_DataArrayPath);
  REQUIRE(dataArrayPtr != nullptr);

  auto& dataArray = *dataArrayPtr;
  auto count = dataArray.getSize();
  for(usize i = 0; i < count; i++)
  {
    REQUIRE(dataArray[i] == 4);
    // REQUIRE(dataArray[i] == 8);
  }
}
