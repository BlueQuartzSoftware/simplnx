#include "SimplnxCore/Filters/ComputeFeatureCentroidsFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Pipeline/PipelineFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <array>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;
using namespace nx::core;
using namespace nx::core::UnitTest;

// =============================================================================
// V&V Class 1 (Analytical) + Class 4 (Invariant) oracle support — added 2026-07-07.
//
// The centroid of a feature is the arithmetic mean of the voxel-center coordinates
// (voxel-center = origin + (index + 0.5) * spacing) of every cell belonging to that
// feature. This is a closed-form Class 1 oracle: expected values are hand-derivable on
// a toy grid, independently of any DREAM3D implementation. These inline fixtures replace
// the retired consistency-with-self exemplar test (Centroids NX vs a sibling Centroids
// array in 6_6_stats_test_v2.dream3d), which was a circular oracle.
//
// Reference: src/Plugins/SimplnxCore/vv/ComputeFeatureCentroidsFilter.md
// =============================================================================

namespace CentroidToy
{
const std::string k_GeomName = "Image";
const std::string k_CellAMName = "CellData";
const std::string k_FeatureAMName = "FeatureData";
const std::string k_FeatureIdsName = "FeatureIds";
const std::string k_CentroidsName = "Centroids";

struct Scaffold
{
  DataStructure ds;
  DataPath geomPath;
  DataPath featureIdsPath;
  DataPath featureAMPath;
  DataPath centroidsPath;
};

// Build an ImageGeom + Cell Data AM (with FeatureIds) + empty Feature Data AM sized to numFeatures.
// featureIds are supplied in row-major (z, y, x) order.
inline Scaffold Build(usize dimX, usize dimY, usize dimZ, std::array<float32, 3> spacing, std::array<float32, 3> origin, usize numFeatures, const std::vector<int32>& featureIdValues)
{
  Scaffold s;
  auto* imageGeom = ImageGeom::Create(s.ds, k_GeomName);
  imageGeom->setDimensions({dimX, dimY, dimZ});
  imageGeom->setSpacing({spacing[0], spacing[1], spacing[2]});
  imageGeom->setOrigin({origin[0], origin[1], origin[2]});

  const ShapeType cellTupleShape{dimZ, dimY, dimX};
  auto* cellAM = AttributeMatrix::Create(s.ds, k_CellAMName, cellTupleShape, imageGeom->getId());
  imageGeom->setCellData(*cellAM);

  auto* featureIds = CreateTestDataArray<int32>(s.ds, k_FeatureIdsName, cellTupleShape, {1}, cellAM->getId());
  REQUIRE(featureIdValues.size() == dimX * dimY * dimZ);
  for(usize i = 0; i < featureIdValues.size(); ++i)
  {
    (*featureIds)[i] = featureIdValues[i];
  }

  AttributeMatrix::Create(s.ds, k_FeatureAMName, ShapeType{numFeatures}, imageGeom->getId());

  s.geomPath = DataPath({k_GeomName});
  s.featureIdsPath = s.geomPath.createChildPath(k_CellAMName).createChildPath(k_FeatureIdsName);
  s.featureAMPath = s.geomPath.createChildPath(k_FeatureAMName);
  s.centroidsPath = s.featureAMPath.createChildPath(k_CentroidsName);
  return s;
}

// Run the filter and return the flat [numFeatures * 3] Centroids values.
inline std::vector<float32> Run(Scaffold& s, bool isPeriodic)
{
  ComputeFeatureCentroidsFilter filter;
  Arguments args;
  args.insertOrAssign(ComputeFeatureCentroidsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(s.geomPath));
  args.insertOrAssign(ComputeFeatureCentroidsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(s.featureIdsPath));
  args.insertOrAssign(ComputeFeatureCentroidsFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(s.featureAMPath));
  args.insertOrAssign(ComputeFeatureCentroidsFilter::k_CentroidsArrayName_Key, std::make_any<std::string>(k_CentroidsName));
  args.insertOrAssign(ComputeFeatureCentroidsFilter::k_IsPeriodic_Key, std::make_any<bool>(isPeriodic));

  auto preflightResult = filter.preflight(s.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions)

  auto executeResult = filter.execute(s.ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result)

  REQUIRE_NOTHROW(s.ds.getDataRefAs<Float32Array>(s.centroidsPath));
  const auto& centroids = s.ds.getDataRefAs<Float32Array>(s.centroidsPath);
  std::vector<float32> out(centroids.getSize());
  for(usize i = 0; i < centroids.getSize(); ++i)
  {
    out[i] = centroids[i];
  }
  return out;
}

inline void RequireCentroid(const std::vector<float32>& c, usize featureId, float32 x, float32 y, float32 z, float32 margin = 1.0e-4f)
{
  REQUIRE(c[featureId * 3 + 0] == Approx(x).margin(margin));
  REQUIRE(c[featureId * 3 + 1] == Approx(y).margin(margin));
  REQUIRE(c[featureId * 3 + 2] == Approx(z).margin(margin));
}
} // namespace CentroidToy

TEST_CASE("SimplnxCore::ComputeFeatureCentroidsFilter: Class 1 - Analytical Centroids", "[SimplnxCore][ComputeFeatureCentroidsFilter]")
{
  using namespace CentroidToy;
  UnitTest::LoadPlugins();

  SECTION("Fixture A - single multi-cell feature + empty background")
  {
    // 3x1x1, spacing 1, origin 0; FeatureIds [1,1,1]; 2 features (0 empty, 1 present)
    auto s = Build(3, 1, 1, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, 2, {1, 1, 1});
    auto c = Run(s, false);
    RequireCentroid(c, 0, 0.0f, 0.0f, 0.0f); // count==0 -> stays (0,0,0)
    RequireCentroid(c, 1, 1.5f, 0.5f, 0.5f); // x=(0.5+1.5+2.5)/3
  }

  SECTION("Fixture B - multi-feature 2D (averaging, single-cell, empty id)")
  {
    // 4x2x1; row y0 [1,1,2,2], row y1 [1,3,2,2]; 4 features
    auto s = Build(4, 2, 1, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, 4, {1, 1, 2, 2, 1, 3, 2, 2});
    auto c = Run(s, false);
    RequireCentroid(c, 0, 0.0f, 0.0f, 0.0f);               // empty
    RequireCentroid(c, 1, 5.0f / 6.0f, 5.0f / 6.0f, 0.5f); // (0.8333.., 0.8333.., 0.5)
    RequireCentroid(c, 2, 3.0f, 1.0f, 0.5f);
    RequireCentroid(c, 3, 1.5f, 1.5f, 0.5f); // single cell (1,1)
  }

  SECTION("Fixture C - 3D volume (z-stride correctness)")
  {
    // 2x2x2; z0 plane all fid1, z1 plane all fid2; 3 features
    auto s = Build(2, 2, 2, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, 3, {1, 1, 1, 1, 2, 2, 2, 2});
    auto c = Run(s, false);
    RequireCentroid(c, 0, 0.0f, 0.0f, 0.0f);
    RequireCentroid(c, 1, 1.0f, 1.0f, 0.5f); // z index 0
    RequireCentroid(c, 2, 1.0f, 1.0f, 1.5f); // z index 1
  }

  UnitTest::CheckArraysInheritTupleDims(Build(3, 1, 1, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, 2, {1, 1, 1}).ds);
}

TEST_CASE("SimplnxCore::ComputeFeatureCentroidsFilter: Class 1/4 - Periodic Boundary", "[SimplnxCore][ComputeFeatureCentroidsFilter][Periodic]")
{
  using namespace CentroidToy;
  UnitTest::LoadPlugins();

  SECTION("Fixture D - periodic wrap, unit spacing")
  {
    // 4x1x1; FeatureIds [1,2,1,1]; feature 1 spans x=0 and x=3 (full extent), feature 2 (x=1) does not.
    // feature 1 cells x=0,2,3 -> centers 0.5,2.5,3.5 -> mean 6.5/3 = 2.16667
    auto sNP = Build(4, 1, 1, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, 3, {1, 2, 1, 1});
    auto nonPeriodic = Run(sNP, false);
    RequireCentroid(nonPeriodic, 1, 6.5f / 3.0f, 0.5f, 0.5f);
    RequireCentroid(nonPeriodic, 2, 1.5f, 0.5f, 0.5f); // single cell x=1

    auto sP = Build(4, 1, 1, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, 3, {1, 2, 1, 1});
    auto periodic = Run(sP, true);
    // Class 1 (given the DREAM3D periodic-shift model): spanning feature 1 gets +(dim-1)/2 = +1.5.
    RequireCentroid(periodic, 1, 6.5f / 3.0f + 1.5f, 0.5f, 0.5f);
    // Class 4 invariant: the non-spanning feature 2 must be unchanged by the periodic pass.
    RequireCentroid(periodic, 2, 1.5f, 0.5f, 0.5f);
  }

  SECTION("Fixture E - periodic wrap, NON-unit spacing (regression pin for the spacing fix)")
  {
    // 4x1x1, spacing (2,1,1), origin (10,0,0); FeatureIds [1,2,2,1]
    // x-centers: idx0 -> 11.0, idx3 -> 17.0; non-periodic centroid.x = 14.0
    auto sNP = Build(4, 1, 1, {2.0f, 1.0f, 1.0f}, {10.0f, 0.0f, 0.0f}, 3, {1, 2, 2, 1});
    auto nonPeriodic = Run(sNP, false);
    RequireCentroid(nonPeriodic, 1, 14.0f, 0.5f, 0.5f); // unambiguous analytical value

    auto sP = Build(4, 1, 1, {2.0f, 1.0f, 1.0f}, {10.0f, 0.0f, 0.0f}, 3, {1, 2, 2, 1});
    auto periodic = Run(sP, true);
    // The periodic offset must scale with spacing: (dim-1)*spacing_x/2 = 3*2/2 = 3.0 -> 14.0 + 3.0 = 17.0.
    // Before the fix this returned 15.5 (offset 1.5 in cell units, ignoring spacing). Regression pin for
    // the AdjustCentroidsForPeriodicFaces spacing fix. See vv/ComputeFeatureCentroidsFilter.md Phase 6.
    RequireCentroid(periodic, 1, 17.0f, 0.5f, 0.5f);
  }
}

TEST_CASE("SimplnxCore::ComputeFeatureCentroidsFilter: Error - FeatureId exceeds Feature AM", "[SimplnxCore][ComputeFeatureCentroidsFilter]")
{
  using namespace CentroidToy;
  UnitTest::LoadPlugins();

  // 2x1x1 with FeatureIds {0, 5} but only 2 feature tuples -> id 5 >= numFeatures -> error -5351.
  auto s = Build(2, 1, 1, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, 2, {0, 5});

  ComputeFeatureCentroidsFilter filter;
  Arguments args;
  args.insertOrAssign(ComputeFeatureCentroidsFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(s.geomPath));
  args.insertOrAssign(ComputeFeatureCentroidsFilter::k_CellFeatureIdsArrayPath_Key, std::make_any<DataPath>(s.featureIdsPath));
  args.insertOrAssign(ComputeFeatureCentroidsFilter::k_FeatureAttributeMatrixPath_Key, std::make_any<DataPath>(s.featureAMPath));
  args.insertOrAssign(ComputeFeatureCentroidsFilter::k_CentroidsArrayName_Key, std::make_any<std::string>(k_CentroidsName));
  args.insertOrAssign(ComputeFeatureCentroidsFilter::k_IsPeriodic_Key, std::make_any<bool>(false));

  auto executeResult = filter.execute(s.ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(executeResult.result)
}

TEST_CASE("SimplnxCore::ComputeFeatureCentroidsFilter: SIMPL Backwards Compatibility", "[SimplnxCore][ComputeFeatureCentroidsFilter][BackwardsCompatibility]")
{
  auto app = Application::GetOrCreateInstance();
  UnitTest::LoadPlugins();
  auto filterList = app->getFilterList();

  const fs::path conversionDir = fs::path(nx::core::unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";

  const std::vector<std::pair<std::string, fs::path>> fixtures = {
      {"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "ComputeFeatureCentroidsFilter.json"},
      {"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "ComputeFeatureCentroidsFilter.json"},
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
      REQUIRE(filter->uuid() == FilterTraits<ComputeFeatureCentroidsFilter>::uuid);

      CHECK(pipelineFilter->getComments().empty());

      const Arguments args = pipelineFilter->getArguments();
      CHECK(args.value<DataPath>(ComputeFeatureCentroidsFilter::k_SelectedImageGeometryPath_Key) == DataPath({"DataContainer"}));
      CHECK(args.value<DataPath>(ComputeFeatureCentroidsFilter::k_CellFeatureIdsArrayPath_Key) == DataPath({"DataContainer", "CellData", "TestArray"}));
      // Complex type (DataArrayCreationToAMFilterParameterConverter) - verified by successful pipeline loading
      CHECK(args.value<std::string>(ComputeFeatureCentroidsFilter::k_CentroidsArrayName_Key) == "TestArray");
    }
  }
}
