#include "simplnx/Filter/Actions/RenameDataAction.hpp"

#include <catch2/catch.hpp>

using namespace nx::core;

TEST_CASE("simplnx::RenameDataAction: clone keeps every setting", "[simplnx][RenameDataAction]")
{
  const DataPath sourcePath({"Group", "Source"});
  const bool overwrite = GENERATE(true, false);
  CAPTURE(overwrite);
  const RenameDataAction action(sourcePath, "Renamed", overwrite);
  const auto clonePtr = action.clone();
  const auto* clone = dynamic_cast<const RenameDataAction*>(clonePtr.get());
  REQUIRE(clone != nullptr);
  CHECK(clone->path() == sourcePath);
  CHECK(clone->newName() == "Renamed");
  CHECK(clone->overwrite() == overwrite);
}
