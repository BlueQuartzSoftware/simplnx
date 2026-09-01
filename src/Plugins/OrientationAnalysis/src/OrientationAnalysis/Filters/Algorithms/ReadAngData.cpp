#include "ReadAngData.hpp"

#include "simplnx/Common/RgbColor.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <EbsdLib/Core/Orientation.hpp>

#include <fmt/format.h>
#include <nonstd/span.hpp>

using namespace nx::core;

ReadAngData::ReadAngData(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, ReadAngDataInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_MessageHandler(msgHandler)
, m_ShouldCancel(shouldCancel)
, m_InputValues(inputValues)
{
}

ReadAngData::~ReadAngData() noexcept = default;

Result<> ReadAngData::operator()()
{
  m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Reading .ang file '{}'", m_InputValues->InputFile.string()));
  ebsdlib::AngReader reader;
  reader.setFileName(m_InputValues->InputFile.string());
  const int32_t err = reader.readFile();
  if(err < 0)
  {
    return MakeErrorResult(reader.getErrorCode(), reader.getErrorMessage());
  }
  Result<> result = loadMaterialInfo(&reader);
  if(result.invalid())
  {
    return result;
  }
  if(m_ShouldCancel)
  {
    return {};
  }

  m_MessageHandler(IFilter::Message::Type::Info, "Copying cell data into the Image Geometry");
  return copyRawEbsdData(&reader);
}

Result<> ReadAngData::loadMaterialInfo(ebsdlib::AngReader* reader) const
{
  const std::vector<ebsdlib::AngPhase::Pointer> phases = reader->getPhaseVector();
  if(phases.empty())
  {
    return MakeErrorResult(reader->getErrorCode(), reader->getErrorMessage());
  }

  const DataPath cellEnsembleAttributeMatrixPath = m_InputValues->DataContainerName.createChildPath(m_InputValues->CellEnsembleAttributeMatrixName);

  auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(cellEnsembleAttributeMatrixPath.createChildPath(ebsdlib::AngFile::CrystalStructures));

  auto& materialNames = m_DataStructure.getDataRefAs<StringArray>(cellEnsembleAttributeMatrixPath.createChildPath(ebsdlib::AngFile::MaterialName));

  auto& latticeConstants = m_DataStructure.getDataRefAs<Float32Array>(cellEnsembleAttributeMatrixPath.createChildPath(ebsdlib::AngFile::LatticeConstants));

  const std::string k_InvalidPhase = "Invalid Phase";
  const usize numTuples = crystalStructures.getNumberOfTuples();

  // Defaults preserve invalid and omitted phase slots.
  for(usize tupleIndex = 0; tupleIndex < numTuples; tupleIndex++)
  {
    crystalStructures[tupleIndex] = ebsdlib::CrystalStructure::UnknownCrystalStructure;
    materialNames[tupleIndex] = k_InvalidPhase;
    for(usize i = 0; i < 6; i++)
    {
      latticeConstants.getDataStoreRef().setComponent(tupleIndex, i, 0.0F);
    }
  }

  for(const ebsdlib::AngPhase::Pointer& phase : phases)
  {
    const int32_t phaseID = phase->getPhaseIndex();
    // .ang phase numbering starts at one. Phase zero remains invalid.
    if(phaseID < 1)
    {
      return MakeErrorResult(-19502, fmt::format("The .ang file declares phase index {}, but .ang phase numbering starts at 1 (Phase 0 and negative phases are not supported).", phaseID));
    }
    // An out-of-range phase indicates a file change after preflight.
    if(static_cast<usize>(phaseID) >= numTuples)
    {
      return MakeErrorResult(
          -19504, fmt::format("Phase index {} from the .ang file is at or above the Ensemble Attribute Matrix count {}. The input file may have changed since preflight.", phaseID, numTuples));
    }
    crystalStructures[phaseID] = phase->determineOrientationOpsIndex();
    // EbsdLib's AngPhase::parseMaterialName() rejoins the name tokens with a trailing
    // space (e.g. "Nickel "), so the name must be trimmed before storing it.
    materialNames[phaseID] = nx::core::StringUtilities::trimmed(phase->getMaterialName());

    std::vector<float> lattConst = phase->getLatticeConstants();

    for(usize i = 0; i < 6; i++)
    {
      latticeConstants.getDataStoreRef().setComponent(phaseID, i, lattConst[i]);
    }
  }
  return {};
}

Result<> ReadAngData::copyRawEbsdData(ebsdlib::AngReader* reader) const
{
  const DataPath cellAttributeMatrixPath = m_InputValues->DataContainerName.createChildPath(m_InputValues->CellAttributeMatrixName);

  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->DataContainerName);
  const usize totalCells = imageGeom.getNumberOfCells();

  // The geometry and reader can disagree after a file change. Guard reader
  // buffers before copy operations.
  if(reader->getNumberOfElements() < totalCells)
  {
    return MakeErrorResult(-19503,
                           fmt::format("The .ang reader produced {} scan points but the Image Geometry created at preflight expects {}. The input file may have changed since preflight, or its "
                                       "column header (NCOLS_ODD/NCOLS_EVEN) is inconsistent.",
                                       reader->getNumberOfElements(), totalCells));
  }

  // .ang phase zero and negative values map to the first valid phase.
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    auto& targetArray = m_DataStructure.getDataRefAs<Int32Array>(cellAttributeMatrixPath.createChildPath(ebsdlib::AngFile::Phases));
    auto* phasePtr = reinterpret_cast<int32*>(reader->getPointerByName(ebsdlib::Ang::PhaseData));
    for(usize i = 0; i < totalCells; i++)
    {
      if(phasePtr[i] < 1)
      {
        phasePtr[i] = 1;
      }
    }
    targetArray.getDataStoreRef().copyFromBuffer(0, nonstd::span<const int32>(phasePtr, totalCells));
  }

  // Condense the Euler Angles from 3 separate source arrays (Phi1, Phi, Phi2) into a
  // single interleaved 3-component destination array. Uses chunked interleaving to
  // bound memory while still writing bulk chunks via copyFromBuffer.
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    const auto* fComp0 = reinterpret_cast<const float*>(reader->getPointerByName(ebsdlib::Ang::Phi1));
    const auto* fComp1 = reinterpret_cast<const float*>(reader->getPointerByName(ebsdlib::Ang::Phi));
    const auto* fComp2 = reinterpret_cast<const float*>(reader->getPointerByName(ebsdlib::Ang::Phi2));

    auto& cellEulerAngles = m_DataStructure.getDataRefAs<Float32Array>(cellAttributeMatrixPath.createChildPath(ebsdlib::AngFile::EulerAngles));
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
    return {};
  }

  // OOC-safe bulk writes for single-component float arrays.
  // Each copyFromBuffer() writes the entire reader buffer in one I/O operation,
  // which is optimal for both in-memory and OOC DataStore backends.
  {
    auto* srcPtr = reinterpret_cast<float32*>(reader->getPointerByName(ebsdlib::Ang::ImageQuality));
    auto& targetArray = m_DataStructure.getDataRefAs<Float32Array>(cellAttributeMatrixPath.createChildPath(ebsdlib::Ang::ImageQuality));
    targetArray.getDataStoreRef().copyFromBuffer(0, nonstd::span<const float32>(srcPtr, totalCells));
  }

  {
    auto* srcPtr = reinterpret_cast<float32*>(reader->getPointerByName(ebsdlib::Ang::ConfidenceIndex));
    auto& targetArray = m_DataStructure.getDataRefAs<Float32Array>(cellAttributeMatrixPath.createChildPath(ebsdlib::Ang::ConfidenceIndex));
    targetArray.getDataStoreRef().copyFromBuffer(0, nonstd::span<const float32>(srcPtr, totalCells));
  }

  {
    auto* srcPtr = reinterpret_cast<float32*>(reader->getPointerByName(ebsdlib::Ang::SEMSignal));
    auto& targetArray = m_DataStructure.getDataRefAs<Float32Array>(cellAttributeMatrixPath.createChildPath(ebsdlib::Ang::SEMSignal));
    targetArray.getDataStoreRef().copyFromBuffer(0, nonstd::span<const float32>(srcPtr, totalCells));
  }

  {
    auto* srcPtr = reinterpret_cast<float32*>(reader->getPointerByName(ebsdlib::Ang::Fit));
    auto& targetArray = m_DataStructure.getDataRefAs<Float32Array>(cellAttributeMatrixPath.createChildPath(ebsdlib::Ang::Fit));
    targetArray.getDataStoreRef().copyFromBuffer(0, nonstd::span<const float32>(srcPtr, totalCells));
  }

  {
    auto* srcPtr = reinterpret_cast<float32*>(reader->getPointerByName(ebsdlib::Ang::XPosition));
    auto& targetArray = m_DataStructure.getDataRefAs<Float32Array>(cellAttributeMatrixPath.createChildPath(ebsdlib::Ang::XPosition));
    targetArray.getDataStoreRef().copyFromBuffer(0, nonstd::span<const float32>(srcPtr, totalCells));
  }

  {
    auto* srcPtr = reinterpret_cast<float32*>(reader->getPointerByName(ebsdlib::Ang::YPosition));
    auto& targetArray = m_DataStructure.getDataRefAs<Float32Array>(cellAttributeMatrixPath.createChildPath(ebsdlib::Ang::YPosition));
    targetArray.getDataStoreRef().copyFromBuffer(0, nonstd::span<const float32>(srcPtr, totalCells));
  }

  return {};
}
