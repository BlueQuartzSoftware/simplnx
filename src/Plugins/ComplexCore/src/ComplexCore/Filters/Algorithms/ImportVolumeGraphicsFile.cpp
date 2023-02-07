#include "ImportVolumeGraphicsFile.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"

using namespace complex;
namespace fs = std::filesystem;

namespace
{
inline const int32 k_VolBinaryAllocateMismatch = -91504;
} // namespace

// -----------------------------------------------------------------------------
ImportVolumeGraphicsFile::ImportVolumeGraphicsFile(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel,
                                                   ImportVolumeGraphicsFileInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(msgHandler)
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
  const DataPath densityArrayPath = m_InputValues->ImageGeometryPath.createChildPath(m_InputValues->CellAttributeMatrixName).createChildPath(m_InputValues->DensityArrayName);
  auto& density = m_DataStructure.getDataRefAs<Float32Array>(densityArrayPath);

  auto filesize = static_cast<usize>(fs::file_size(m_InputValues->VGDataFile));
  const usize allocatedBytes = density.getSize() * sizeof(float32);

  if(filesize < allocatedBytes)
  {
    return {MakeErrorResult(k_VolBinaryAllocateMismatch, fmt::format("Binary file size ({}) is smaller than the number of allocated bytes ({}).", filesize, allocatedBytes))};
  }

  m_MessageHandler(IFilter::Message::Type::Info, "Reading Data from .vol File.....");
  return ImportFromBinaryFile(m_InputValues->VGDataFile, density);
}
