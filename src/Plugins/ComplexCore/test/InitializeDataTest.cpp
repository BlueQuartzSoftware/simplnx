#include "ComplexCore/Filters/InitializeData.hpp"
#include "ComplexCore/ComplexCore_test_dirs.hpp"

#include "complex/Common/TypeTraits.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/UnitTest/UnitTestCommon.hpp"

#include <catch2/catch.hpp>

#include <stdexcept>

using namespace complex;

namespace
{
} // namespace

TEST_CASE("ComplexCore::InitializeData Single Component Value Initialization", "[ComplexCore][InitializeData]")
{
  InitializeData filter;
}

TEST_CASE("ComplexCore::InitializeData Single Component Random Initialization", "[ComplexCore][InitializeData]")
{
  InitializeData filter;
}

TEST_CASE("ComplexCore::InitializeData Single Component Random With Range Initialization", "[ComplexCore][InitializeData]")
{
  InitializeData filter;
}
