#include "ImportVolumeGraphicsFile.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"

using namespace complex;
namespace fs = std::filesystem;

namespace
{
inline constexpr int32 k_VolBinaryAllocateMismatch = -91504;
inline constexpr int32 k_VolOpenError = -91505;
inline constexpr int32 k_VolReadError = -91506;
} // namespace

// -----------------------------------------------------------------------------
ImportVolumeGraphicsFile::ImportVolumeGraphicsFile(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                   ImportVolumeGraphicsFileInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ImportVolumeGraphicsFile::~ImportVolumeGraphicsFile() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ImportVolumeGraphicsFile::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ImportVolumeGraphicsFile::operator()()
{
  const ImageGeom& image = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);

  DataPath densityArrayPath = m_InputValues->ImageGeometryPath.createChildPath(m_InputValues->CellAttributeMatrixName).createChildPath(m_InputValues->DensityArrayName);
  Float32Array& density = m_DataStructure.getDataRefAs<Float32Array>(densityArrayPath);

  usize filesize = static_cast<usize>(fs::file_size(m_InputValues->VGDataFile));
  usize allocatedBytes = density.getSize() * sizeof(float32);

  if(filesize < allocatedBytes)
  {
    return {MakeErrorResult(k_VolBinaryAllocateMismatch, fmt::format("Binary file size ({}) is smaller than the number of allocated bytes ({}).", filesize, allocatedBytes))};
  }

  FILE* f = fopen(m_InputValues->VGDataFile.c_str(), "rb");
  if(nullptr == f)
  {
    return {MakeErrorResult(k_VolOpenError, fmt::format("Error opening binary input file: {}"))};
  }

  m_MessageHandler(IFilter::Message::Type::Info, "Reading Data from .vol File.....");
  std::byte* chunkptr = reinterpret_cast<std::byte*>(density.template getIDataStoreAs<DataStore<float32>>()->data());
  usize bytesRead = fread(chunkptr, sizeof(std::byte), filesize, f);
  if(bytesRead != filesize)
  {
    std::fclose(f);
    return {MakeErrorResult(k_VolReadError, fmt::format("Error Reading .vol file. The file size is {}, but only {} bytes were read.", filesize, bytesRead))};
  }

  std::fclose(f);
  return {};
}
