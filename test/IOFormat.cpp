#include <catch2/catch.hpp>

#include "complex/Core/Application.hpp"
#include "complex/DataStructure/IO/Generic/DataIOCollection.hpp"

using namespace complex;

TEST_CASE("Contains HDF5 IO Support", "IOTest")
{
  Application app;

  auto ioCollection = app.getIOCollection();
  auto h5IO = ioCollection->getManager("HDF5");
  REQUIRE(h5IO != nullptr);
}
