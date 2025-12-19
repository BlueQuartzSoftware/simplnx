#include "ReadNotesFile.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/StringArray.hpp"

#include <filesystem>
#include <vector>

namespace fs = std::filesystem;
using namespace nx::core;

// -----------------------------------------------------------------------------
ReadNotesFile::ReadNotesFile(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadNotesFileInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ReadNotesFile::~ReadNotesFile() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ReadNotesFile::operator()()
{
  std::vector<uint8_t> buffer;

  // Read the file into a uint8 vector
  try
  {
    // Check if the path points to a regular file
    if(fs::is_regular_file(m_InputValues->InputFilePath))
    {
      // Get the size of the file in bytes
      usize size_in_bytes = fs::file_size(m_InputValues->InputFilePath);

      // 1. Open the file in binary mode
      // std::ios::ate seeks to the end of the file immediately after opening
      std::ifstream input_file(m_InputValues->InputFilePath, std::ios_base::in | std::ios_base::binary);

      // 2. Check if the file opened successfully
      if(!input_file.is_open())
      {
        std::cerr << "Error: Unable to open file " << m_InputValues->InputFilePath << std::endl;
        return {MakeErrorResult(-38202, fmt::format("Could not open input file '{}'", m_InputValues->InputFilePath.string()))};
      }

      // 5. Resize the vector to the file size to allocate the required storage
      buffer.resize(size_in_bytes);

      // 6. Read the entire file content into the vector's underlying data array
      // reinterpret_cast is used to treat uint8_t* as char* for the read function
      input_file.read(reinterpret_cast<char*>(buffer.data()), size_in_bytes);

      // 7. Close the file (optional, as the ifstream destructor will do this)
      input_file.close();
    }
    else
    {
      return {MakeErrorResult<>(-38200, fmt::format("The path '{}' does not point to a regular file, or the file does not exist.", m_InputValues->InputFilePath.string()))};
    }
  } catch(fs::filesystem_error const& ex)
  {
    // Handle potential errors (e.g., file not found, permission denied)
    return {MakeErrorResult<>(-38201, fmt::format("Filesystem error for file '{}'\nOperating System returned error '{}'", m_InputValues->InputFilePath.string(), ex.what()))};
  }

  auto& outputArrayRef = m_DataStructure.getDataRefAs<StringArray>(m_InputValues->CreatedDataPath);
  std::string bufAsString(buffer.begin(), buffer.end());
  outputArrayRef.setValue(0, bufAsString);

  return {};
}
