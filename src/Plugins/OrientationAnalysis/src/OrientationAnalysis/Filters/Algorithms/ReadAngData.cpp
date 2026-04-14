#include "ReadAngData.hpp"

#include "simplnx/Common/RgbColor.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <EbsdLib/Core/Orientation.hpp>

#include <nonstd/span.hpp>

using namespace nx::core;

using FloatVec3Type = std::vector<float>;

// -----------------------------------------------------------------------------
/**
 * @brief Constructs ReadAngData with the given DataStructure, message handler, cancel flag, and input values.
 */
ReadAngData::ReadAngData(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, ReadAngDataInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_MessageHandler(msgHandler)
, m_ShouldCancel(shouldCancel)
, m_InputValues(inputValues)
{
}

// -----------------------------------------------------------------------------
ReadAngData::~ReadAngData() noexcept = default;

// -----------------------------------------------------------------------------
/**
 * @brief Reads a .ang EBSD data file and populates the DataStructure with the parsed arrays.
 *
 * Delegates to EbsdLib's AngReader for file parsing, then calls loadMaterialInfo()
 * to populate ensemble-level arrays and copyRawEbsdData() to populate cell-level arrays.
 *
 * @return Result<> indicating success or an error from the EbsdLib reader.
 */
Result<> ReadAngData::operator()()
{
  ebsdlib::AngReader reader;
  reader.setFileName(m_InputValues->InputFile.string());
  const int32_t err = reader.readFile();
  if(err < 0)
  {
    return MakeErrorResult(reader.getErrorCode(), reader.getErrorMessage());
  }
  const auto result = loadMaterialInfo(&reader);
  if(result.first < 0)
  {
    return MakeErrorResult(result.first, result.second);
  }
  if(m_ShouldCancel)
  {
    return {};
  }

  copyRawEbsdData(&reader);

  return {};
}

// -----------------------------------------------------------------------------
std::pair<int32, std::string> ReadAngData::loadMaterialInfo(ebsdlib::AngReader* reader) const
{

  const std::vector<ebsdlib::AngPhase::Pointer> phases = reader->getPhaseVector();
  if(phases.empty())
  {
    return {reader->getErrorCode(), reader->getErrorMessage()};
  }

  const DataPath CellEnsembleAttributeMatrixPath = m_InputValues->DataContainerName.createChildPath(m_InputValues->CellEnsembleAttributeMatrixName);

  auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(CellEnsembleAttributeMatrixPath.createChildPath(ebsdlib::AngFile::CrystalStructures));

  auto& materialNames = m_DataStructure.getDataRefAs<StringArray>(CellEnsembleAttributeMatrixPath.createChildPath(ebsdlib::AngFile::MaterialName));

  auto& latticeConstants = m_DataStructure.getDataRefAs<Float32Array>(CellEnsembleAttributeMatrixPath.createChildPath(ebsdlib::AngFile::LatticeConstants));

  const std::string k_InvalidPhase = "Invalid Phase";

  // Initialize the zero'th element to unknowns. The other elements will
  // be filled in based on values from the data file
  crystalStructures[0] = ebsdlib::CrystalStructure::UnknownCrystalStructure;
  materialNames[0] = k_InvalidPhase;

  for(size_t i = 0; i < 6; i++)
  {
    latticeConstants.getDataStoreRef().setComponent(0, i, 0.0F);
  }

  for(const ebsdlib::AngPhase::Pointer& phase : phases)
  {
    const int32_t phaseID = phase->getPhaseIndex();
    crystalStructures[phaseID] = phase->determineOrientationOpsIndex();
    std::string materialName = phase->getMaterialName();
    materialName = nx::core::StringUtilities::replace(materialName, "MaterialName", "");
    materialName = nx::core::StringUtilities::trimmed(materialName);
    materialNames[phaseID] = materialName;

    std::vector<float> lattConst = phase->getLatticeConstants();

    for(size_t i = 0; i < 6; i++)
    {
      latticeConstants.getDataStoreRef().setComponent(phaseID, i, lattConst[i]);
    }
  }
  return {0, ""};
}

// -----------------------------------------------------------------------------
/**
 * @brief Copies raw EBSD data from the EbsdLib AngReader buffers into the DataStructure arrays.
 *
 * @section ooc_strategy OOC Strategy
 * The EbsdLib reader holds the parsed data in contiguous in-memory buffers. We need to
 * transfer this data into DataStore-backed arrays that may be out-of-core. Rather than
 * using per-element operator[] (which would trigger a chunk load/evict per write on OOC
 * stores), we use copyFromBuffer() to write entire contiguous ranges in single bulk
 * operations.
 *
 * For single-component arrays (ImageQuality, ConfidenceIndex, etc.), a single
 * copyFromBuffer() call writes all values at once since the reader buffer is already
 * contiguous.
 *
 * For the Euler angles (3 separate source arrays that must be interleaved into a
 * 3-component destination), we use a chunked approach: interleave k_ChunkSize tuples
 * into a local buffer, then copyFromBuffer() the chunk. This bounds memory usage to
 * ~768 KB (65536 * 3 * sizeof(float32)) while still achieving bulk I/O efficiency.
 *
 * @param reader Pointer to the EbsdLib AngReader that has already parsed the file.
 */
void ReadAngData::copyRawEbsdData(ebsdlib::AngReader* reader) const
{
  const DataPath CellAttributeMatrixPath = m_InputValues->DataContainerName.createChildPath(m_InputValues->CellAttributeMatrixName);

  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->DataContainerName);
  const usize totalCells = imageGeom.getNumberOfCells();

  // Prepare the Cell Attribute Matrix with the correct number of tuples based on the total Cells being read from the file.
  std::vector<usize> tDims = {imageGeom.getNumXCells(), imageGeom.getNumYCells(), imageGeom.getNumZCells()};

  // Adjust the values of the 'phase' data to correct for invalid values, then bulk-write
  // via copyFromBuffer (OOC-safe: single I/O call for the entire array)
  {
    if(m_ShouldCancel)
    {
      return;
    }
    auto& targetArray = m_DataStructure.getDataRefAs<Int32Array>(CellAttributeMatrixPath.createChildPath(ebsdlib::AngFile::Phases));
    auto* phasePtr = reinterpret_cast<int32*>(reader->getPointerByName(ebsdlib::Ang::PhaseData));
    // Validate phases in-place in the reader's buffer before bulk-writing
    for(usize i = 0; i < totalCells; i++)
    {
      if(phasePtr[i] < 1)
      {
        phasePtr[i] = 1;
      }
    }
    // OOC-safe: single bulk write of the entire phase array
    targetArray.getDataStoreRef().copyFromBuffer(0, nonstd::span<const int32>(phasePtr, totalCells));
  }

  // Condense the Euler Angles from 3 separate source arrays (Phi1, Phi, Phi2) into a
  // single interleaved 3-component destination array. Uses chunked interleaving to
  // bound memory while still writing bulk chunks via copyFromBuffer.
  {
    if(m_ShouldCancel)
    {
      return;
    }
    const auto* fComp0 = reinterpret_cast<const float*>(reader->getPointerByName(ebsdlib::Ang::Phi1));
    const auto* fComp1 = reinterpret_cast<const float*>(reader->getPointerByName(ebsdlib::Ang::Phi));
    const auto* fComp2 = reinterpret_cast<const float*>(reader->getPointerByName(ebsdlib::Ang::Phi2));

    auto& cellEulerAngles = m_DataStructure.getDataRefAs<Float32Array>(CellAttributeMatrixPath.createChildPath(ebsdlib::AngFile::EulerAngles));
    auto& eulerStore = cellEulerAngles.getDataStoreRef();

    // Chunked interleaving: interleave k_ChunkSize tuples into a local buffer,
    // then bulk-write the chunk. This avoids both per-element OOC access and
    // allocating a buffer for the entire volume.
    constexpr usize k_ChunkSize = 65536;
    std::vector<float32> eulerBuf(k_ChunkSize * 3);
    for(usize offset = 0; offset < totalCells; offset += k_ChunkSize)
    {
      usize count = std::min(k_ChunkSize, totalCells - offset);
      for(usize i = 0; i < count; i++)
      {
        eulerBuf[3 * i] = fComp0[offset + i];
        eulerBuf[3 * i + 1] = fComp1[offset + i];
        eulerBuf[3 * i + 2] = fComp2[offset + i];
      }
      eulerStore.copyFromBuffer(offset * 3, nonstd::span<const float32>(eulerBuf.data(), count * 3));
    }
  }

  if(m_ShouldCancel)
  {
    return;
  }

  // OOC-safe bulk writes for single-component float arrays.
  // Each copyFromBuffer() writes the entire reader buffer in one I/O operation,
  // which is optimal for both in-memory and OOC DataStore backends.
  {
    auto* srcPtr = reinterpret_cast<float32*>(reader->getPointerByName(ebsdlib::Ang::ImageQuality));
    auto& targetArray = m_DataStructure.getDataRefAs<Float32Array>(CellAttributeMatrixPath.createChildPath(ebsdlib::Ang::ImageQuality));
    targetArray.getDataStoreRef().copyFromBuffer(0, nonstd::span<const float32>(srcPtr, totalCells));
  }

  {
    auto* srcPtr = reinterpret_cast<float32*>(reader->getPointerByName(ebsdlib::Ang::ConfidenceIndex));
    auto& targetArray = m_DataStructure.getDataRefAs<Float32Array>(CellAttributeMatrixPath.createChildPath(ebsdlib::Ang::ConfidenceIndex));
    targetArray.getDataStoreRef().copyFromBuffer(0, nonstd::span<const float32>(srcPtr, totalCells));
  }

  {
    auto* srcPtr = reinterpret_cast<float32*>(reader->getPointerByName(ebsdlib::Ang::SEMSignal));
    auto& targetArray = m_DataStructure.getDataRefAs<Float32Array>(CellAttributeMatrixPath.createChildPath(ebsdlib::Ang::SEMSignal));
    targetArray.getDataStoreRef().copyFromBuffer(0, nonstd::span<const float32>(srcPtr, totalCells));
  }

  {
    auto* srcPtr = reinterpret_cast<float32*>(reader->getPointerByName(ebsdlib::Ang::Fit));
    auto& targetArray = m_DataStructure.getDataRefAs<Float32Array>(CellAttributeMatrixPath.createChildPath(ebsdlib::Ang::Fit));
    targetArray.getDataStoreRef().copyFromBuffer(0, nonstd::span<const float32>(srcPtr, totalCells));
  }

  {
    auto* srcPtr = reinterpret_cast<float32*>(reader->getPointerByName(ebsdlib::Ang::XPosition));
    auto& targetArray = m_DataStructure.getDataRefAs<Float32Array>(CellAttributeMatrixPath.createChildPath(ebsdlib::Ang::XPosition));
    targetArray.getDataStoreRef().copyFromBuffer(0, nonstd::span<const float32>(srcPtr, totalCells));
  }

  {
    auto* srcPtr = reinterpret_cast<float32*>(reader->getPointerByName(ebsdlib::Ang::YPosition));
    auto& targetArray = m_DataStructure.getDataRefAs<Float32Array>(CellAttributeMatrixPath.createChildPath(ebsdlib::Ang::YPosition));
    targetArray.getDataStoreRef().copyFromBuffer(0, nonstd::span<const float32>(srcPtr, totalCells));
  }
}
