#include "ReadVolumeGraphicsFile.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
const int32 k_VolBinaryAllocateMismatch = -91504;
} // namespace

// -----------------------------------------------------------------------------
ReadVolumeGraphicsFile::ReadVolumeGraphicsFile(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel,
                                               ReadVolumeGraphicsFileInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(msgHandler)
{
}

// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
const std::atomic_bool& ReadVolumeGraphicsFile::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ReadVolumeGraphicsFile::operator()()
{
  const DataPath densityArrayPath = m_InputValues->image_geometry_path.createChildPath(m_InputValues->cell_attribute_matrix_name).createChildPath(m_InputValues->density_array_name);
  auto* densityArrayPtr = m_DataStructure.getDataAs<Float32ArrayType>(densityArrayPath);
  auto& density = m_DataStructure.getDataAs<Float32ArrayType>(densityArrayPath)->getDataStoreRef();

  auto filesize = static_cast<usize>(fs::file_size(m_InputValues->vg_data_file));
  const usize allocatedBytes = density.getSize() * sizeof(float32);

  if(filesize < allocatedBytes)
  {
    return MakeErrorResult(k_VolBinaryAllocateMismatch, fmt::format("Binary file size ({}) is smaller than the number of allocated bytes ({}).", filesize, allocatedBytes));
  }

  if(m_ShouldCancel)
  {
    return {};
  }
  m_MessageHandler.sendInfoMessage("Reading data from .vol file");
  m_MessageHandler.sendProgressCount("Reading volume files", 0, 1);
  auto importResult = ImportFromBinaryFile(m_InputValues->vg_data_file, *densityArrayPtr);
  if(importResult.valid())
  {
    m_MessageHandler.sendProgressCount("Reading volume files", 1, 1);
  }
  return importResult;
}
