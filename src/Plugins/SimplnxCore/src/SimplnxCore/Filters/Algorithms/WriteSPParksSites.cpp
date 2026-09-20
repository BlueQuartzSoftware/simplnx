#include "WriteSPParksSites.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <fstream>
#include <memory>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{

/**
 * @brief Writes the SPPARKS geometry and site-count header.
 * @param dataStructure Provides image dimensions and Feature-ID tuple count.
 * @param inputValues Specifies source paths.
 * @param outfile Receives formatted header text.
 * @return Success. Stream status is not inspected.
 */
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

/**
 * @brief Streams site IDs and Feature IDs to SPPARKS from fixed DataStore pages.
 * @param dataStructure Provides source geometry and Feature IDs.
 * @param inputValues Specifies source paths.
 * @param outfile Receives formatted site lines.
 * @param messageHandler Receives periodic progress.
 * @param shouldCancel Stops before later site lines when true.
 * @return Feature-ID read error, or success after completion or cancellation.
 *
 * Only formatting remains per-site; the potentially disk-backed Feature-ID
 * source is loaded sequentially in approximately 1 MiB buffers. Stream status is not inspected.
 */
Result<> WriteFile(const DataStructure& dataStructure, const WriteSPParksSitesInputValues* inputValues, std::ofstream& outfile, const IFilter::MessageHandler& messageHandler,
                   const std::atomic_bool& shouldCancel)
{
  SizeVec3 dims = dataStructure.getDataAs<ImageGeom>(inputValues->ImageGeomPath)->getDimensions();
  auto& featureIds = dataStructure.getDataAs<Int32Array>(inputValues->FeatureIdsArrayPath)->getDataStoreRef();

  size_t totalpoints = featureIds.getNumberOfTuples();

  auto start = std::chrono::steady_clock::now();

  constexpr usize k_TargetBufferBytes = 1024 * 1024;
  const usize bufferElements = std::max<usize>(1, std::min(totalpoints, k_TargetBufferBytes / sizeof(int32)));
  auto featureIdBuffer = std::make_unique<int32[]>(bufferElements);

  for(usize offset = 0; offset < totalpoints; offset += bufferElements)
  {
    const usize count = std::min(bufferElements, totalpoints - offset);
    Result<> readResult = featureIds.copyIntoBuffer(offset, nonstd::span<int32>(featureIdBuffer.get(), count));
    if(readResult.invalid())
    {
      return readResult;
    }

    for(usize localIndex = 0; localIndex < count; localIndex++)
    {
      const usize pointIndex = offset + localIndex;
      auto now = std::chrono::steady_clock::now();
      if(std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() > 1000)
      {
        const int32 progInt = static_cast<int32>((static_cast<float32>(pointIndex) / totalpoints) * 100.0f);
        std::string message = fmt::format("Writing File {}%", progInt);
        messageHandler(nx::core::IFilter::ProgressMessage{nx::core::IFilter::Message::Type::Info, message, progInt});
        start = std::chrono::steady_clock::now();
      }
      if(shouldCancel)
      {
        return {};
      }
      outfile << pointIndex + 1 << " " << featureIdBuffer[localIndex] << "\n";
    }
  }

  return {};
}

} // namespace

WriteSPParksSites::WriteSPParksSites(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WriteSPParksSitesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

WriteSPParksSites::~WriteSPParksSites() noexcept = default;

const std::atomic_bool& WriteSPParksSites::getCancel()
{
  return m_ShouldCancel;
}

Result<> WriteSPParksSites::operator()()
{
  // Create parent directories before opening the requested output path.
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
