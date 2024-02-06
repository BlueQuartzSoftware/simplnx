#include "SimplnxCore/Filters/ReadPeregrineHDF5FileFilter.hpp"
#include "SimplnxCore/SimplnxCore_test_dirs.hpp"

#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/ReadHDF5DatasetParameter.hpp"
#include "simplnx/UnitTest/UnitTestCommon.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"
#include "simplnx/Utilities/Parsing/HDF5/Writers/FileWriter.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <catch2/catch.hpp>

#include <functional>

namespace fs = std::filesystem;
using namespace nx::core;

// -----------------------------------------------------------------------------
TEST_CASE("SimplnxCore::ReadPeregrineHDF5File Filter")
{
  //  {
  //    writeHDF5File();
  //
  //    ReadPeregrineHDF5FileFilter filter;
  //    testFilterPreflight(filter);
  //    testFilterExecute(filter);
  //  }
  //
  //  if(fs::exists(m_FilePath))
  //  {
  //    if(!fs::remove(m_FilePath))
  //    {
  //      REQUIRE(0 == 1);
  //    }
  //  }
}
