#include "ReadCtfData.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/StringArray.hpp"

#include <EbsdLib/IO/HKL/CtfConstants.h>
#include <EbsdLib/IO/HKL/CtfPhase.h>
#include <EbsdLib/Math/EbsdLibMath.h>

#include <fmt/format.h>

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

using namespace nx::core;

// -----------------------------------------------------------------------------
ReadCtfData::ReadCtfData(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, ReadCtfDataInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(msgHandler)
{
}

// -----------------------------------------------------------------------------
ReadCtfData::~ReadCtfData() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ReadCtfData::operator()()
{
  m_MessageHandler.sendInfoMessage(fmt::format("Reading .ctf file '{}'", m_InputValues->InputFile.string()));
  ebsdlib::CtfReader reader;
  reader.setFileName(m_InputValues->InputFile.string());
  const int32_t err = reader.readFile();
  if(err < 0)
  {
    // CtfReader does not set its error-code member on every failure path (the zero-step and
    // zero-cells rejections only set the message), so fall back to the returned code.
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

  m_MessageHandler.sendInfoMessage("Copying cell data into the Image Geometry");
  return copyRawEbsdData(&reader);
}

// -----------------------------------------------------------------------------
Result<> ReadCtfData::loadMaterialInfo(ebsdlib::CtfReader* reader) const
{
  const std::vector<ebsdlib::CtfPhase::Pointer> phases = reader->getPhaseVector();
  if(phases.empty())
  {
    // A "Phases 0" header parses successfully (CtfReader's error code stays 0), but a file with no
    // phase definitions carries no usable ensemble information, and every data row's phase value
    // would then be out of range. Reject the file with a clear message instead. (Historical note:
    // before this guard the early return here skipped the ensemble initialization below, and since
    // Hexagonal_High is enum value 0, the zero-filled CrystalStructures applied the +30 degree
    // hexagonal alignment to every point.)
    return MakeErrorResult(-19600, fmt::format("The .ctf file '{}' declares no phases in its header. At least one phase definition is required.", m_InputValues->InputFile.string()));
  }

  const DataPath cellEnsembleAttributeMatrixPath = m_InputValues->DataContainerName.createChildPath(m_InputValues->CellEnsembleAttributeMatrixName);

  auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(cellEnsembleAttributeMatrixPath.createChildPath(ebsdlib::CtfFile::CrystalStructures));
  auto& materialNames = m_DataStructure.getDataRefAs<StringArray>(cellEnsembleAttributeMatrixPath.createChildPath(ebsdlib::CtfFile::MaterialName));
  auto& latticeConstants = m_DataStructure.getDataRefAs<Float32Array>(cellEnsembleAttributeMatrixPath.createChildPath(ebsdlib::CtfFile::LatticeConstants));

  const std::string k_InvalidPhase = "Invalid Phase";
  const usize numTuples = crystalStructures.getNumberOfTuples();

  // Initialize EVERY slot to the "Invalid Phase" defaults first. Slot 0 is always the invalid
  // phase; CtfReader assigns phase indices sequentially (1..N) so every other slot is refilled
  // by the loop below, but initializing them all keeps the defaults authoritative rather than
  // relying on zero-initialized storage.
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
    // The ensemble arrays were sized at preflight from the same file's phase count, so an index at
    // or above the tuple count can only mean the file gained a phase between preflight and execute.
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

// -----------------------------------------------------------------------------
Result<> ReadCtfData::copyRawEbsdData(ebsdlib::CtfReader* reader) const
{
  const DataPath cellAttributeMatrixPath = m_InputValues->DataContainerName.createChildPath(m_InputValues->CellAttributeMatrixName);
  const DataPath cellEnsembleAttributeMatrixPath = m_InputValues->DataContainerName.createChildPath(m_InputValues->CellEnsembleAttributeMatrixName);

  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->DataContainerName);
  const usize totalCells = imageGeom.getNumberOfCells();

  // The Image Geometry was sized at preflight from the file's XCells/YCells/ZCells header keys.
  // Every copy below reads totalCells elements out of the reader's buffers, so if the reader
  // actually produced fewer elements (a file that changed between preflight and execute), the
  // copies would read past the end of the reader's heap buffers. Guard against that.
  if(reader->getNumberOfElements() < totalCells)
  {
    return MakeErrorResult(-19603, fmt::format("The .ctf reader produced {} scan points but the Image Geometry created at preflight expects {}. The input file may have changed since preflight.",
                                               reader->getNumberOfElements(), totalCells));
  }

  // A .ctf file's data section defines its own columns; a file missing one of the standard
  // columns hands back a null buffer, which must be rejected rather than dereferenced.
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

  // Copy the Phase column verbatim. Unlike EDAX .ang files, a phase value of 0 is meaningful in
  // .ctf files (a "zero solutions" / unindexed point) and is preserved as-is; the legacy remap of
  // phase<1 -> 1 was deliberately removed (PR #937).
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
      // The raw phase value indexes the ensemble arrays in the Euler loop below, so an
      // out-of-range value (a corrupt file, or a phase column inconsistent with the header's
      // phase count) would be an out-of-bounds read there. Reject it here.
      if(phasePtr[i] < 0 || static_cast<usize>(phasePtr[i]) >= ensembleTupleCount)
      {
        return MakeErrorResult(
            -19602, fmt::format("Scan point {} carries phase value {}, which is outside the valid range [0, {}] established by the file's phase definitions.", i, phasePtr[i], ensembleTupleCount - 1));
      }
      targetArray[i] = phasePtr[i];
    }
  }

  // Condense the Euler Angles from 3 separate arrays into a single 3-component array, applying
  // the optional EDAX hexagonal-alignment (+30 degrees on phi2) and degrees-to-radians
  // conversions. Both use double-precision intermediates so the stored float32 values are the
  // correctly-rounded results (this also matches DREAM3D 6.5.171 bit-for-bit).
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    const auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(cellEnsembleAttributeMatrixPath.createChildPath(ebsdlib::CtfFile::CrystalStructures));
    // Read the phase values from the reader's buffer rather than the just-written Phases array:
    // the copy above was verbatim and range-validated, and this avoids streaming a second
    // DataStructure array (which may be out-of-core backed) through the loop.
    const auto* cellPhases = static_cast<const int32*>(phaseColumnPtr);

    const auto* fComp0 = static_cast<const float32*>(euler1Ptr);
    const auto* fComp1 = static_cast<const float32*>(euler2Ptr);
    const auto* fComp2 = static_cast<const float32*>(euler3Ptr);

    auto& cellEulerAngles = m_DataStructure.getDataRefAs<Float32Array>(cellAttributeMatrixPath.createChildPath(ebsdlib::CtfFile::EulerAngles));
    for(usize i = 0; i < totalCells; i++)
    {
      float32 euler1 = fComp0[i];
      float32 euler2 = fComp1[i];
      float32 euler3 = fComp2[i];
      // See the filter documentation for this correction factor: Oxford Instruments aligns the
      // hexagonal crystal x-axis with [10-10] while DREAM3D-NX follows the EDAX/TSL convention
      // of aligning it with [2-1-10], a 30 degree rotation about [0001] applied to phi2.
      if(crystalStructures[cellPhases[i]] == ebsdlib::CrystalStructure::Hexagonal_High && m_InputValues->EdaxHexagonalAlignment)
      {
        euler3 = static_cast<float32>(static_cast<float64>(euler3) + 30.0);
      }
      if(m_InputValues->DegreesToRadians)
      {
        euler1 = static_cast<float32>(static_cast<float64>(euler1) * ebsdlib::constants::k_PiOver180D);
        euler2 = static_cast<float32>(static_cast<float64>(euler2) * ebsdlib::constants::k_PiOver180D);
        euler3 = static_cast<float32>(static_cast<float64>(euler3) * ebsdlib::constants::k_PiOver180D);
      }
      cellEulerAngles[3 * i] = euler1;
      cellEulerAngles[3 * i + 1] = euler2;
      cellEulerAngles[3 * i + 2] = euler3;
    }
  }

  if(m_ShouldCancel)
  {
    return {};
  }

  // The remaining columns are copied verbatim.
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
      std::copy(sourcePtr, sourcePtr + totalCells, targetArray.begin());
    }
    else
    {
      const auto* sourcePtr = static_cast<const float32*>(columnPtr);
      auto& targetArray = m_DataStructure.getDataRefAs<Float32Array>(targetPath);
      std::copy(sourcePtr, sourcePtr + totalCells, targetArray.begin());
    }
  }

  return {};
}
