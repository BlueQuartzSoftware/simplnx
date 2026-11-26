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

void RequireBadReaderMetadata(const IFilter& filter, const Arguments& args, const std::filesystem::path& file, bool spacing);

inline void RequireReaderGeometry(const DataStructure& dataStructure, const DataPath& path, const FloatVec3& origin, const FloatVec3& spacing)
{
  const auto* geometryPtr = dataStructure.getDataAs<ImageGeom>(path);
  REQUIRE(geometryPtr != nullptr);
  CHECK(geometryPtr->getOrigin() == origin);
  CHECK(geometryPtr->getSpacing() == spacing);
}

inline void RequireReaderGeometryExecution(const IFilter& filter, const Arguments& args, const DataPath& path, const FloatVec3& origin, const FloatVec3& spacing)
{
  DataStructure dataStructure;
  const auto result = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(result.result);
  RequireReaderGeometry(dataStructure, path, origin, spacing);
}
} // namespace nx::core::UnitTest
