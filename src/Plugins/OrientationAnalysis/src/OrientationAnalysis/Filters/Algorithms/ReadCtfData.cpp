#include "ReadCtfData.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/StringArray.hpp"

#include <EbsdLib/IO/HKL/CtfConstants.h>
#include <EbsdLib/IO/HKL/CtfPhase.h>
#include <EbsdLib/Math/EbsdLibMath.h>

#include <fmt/format.h>
#include <nonstd/span.hpp>

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

using namespace nx::core;

ReadCtfData::ReadCtfData(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, ReadCtfDataInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(msgHandler)
{
}

ReadCtfData::~ReadCtfData() noexcept = default;

Result<> ReadCtfData::operator()()
{
  m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Reading .ctf file '{}'", m_InputValues->InputFile.string()));
  ebsdlib::CtfReader reader;
  reader.setFileName(m_InputValues->InputFile.string());
  const int32_t err = reader.readFile();
  if(err < 0)
  {
    // Some CtfReader failures supply only the returned error code.
    const int32_t errorCode = reader.getErrorCode() < 0 ? reader.getErrorCode() : err;
    return MakeErrorResult(errorCode, reader.getErrorMessage());
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

Result<> ReadCtfData::loadMaterialInfo(ebsdlib::CtfReader* reader) const
{
  const std::vector<ebsdlib::CtfPhase::Pointer> phases = reader->getPhaseVector();
  if(phases.empty())
  {
    // Without phase definitions, cell phase values cannot index ensemble data.
    return MakeErrorResult(-19600, fmt::format("The .ctf file '{}' declares no phases in its header. At least one phase definition is required.", m_InputValues->InputFile.string()));
  }

  const DataPath cellEnsembleAttributeMatrixPath = m_InputValues->DataContainerName.createChildPath(m_InputValues->CellEnsembleAttributeMatrixName);

  auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(cellEnsembleAttributeMatrixPath.createChildPath(ebsdlib::CtfFile::CrystalStructures));
  auto& materialNames = m_DataStructure.getDataRefAs<StringArray>(cellEnsembleAttributeMatrixPath.createChildPath(ebsdlib::CtfFile::MaterialName));
  auto& latticeConstants = m_DataStructure.getDataRefAs<Float32Array>(cellEnsembleAttributeMatrixPath.createChildPath(ebsdlib::CtfFile::LatticeConstants));

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

  for(const ebsdlib::CtfPhase::Pointer& phase : phases)
  {
    const auto phaseID = static_cast<usize>(phase->getPhaseIndex());
    // An out-of-range phase indicates a file change after preflight.
    if(phaseID >= numTuples)
    {
      return MakeErrorResult(
          -19605, fmt::format("Phase index {} from the .ctf file is at or above the Ensemble Attribute Matrix count {}. The input file may have changed since preflight.", phaseID, numTuples));
    }
    crystalStructures[phaseID] = phase->determineOrientationOpsIndex();
    materialNames[phaseID] = phase->getMaterialName();

    const std::vector<float32> lattConst = phase->getLatticeConstants();
    for(usize i = 0; i < 6; i++)
    {
      latticeConstants.getDataStoreRef().setComponent(phaseID, i, lattConst[i]);
    }
  }
  return {};
}

Result<> ReadCtfData::copyRawEbsdData(ebsdlib::CtfReader* reader) const
{
  const DataPath cellAttributeMatrixPath = m_InputValues->DataContainerName.createChildPath(m_InputValues->CellAttributeMatrixName);
  const DataPath cellEnsembleAttributeMatrixPath = m_InputValues->DataContainerName.createChildPath(m_InputValues->CellEnsembleAttributeMatrixName);

  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->DataContainerName);
  const usize totalCells = imageGeom.getNumberOfCells();

  // Guard reader buffers against a file change after preflight.
  if(reader->getNumberOfElements() < totalCells)
  {
    return MakeErrorResult(-19603, fmt::format("The .ctf reader produced {} scan points but the Image Geometry created at preflight expects {}. The input file may have changed since preflight.",
                                               reader->getNumberOfElements(), totalCells));
  }

  // Required .ctf columns must provide reader buffers.
  auto fetchColumn = [&reader](const std::string& columnName, void*& ptr) -> Result<> {
    ptr = reader->getPointerByName(columnName);
    if(ptr == nullptr)
    {
      return MakeErrorResult(-19601, fmt::format("The .ctf file's data section does not contain the required column '{}'.", columnName));
    }
    return {};
  };

  void* phaseColumnPtr = nullptr;
  void* euler1Ptr = nullptr;
  void* euler2Ptr = nullptr;
  void* euler3Ptr = nullptr;
  {
    Result<> fetchResult = fetchColumn(ebsdlib::Ctf::Phase, phaseColumnPtr);
    if(fetchResult.invalid())
    {
      return fetchResult;
    }
    fetchResult = fetchColumn(ebsdlib::Ctf::Euler1, euler1Ptr);
    if(fetchResult.invalid())
    {
      return fetchResult;
    }
    fetchResult = fetchColumn(ebsdlib::Ctf::Euler2, euler2Ptr);
    if(fetchResult.invalid())
    {
      return fetchResult;
    }
    fetchResult = fetchColumn(ebsdlib::Ctf::Euler3, euler3Ptr);
    if(fetchResult.invalid())
    {
      return fetchResult;
    }
  }

  // .ctf phase zero identifies an unindexed point and remains unchanged.
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    auto& targetArray = m_DataStructure.getDataRefAs<Int32Array>(cellAttributeMatrixPath.createChildPath(ebsdlib::CtfFile::Phases));
    auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(cellEnsembleAttributeMatrixPath.createChildPath(ebsdlib::CtfFile::CrystalStructures));
    const usize ensembleTupleCount = crystalStructures.getNumberOfTuples();

    const auto* phasePtr = static_cast<const int32*>(phaseColumnPtr);
    for(usize i = 0; i < totalCells; i++)
    {
      // Validate phase values before they index ensemble data.
      if(phasePtr[i] < 0 || static_cast<usize>(phasePtr[i]) >= ensembleTupleCount)
      {
        return MakeErrorResult(
            -19602, fmt::format("Scan point {} carries phase value {}, which is outside the valid range [0, {}] established by the file's phase definitions.", i, phasePtr[i], ensembleTupleCount - 1));
      }
    }
    targetArray.getDataStoreRef().copyFromBuffer(0, nonstd::span<const int32>(phasePtr, totalCells));
  }

  // Interleave Euler values with optional hex correction and unit conversion.
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    const auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(cellEnsembleAttributeMatrixPath.createChildPath(ebsdlib::CtfFile::CrystalStructures));
    // Reuse validated reader phases to avoid a second output-array read.
    const auto* cellPhases = static_cast<const int32*>(phaseColumnPtr);

    // Cache the small ensemble-level array once to avoid repeated out-of-core lookups.
    const auto& csStore = crystalStructures.getDataStoreRef();
    const usize numPhases = csStore.getNumberOfTuples();
    std::vector<uint32> csCache(numPhases);
    csStore.copyIntoBuffer(0, nonstd::span<uint32>(csCache.data(), numPhases));

    const auto* fComp0 = static_cast<const float32*>(euler1Ptr);
    const auto* fComp1 = static_cast<const float32*>(euler2Ptr);
    const auto* fComp2 = static_cast<const float32*>(euler3Ptr);

    auto& cellEulerAngles = m_DataStructure.getDataRefAs<Float32Array>(cellAttributeMatrixPath.createChildPath(ebsdlib::CtfFile::EulerAngles));
    auto& eulerStore = cellEulerAngles.getDataStoreRef();

    // Chunked interleaving with corrections applied in the local buffer before bulk write
    constexpr usize k_ChunkSize = 65536;
    std::vector<float32> eulerBuf(k_ChunkSize * 3);
    for(usize offset = 0; offset < totalCells; offset += k_ChunkSize)
    {
      const usize count = std::min(k_ChunkSize, totalCells - offset);
      for(usize i = 0; i < count; i++)
      {
        float32 euler1 = fComp0[offset + i];
        float32 euler2 = fComp1[offset + i];
        float32 euler3 = fComp2[offset + i];
        if(csCache[static_cast<usize>(cellPhases[offset + i])] == ebsdlib::CrystalStructure::Hexagonal_High && m_InputValues->EdaxHexagonalAlignment)
        {
          euler3 = static_cast<float32>(static_cast<float64>(euler3) + 30.0);
        }
        if(m_InputValues->DegreesToRadians)
        {
          euler1 = static_cast<float32>(static_cast<float64>(euler1) * ebsdlib::constants::k_PiOver180D);
          euler2 = static_cast<float32>(static_cast<float64>(euler2) * ebsdlib::constants::k_PiOver180D);
          euler3 = static_cast<float32>(static_cast<float64>(euler3) * ebsdlib::constants::k_PiOver180D);
        }
        eulerBuf[3 * i] = euler1;
        eulerBuf[3 * i + 1] = euler2;
        eulerBuf[3 * i + 2] = euler3;
      }
      eulerStore.copyFromBuffer(offset * 3, nonstd::span<const float32>(eulerBuf.data(), count * 3));
    }
  }

  if(m_ShouldCancel)
  {
    return {};
  }

  // The remaining columns are copied verbatim with one bulk write per array.
  const std::vector<std::pair<std::string, nx::core::DataType>> passthroughColumns = {
      {ebsdlib::Ctf::Bands, DataType::int32}, {ebsdlib::Ctf::Error, DataType::int32}, {ebsdlib::Ctf::MAD, DataType::float32}, {ebsdlib::Ctf::BC, DataType::int32},
      {ebsdlib::Ctf::BS, DataType::int32},    {ebsdlib::Ctf::X, DataType::float32},   {ebsdlib::Ctf::Y, DataType::float32},
  };
  for(const auto& [columnName, dataType] : passthroughColumns)
  {
    void* columnPtr = nullptr;
    Result<> fetchResult = fetchColumn(columnName, columnPtr);
    if(fetchResult.invalid())
    {
      return fetchResult;
    }
    const DataPath targetPath = cellAttributeMatrixPath.createChildPath(columnName);
    if(dataType == DataType::int32)
    {
      const auto* sourcePtr = static_cast<const int32*>(columnPtr);
      auto& targetArray = m_DataStructure.getDataRefAs<Int32Array>(targetPath);
      targetArray.getDataStoreRef().copyFromBuffer(0, nonstd::span<const int32>(sourcePtr, totalCells));
    }
    else
    {
      const auto* sourcePtr = static_cast<const float32*>(columnPtr);
      auto& targetArray = m_DataStructure.getDataRefAs<Float32Array>(targetPath);
      targetArray.getDataStoreRef().copyFromBuffer(0, nonstd::span<const float32>(sourcePtr, totalCells));
    }
  }

  return {};
}
