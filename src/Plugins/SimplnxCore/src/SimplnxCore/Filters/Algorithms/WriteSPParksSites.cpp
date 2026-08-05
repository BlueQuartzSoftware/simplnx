#include "WriteSPParksSites.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <fstream>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{

Result<> WriteHeader(const DataStructure& dataStructure, const WriteSPParksSitesInputValues* inputValues, std::ofstream& outfile)
{
  SizeVec3 dims = dataStructure.getDataAs<ImageGeom>(inputValues->ImageGeomPath)->getDimensions();
  auto& featureIds = dataStructure.getDataAs<Int32Array>(inputValues->FeatureIdsArrayPath)->getDataStoreRef();

  const size_t totalPoints = featureIds.getNumberOfTuples();

  outfile << "-" << "\n";
  outfile << "3 dimension" << "\n";
  outfile << totalPoints << " sites" << "\n";
  outfile << "26 max neighbors" << "\n";
  outfile << "0 " << dims[0] << " xlo xhi" << "\n";
  outfile << "0 " << dims[1] << " ylo yhi" << "\n";
  outfile << "0 " << dims[2] << " zlo zhi" << "\n";
  outfile << "\n";
  outfile << "Values" << "\n";
  outfile << "\n";

  return {};
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
Result<> WriteFile(const DataStructure& dataStructure, const WriteSPParksSitesInputValues* inputValues, std::ofstream& outfile, const IFilter::MessageHandler& messageHandler,
                   const std::atomic_bool& shouldCancel)
{
  SizeVec3 dims = dataStructure.getDataAs<ImageGeom>(inputValues->ImageGeomPath)->getDimensions();
  auto& featureIds = dataStructure.getDataAs<Int32Array>(inputValues->FeatureIdsArrayPath)->getDataStoreRef();

  size_t totalpoints = featureIds.getNumberOfTuples();

  auto start = std::chrono::steady_clock::now();

  for(size_t k = 0; k < totalpoints; k++)
  {
    auto now = std::chrono::steady_clock::now();
    // Only send updates every 1 second
    if(std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() > 1000)
    {
      const int32 progInt = static_cast<int32>((static_cast<float32>(k) / totalpoints) * 100.0f);
      messageHandler.sendProgressMessage("Writing File", progInt);
      start = std::chrono::steady_clock::now();
    }
    if(shouldCancel)
    {
      return {};
    }
    outfile << k + 1 << " " << featureIds[k] << "\n";
  }

  return {};
}

} // namespace

// -----------------------------------------------------------------------------
WriteSPParksSites::WriteSPParksSites(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WriteSPParksSitesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
WriteSPParksSites::~WriteSPParksSites() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& WriteSPParksSites::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> WriteSPParksSites::operator()()
{
  // Make sure any directory path is also available as the user may have just typed
  // in a path without actually creating the full path
  Result<> createDirectoriesResult = nx::core::CreateOutputDirectories(m_InputValues->OutputFile.parent_path());
  if(createDirectoriesResult.invalid())
  {
    return createDirectoriesResult;
  }

  std::ofstream file = std::ofstream(m_InputValues->OutputFile, std::ios_base::out | std::ios_base::binary);
  if(!file.is_open())
  {
    return MakeErrorResult(-77450, fmt::format("Error creating and opening output file at path: {}", m_InputValues->OutputFile.string()));
  }

  Result<> result = WriteHeader(m_DataStructure, m_InputValues, file);
  if(m_ShouldCancel)
  {
    return {};
  }
  if(result.invalid())
  {
    return MakeErrorResult(-77451, fmt::format("Error writing header to file: {}", m_InputValues->OutputFile.string()));
  }

  result = WriteFile(m_DataStructure, m_InputValues, file, m_MessageHandler, m_ShouldCancel);
  if(m_ShouldCancel)
  {
    return {};
  }
  if(result.invalid())
  {
    return MakeErrorResult(-77452, fmt::format("Error writing body of file: {}", m_InputValues->OutputFile.string()));
  }

  file.flush();
  file.close();

  return result;
}
