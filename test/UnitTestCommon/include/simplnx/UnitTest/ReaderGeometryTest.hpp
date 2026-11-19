#pragma once

#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/ImageIO/ReaderGeometryOverrides.hpp"

namespace nx::core::UnitTest
{
inline void SetReaderGeometryOverrides(Arguments& args, const FloatVec3& origin, const FloatVec3& spacing)
{
  args.insertOrAssign(ImageIO::k_ChangeOrigin, true);
  args.insertOrAssign(ImageIO::k_Origin, origin.toContainer<std::vector<float32>>());
  args.insertOrAssign(ImageIO::k_ChangeSpacing, true);
  args.insertOrAssign(ImageIO::k_Spacing, spacing.toContainer<std::vector<float32>>());
}

inline void RequireBadReaderMetadata(const IFilter& filter, const Arguments& args, const std::filesystem::path& file, bool spacing)
{
  DataStructure ds;
  const auto preflight = filter.preflight(ds, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflight.outputActions);
  const auto& error = preflight.outputActions.errors().front();
  CHECK(error.code == (spacing ? ImageGeom::k_InvalidSpacing : ImageGeom::k_InvalidOrigin));
  CHECK(error.message.starts_with(fmt::format("Input file '{}' has invalid {} values. Error Message: ", file.string(), spacing ? "spacing" : "origin")));
  CHECK(error.message.ends_with(fmt::format("Enable '{}' to use your own values.", spacing ? "Set Spacing" : "Set Origin")));
}

inline void RequireReaderGeometry(const DataStructure& ds, const DataPath& path, const FloatVec3& origin, const FloatVec3& spacing)
{
  const auto* geometry = ds.getDataAs<ImageGeom>(path);
  REQUIRE(geometry != nullptr);
  CHECK(geometry->getOrigin() == origin);
  CHECK(geometry->getSpacing() == spacing);
}

inline void RequireReaderGeometryExecution(const IFilter& filter, const Arguments& args, const DataPath& path, const FloatVec3& origin, const FloatVec3& spacing)
{
  DataStructure ds;
  const auto result = filter.execute(ds, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  RequireReaderGeometry(ds, path, origin, spacing);
}
} // namespace nx::core::UnitTest
