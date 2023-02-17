#include "ImportBinaryCTNorthstar.hpp"

#include "complex/Common/ScopeGuard.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"

#include "ComplexCore/Filters/ImportBinaryCTNorthstarFilter.hpp"

using namespace complex;

namespace
{
#if defined(_MSC_VER)
#define FSEEK64 _fseeki64
#else
#define FSEEK64 std::fseek
#endif

// -----------------------------------------------------------------------------
Result<> SanityCheckFileSizeVersusAllocatedSize(size_t allocatedBytes, size_t fileSize)
{
  if(fileSize < allocatedBytes)
  {
    return MakeErrorResult(-4000, fmt::format("File size ({} bytes) is less than allocated size ({} bytes).", fileSize, allocatedBytes));
  }

  // File Size and Allocated Size are equal so we  are good to go
  return {};
}

// -----------------------------------------------------------------------------
Result<> ReadBinaryCTFiles(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel, const ImportBinaryCTNorthstarInputValues* inputValues)
{
  auto& density = dataStructure.getDataRefAs<Float32Array>(inputValues->DensityArrayPath);
  density.fill(0xABCDEF);

  usize deltaX = inputValues->EndVoxelCoord[0] - inputValues->StartVoxelCoord[0] + 1;

  usize zShift = 0;
  int32 fileIndex = 1;

  for(const auto& dataFileInput : inputValues->DataFilePaths)
  {
    fs::path dataFilePath = inputValues->InputHeaderFile.parent_path() / dataFileInput.first;
    const usize fileSize = fs::file_size(dataFilePath);
    // allocated bytes should be the x * y dims * number of slices in the current data file....not necessarily the size of the whole density array
    usize allocatedBytes = inputValues->OriginalGeometryDims[0] * inputValues->OriginalGeometryDims[1] * dataFileInput.second * sizeof(float32);

    Result<> result = SanityCheckFileSizeVersusAllocatedSize(allocatedBytes, fileSize);
    if(result.invalid())
    {
      return MakeErrorResult(-38705, fmt::format("The size of file '{}' on the file system ({} bytes) is less than the stated size in the binary CT header ''. ({} bytes).", dataFilePath.string(),
                                                 fileSize, inputValues->InputHeaderFile.string(), allocatedBytes));
    }

    FILE* f = fopen(dataFilePath.string().c_str(), "rb");
    if(nullptr == f)
    {
      return MakeErrorResult(-38706, fmt::format("Error opening binary input file: {}.", dataFilePath.string()));
    }

    auto fileGuard = MakeScopeGuard([f]() noexcept { fclose(f); });

    usize fileZSlice = 0;
    usize index = 0;

    // Now start reading the data in chunks if needed.
    std::vector<float32> buffer(deltaX);

    for(usize z = zShift; z < (zShift + dataFileInput.second); z++)
    {
      if(inputValues->ImportSubvolume && (z < inputValues->StartVoxelCoord[2] || z > inputValues->EndVoxelCoord[2]))
      {
        fileZSlice++;
        continue;
      }
      messageHandler(fmt::format("Importing Data || Data File: {} || Importing Slice {}", dataFileInput.first.string(), z));
      for(usize y = 0; y < inputValues->OriginalGeometryDims[1]; y++)
      {
        if(inputValues->ImportSubvolume && (y < inputValues->StartVoxelCoord[1] || y > inputValues->EndVoxelCoord[1]))
        {
          continue;
        }

        usize fpOffset = ((inputValues->OriginalGeometryDims[1] * inputValues->OriginalGeometryDims[0] * fileZSlice) + (inputValues->OriginalGeometryDims[0] * y) + inputValues->StartVoxelCoord[0]) *
                         sizeof(float32);
        if(FSEEK64(f, static_cast<int32>(fpOffset), SEEK_SET) != 0)
        {
          return MakeErrorResult(-38707, fmt::format("Could not seek to position {} in file '{}'.", fpOffset, dataFileInput.first.string()));
        }

        index = (inputValues->ImportedGeometryDims[0] * inputValues->ImportedGeometryDims[1] * (z - inputValues->StartVoxelCoord[2])) +
                (inputValues->ImportedGeometryDims[0] * (y - inputValues->StartVoxelCoord[1])) + (0);
        if(fread(buffer.data(), sizeof(float32), deltaX, f) != deltaX)
        {
          return MakeErrorResult(-38708, fmt::format("Error reading file at position {} in file '{}'.", fpOffset, dataFileInput.first.string()));
        }

        for(usize i = index; i < deltaX + index; i++)
        {
          density[i] = buffer[i - index];
        }
      }
      fileZSlice++;
    }
    zShift += dataFileInput.second;
    fileIndex++;

    if(shouldCancel)
    {
      break;
    }
  }

  return {};
}
} // namespace

// -----------------------------------------------------------------------------
ImportBinaryCTNorthstar::ImportBinaryCTNorthstar(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                 ImportBinaryCTNorthstarInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ImportBinaryCTNorthstar::~ImportBinaryCTNorthstar() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ImportBinaryCTNorthstar::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ImportBinaryCTNorthstar::operator()()
{
  Result<> result = ReadBinaryCTFiles(m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
  if(result.invalid())
  {
    return result;
  }

  return {};
}
