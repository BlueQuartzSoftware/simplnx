#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/IO/Generic/InMemoryFormatResolver.hpp"
#include "simplnx/Utilities/ArrayCreationUtilities.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;

TEST_CASE("InMemoryFormatResolver always returns in-memory", "[Core][FormatResolver]")
{
  DataStructure ds;
  const InMemoryFormatResolver resolver;
  // Direct (concrete-type) dispatch: always in-core, even for a 1 TB array.
  REQUIRE(resolver.resolveFormat(ds, DataPath({"Foo"}), DataType::float32, 1ULL << 40).empty());
  // Through the interface pointer (the way it is actually used): virtual dispatch still yields in-core.
  const IDataStoreFormatResolver& asBase = resolver;
  REQUIRE(asBase.resolveFormat(ds, DataPath({"Foo"}), DataType::float32, 1ULL << 40).empty());
  // Unknown size (e.g. an unpopulated NeighborList) is still in-core.
  REQUIRE(asBase.resolveFormat(ds, DataPath({"Foo"}), DataType::int32, 0).empty());
}

namespace
{
// Test resolver that reports a fixed sentinel format so we can observe which resolver is consulted.
class FixedResolver : public nx::core::IDataStoreFormatResolver
{
public:
  explicit FixedResolver(std::string fmt)
  : m_Fmt(std::move(fmt))
  {
  }
  std::string resolveFormat(const nx::core::DataStructure&, const nx::core::DataPath&, nx::core::DataType, nx::core::uint64) const override
  {
    return m_Fmt;
  }

private:
  std::string m_Fmt;
};
} // namespace

TEST_CASE("DataStructure resolver: per-instance overrides process default; isolation", "[Core][FormatResolver]")
{
  // NOTE: setDefaultFormatResolver is process-global; this test relies on Catch2's default sequential execution.
  DataStructure::setDefaultFormatResolver(std::make_shared<FixedResolver>("DEFAULT"));

  DataStructure a; // no per-instance resolver -> process default
  DataStructure b;
  b.setFormatResolver(std::make_shared<FixedResolver>("PER-B"));

  const DataPath p({"x"});
  REQUIRE(a.formatResolver().resolveFormat(a, p, DataType::int32, 0) == "DEFAULT");
  REQUIRE(b.formatResolver().resolveFormat(b, p, DataType::int32, 0) == "PER-B");

  // A copy of b shares b's resolver (policy carries on copy).
  DataStructure bCopy = b;
  REQUIRE(bCopy.formatResolver().resolveFormat(bCopy, p, DataType::int32, 0) == "PER-B");

  // Restore the in-memory default so later tests are unaffected.
  DataStructure::setDefaultFormatResolver(std::make_shared<InMemoryFormatResolver>());
}

TEST_CASE("ResolveStorageFormat consults the resolver for every geometry parent", "[Core][FormatResolver]")
{
  DataStructure ds;
  ds.setFormatResolver(std::make_shared<FixedResolver>("RESOLVED"));

  auto* image = ImageGeom::Create(ds, "Image");
  AttributeMatrix::Create(ds, "CellData", ShapeType{10}, image->getId());
  const DataPath imgArrayPath({"Image", "CellData", "arr"});

  auto* tri = TriangleGeom::Create(ds, "Tri");
  AttributeMatrix::Create(ds, "VertexData", ShapeType{5}, tri->getId());
  const DataPath triArrayPath({"Tri", "VertexData", "arr"});

  DataGroup::Create(ds, "Group");
  const DataPath grpArrayPath({"Group", "arr"});

  auto* rectGrid = RectGridGeom::Create(ds, "RectGrid");
  AttributeMatrix::Create(ds, "CellData2", ShapeType{8}, rectGrid->getId());
  const DataPath rectArrayPath({"RectGrid", "CellData2", "arr"});

  for(const DataPath& path : {imgArrayPath, triArrayPath, grpArrayPath, rectArrayPath})
  {
    CHECK(ArrayCreationUtilities::ResolveStorageFormat(ds, path, DataType::float32, 40, "") == "RESOLVED");
    CHECK(ArrayCreationUtilities::ResolveStorageFormat(ds, path, DataType::float32, 40, "EXPLICIT") == "EXPLICIT");
  }
}
