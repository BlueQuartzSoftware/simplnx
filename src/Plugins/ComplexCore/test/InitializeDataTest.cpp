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

TEST_CASE("ComplexCore::InitializeData(Manual)", "[ComplexCore][InitializeData]")
{
  InitializeData filter;
}

TEST_CASE("ComplexCore::InitializeData(Random)", "[ComplexCore][InitializeData]")
{
  InitializeData filter;
}

TEST_CASE("ComplexCore::InitializeData(RandomWithRange)", "[ComplexCore][InitializeData]")
{
  InitializeData filter;
}
