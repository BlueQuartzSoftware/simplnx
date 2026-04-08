#include "ReadCtfData.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/StringArray.hpp"

#include <EbsdLib/IO/HKL/CtfConstants.h>
#include <EbsdLib/Math/EbsdLibMath.h>

#include <nonstd/span.hpp>

using namespace nx::core;

using FloatVec3Type = std::vector<float>;

// -----------------------------------------------------------------------------
ReadCtfData::ReadCtfData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadCtfDataInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ReadCtfData::~ReadCtfData() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ReadCtfData::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ReadCtfData::operator()()
{
  ebsdlib::CtfReader reader;
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

  copyRawEbsdData(&reader);

  return {};
}

// -----------------------------------------------------------------------------
std::pair<int32, std::string> ReadCtfData::loadMaterialInfo(ebsdlib::CtfReader* reader) const
{
  const DataPath cellEnsembleAttributeMatrixPath = m_InputValues->DataContainerName.createChildPath(m_InputValues->CellEnsembleAttributeMatrixName);

  const std::vector<ebsdlib::CtfPhase::Pointer> phases = reader->getPhaseVector();
  if(phases.empty())
  {
    return {reader->getErrorCode(), reader->getErrorMessage()};
  }

  auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(cellEnsembleAttributeMatrixPath.createChildPath(ebsdlib::CtfFile::CrystalStructures));

  auto& materialNames = m_DataStructure.getDataRefAs<StringArray>(cellEnsembleAttributeMatrixPath.createChildPath(ebsdlib::CtfFile::MaterialName));
  auto& latticeConstants = m_DataStructure.getDataRefAs<Float32Array>(cellEnsembleAttributeMatrixPath.createChildPath(ebsdlib::CtfFile::LatticeConstants));

  const std::string k_InvalidPhase = "Invalid Phase";

  // Initialize the zero'th element to unknowns. The other elements will
  // be filled in based on values from the data file
  crystalStructures[0] = ebsdlib::CrystalStructure::UnknownCrystalStructure;
  materialNames[0] = k_InvalidPhase;

  for(size_t i = 0; i < 6; i++)
  {
    latticeConstants.getDataStoreRef().setComponent(0, i, 0.0F);
  }

  for(const ebsdlib::CtfPhase::Pointer& phase : phases)
  {
    const int32_t phaseID = phase->getPhaseIndex();
    crystalStructures[phaseID] = phase->determineOrientationOpsIndex();
    const std::string materialName = phase->getMaterialName();
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
void ReadCtfData::copyRawEbsdData(ebsdlib::CtfReader* reader) const
{
  const DataPath cellAttributeMatrixPath = m_InputValues->DataContainerName.createChildPath(m_InputValues->CellAttributeMatrixName);
  const DataPath cellEnsembleAttributeMatrixPath = m_InputValues->DataContainerName.createChildPath(m_InputValues->CellEnsembleAttributeMatrixName);

  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->DataContainerName);
  const usize totalCells = imageGeom.getNumberOfCells();

  // Prepare the Cell Attribute Matrix with the correct number of tuples based on the total Cells being read from the file.
  std::vector<usize> tDims = {imageGeom.getNumXCells(), imageGeom.getNumYCells(), imageGeom.getNumZCells()};

  // Copy the Phase Array
  {
    auto& targetArray = m_DataStructure.getDataRefAs<Int32Array>(cellAttributeMatrixPath.createChildPath(ebsdlib::CtfFile::Phases));
    auto* phasePtr = reinterpret_cast<int32*>(reader->getPointerByName(ebsdlib::Ctf::Phase));
    targetArray.getDataStoreRef().copyFromBuffer(0, nonstd::span<const int32>(phasePtr, totalCells));
  }

  // Condense the Euler Angles from 3 separate arrays into a single 1x3 array
  {
    auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(cellEnsembleAttributeMatrixPath.createChildPath(ebsdlib::CtfFile::CrystalStructures));
    const auto* phasePtr = reinterpret_cast<const int32*>(reader->getPointerByName(ebsdlib::Ctf::Phase));

    // Cache ensemble-level crystal structures locally
    const auto& csStore = crystalStructures.getDataStoreRef();
    const usize numPhases = csStore.getNumberOfTuples();
    std::vector<uint32> csCache(numPhases);
    csStore.copyIntoBuffer(0, nonstd::span<uint32>(csCache.data(), numPhases));

    const auto* fComp0 = reinterpret_cast<const float*>(reader->getPointerByName(ebsdlib::Ctf::Euler1));
    const auto* fComp1 = reinterpret_cast<const float*>(reader->getPointerByName(ebsdlib::Ctf::Euler2));
    const auto* fComp2 = reinterpret_cast<const float*>(reader->getPointerByName(ebsdlib::Ctf::Euler3));

    auto& cellEulerAngles = m_DataStructure.getDataRefAs<Float32Array>(cellAttributeMatrixPath.createChildPath(ebsdlib::CtfFile::EulerAngles));
    auto& eulerStore = cellEulerAngles.getDataStoreRef();

    constexpr usize k_ChunkSize = 65536;
    std::vector<float32> eulerBuf(k_ChunkSize * 3);
    for(usize offset = 0; offset < totalCells; offset += k_ChunkSize)
    {
      usize count = std::min(k_ChunkSize, totalCells - offset);
      for(usize i = 0; i < count; i++)
      {
        float32 e0 = fComp0[offset + i];
        float32 e1 = fComp1[offset + i];
        float32 e2 = fComp2[offset + i];
        if(csCache[phasePtr[offset + i]] == ebsdlib::CrystalStructure::Hexagonal_High && m_InputValues->EdaxHexagonalAlignment)
        {
          e2 = e2 + 30.0F; // See the documentation for this correction factor
        }
        // Now convert to radians if requested by the user
        if(m_InputValues->DegreesToRadians)
        {
          e0 = e0 * ebsdlib::constants::k_PiOver180F;
          e1 = e1 * ebsdlib::constants::k_PiOver180F;
          e2 = e2 * ebsdlib::constants::k_PiOver180F;
        }
        eulerBuf[3 * i] = e0;
        eulerBuf[3 * i + 1] = e1;
        eulerBuf[3 * i + 2] = e2;
      }
      eulerStore.copyFromBuffer(offset * 3, nonstd::span<const float32>(eulerBuf.data(), count * 3));
    }
  }

  {
    auto* srcPtr = reinterpret_cast<int32*>(reader->getPointerByName(ebsdlib::Ctf::Bands));
    auto& targetArray = m_DataStructure.getDataRefAs<Int32Array>(cellAttributeMatrixPath.createChildPath(ebsdlib::Ctf::Bands));
    targetArray.getDataStoreRef().copyFromBuffer(0, nonstd::span<const int32>(srcPtr, totalCells));
  }

  {
    auto* srcPtr = reinterpret_cast<int32*>(reader->getPointerByName(ebsdlib::Ctf::Error));
    auto& targetArray = m_DataStructure.getDataRefAs<Int32Array>(cellAttributeMatrixPath.createChildPath(ebsdlib::Ctf::Error));
    targetArray.getDataStoreRef().copyFromBuffer(0, nonstd::span<const int32>(srcPtr, totalCells));
  }

  {
    auto* srcPtr = reinterpret_cast<float32*>(reader->getPointerByName(ebsdlib::Ctf::MAD));
    auto& targetArray = m_DataStructure.getDataRefAs<Float32Array>(cellAttributeMatrixPath.createChildPath(ebsdlib::Ctf::MAD));
    targetArray.getDataStoreRef().copyFromBuffer(0, nonstd::span<const float32>(srcPtr, totalCells));
  }

  {
    auto* srcPtr = reinterpret_cast<int32*>(reader->getPointerByName(ebsdlib::Ctf::BC));
    auto& targetArray = m_DataStructure.getDataRefAs<Int32Array>(cellAttributeMatrixPath.createChildPath(ebsdlib::Ctf::BC));
    targetArray.getDataStoreRef().copyFromBuffer(0, nonstd::span<const int32>(srcPtr, totalCells));
  }

  {
    auto* srcPtr = reinterpret_cast<int32*>(reader->getPointerByName(ebsdlib::Ctf::BS));
    auto& targetArray = m_DataStructure.getDataRefAs<Int32Array>(cellAttributeMatrixPath.createChildPath(ebsdlib::Ctf::BS));
    targetArray.getDataStoreRef().copyFromBuffer(0, nonstd::span<const int32>(srcPtr, totalCells));
  }

  {
    auto* srcPtr = reinterpret_cast<float32*>(reader->getPointerByName(ebsdlib::Ctf::X));
    auto& targetArray = m_DataStructure.getDataRefAs<Float32Array>(cellAttributeMatrixPath.createChildPath(ebsdlib::Ctf::X));
    targetArray.getDataStoreRef().copyFromBuffer(0, nonstd::span<const float32>(srcPtr, totalCells));
  }

  {
    auto* srcPtr = reinterpret_cast<float32*>(reader->getPointerByName(ebsdlib::Ctf::Y));
    auto& targetArray = m_DataStructure.getDataRefAs<Float32Array>(cellAttributeMatrixPath.createChildPath(ebsdlib::Ctf::Y));
    targetArray.getDataStoreRef().copyFromBuffer(0, nonstd::span<const float32>(srcPtr, totalCells));
  }
}
