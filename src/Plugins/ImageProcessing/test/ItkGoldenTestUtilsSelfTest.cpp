#include "ItkGoldenTestUtils.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;

TEST_CASE("ImageProcessing::ItkGoldenTestUtils self-test", "[ImageProcessing][ItkGolden]")
{
  UnitTest::LoadPlugins();
  const UnitTest::PreferencesSentinel prefsSentinel(nx::core::DataStorageMode::ForceInCore, 0);

  // (1) NRRD reader route: read RA-Short.nrrd; array + geom must materialize.
  DataStructure ds;
  const DataPath geom({"G"});
  const auto rNrrd = ip_golden::ReadInputImage(ds, ip_golden::InputPath("RA-Short.nrrd"), geom, "CellData", "Input");
  SIMPLNX_RESULT_REQUIRE_VALID(rNrrd);
  const DataPath arr = geom.createChildPath("CellData").createChildPath("Input");
  REQUIRE_NOTHROW(ds.getDataRefAs<IDataArray>(arr));

  // (2) PNG reader route: read STAPLE1.png.
  DataStructure dsPng;
  const auto rPng = ip_golden::ReadInputImage(dsPng, ip_golden::InputPath("STAPLE1.png"), DataPath({"G"}), "CellData", "Input");
  SIMPLNX_RESULT_REQUIRE_VALID(rPng);

  // (3) MHA reader route: read cthead1-Float.mha (records which MHA rung is reachable -- §5).
  DataStructure dsMha;
  const auto rMha = ip_golden::ReadInputImage(dsMha, ip_golden::InputPath("cthead1-Float.mha"), DataPath({"G"}), "CellData", "Input");
  SIMPLNX_RESULT_REQUIRE_VALID(rMha);

  // (4) md5 is stable + non-empty; self-compare at tol 0 is exact.
  const std::string h1 = ip_golden::ComputeMd5Hash(ds, arr);
  REQUIRE(h1.size() == 32);
  REQUIRE(ip_golden::ComputeMd5Hash(ds, arr) == h1);

  // (5) CompareImages of an array against itself is valid at tolerance 0.
  const auto self = ip_golden::CompareImages(ds, geom, arr, geom, arr, 0.0);
  SIMPLNX_RESULT_REQUIRE_VALID(self);

  // (6) LegacyUuidFor resolves a known port (Median -> a60ca165-...).
  const auto legacy = ip_golden::LegacyUuidFor(*Uuid::FromString("bf4671ab-ebdf-4c4c-a4b1-0cdcaa005d9d"));
  REQUIRE(legacy.has_value());
  REQUIRE(legacy.value() == *Uuid::FromString("a60ca165-59ac-486b-b4b4-0f0c24d80af8"));
}
