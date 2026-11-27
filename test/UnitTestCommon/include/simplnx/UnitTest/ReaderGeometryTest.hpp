#pragma once

#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/ImageIO/ReaderGeometryOverrides.hpp"

namespace nx::core::unit_test
{
inline void SetReaderGeometryOverrides(Arguments& args, const FloatVec3Type& origin, const FloatVec3Type& spacing)
{
  args.insertOrAssign(image_io::k_ChangeOrigin, true);
  args.insertOrAssign(image_io::k_Origin, origin.toContainer<std::vector<float32>>());
  args.insertOrAssign(image_io::k_ChangeSpacing, true);
  args.insertOrAssign(image_io::k_Spacing, spacing.toContainer<std::vector<float32>>());
}

void RequireBadReaderMetadata(const IFilter& filter, const Arguments& args, const std::filesystem::path& file, bool spacing);

inline void RequireReaderGeometry(const DataStructure& dataStructure, const DataPath& path, const FloatVec3Type& origin, const FloatVec3Type& spacing)
{
  const auto* geometryPtr = dataStructure.getDataAs<ImageGeom>(path);
  REQUIRE(geometryPtr != nullptr);
  CHECK(geometryPtr->getOrigin() == origin);
  CHECK(geometryPtr->getSpacing() == spacing);
}

inline void RequireReaderGeometryExecution(const IFilter& filter, const Arguments& args, const DataPath& path, const FloatVec3Type& origin, const FloatVec3Type& spacing)
{
  DataStructure dataStructure;
  const auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  RequireReaderGeometry(dataStructure, path, origin, spacing);
}
} // namespace nx::core::unit_test
