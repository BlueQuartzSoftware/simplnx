#include "ReadAngData.hpp"

#include "simplnx/Common/RgbColor.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <EbsdLib/Core/Orientation.hpp>

#include <fmt/format.h>

using namespace nx::core;

// -----------------------------------------------------------------------------
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
Result<> ReadAngData::operator()()
{
  m_MessageHandler.sendInfoMessage(fmt::format("Reading .ang file '{}'", m_InputValues->InputFile.string()));
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

  m_MessageHandler.sendInfoMessage("Copying cell data into the Image Geometry");
  return copyRawEbsdData(&reader);
}

// -----------------------------------------------------------------------------
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

  // Initialize EVERY slot to the "Invalid Phase" defaults first. Slot 0 is always the
  // invalid phase, and any slot not covered by a phase section in the file (possible when
  // the file's phase indices are not contiguous) keeps these defaults instead of
  // zero-initialized garbage.
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
    // .ang phase numbering starts at 1. A phase index < 1 (Phase 0 or negative) is rejected: DREAM3D
    // 6.5.171 tolerated a Phase 0 section (it skipped only negative indices and wrote Phase 0 into
    // ensemble slot 0), so this is a documented behavior change vs legacy — see deviation
    // ReadAngDataFilter-D5. A static "# Phase 0" fixture trips this deterministically.
    if(phaseID < 1)
    {
      return MakeErrorResult(-19502, fmt::format("The .ang file declares phase index {}, but .ang phase numbering starts at 1 (Phase 0 and negative phases are not supported).", phaseID));
    }
    // The ensemble arrays were sized in preflight from the same file's largest phase index, so an index
    // at or above the array count can only mean the file changed between preflight and execute.
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

// -----------------------------------------------------------------------------
Result<> ReadAngData::copyRawEbsdData(ebsdlib::AngReader* reader) const
{
  const DataPath cellAttributeMatrixPath = m_InputValues->DataContainerName.createChildPath(m_InputValues->CellAttributeMatrixName);

  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->DataContainerName);
  const size_t totalCells = imageGeom.getNumberOfCells();

  // The Image Geometry was sized in preflight from the file's column/row header (NumEvenCols x NumRows),
  // but the reader allocates its data buffers from NumOddCols x NumRows. Every copy below reads
  // totalCells elements out of those buffers, so if the reader actually read fewer elements (a file that
  // changed between preflight and execute, or a malformed header where NCOLS_EVEN > NCOLS_ODD) the copies
  // would read past the end of the reader's heap buffers. Guard against that out-of-bounds read.
  if(reader->getNumberOfElements() < totalCells)
  {
    return MakeErrorResult(-19503,
                           fmt::format("The .ang reader produced {} scan points but the Image Geometry created at preflight expects {}. The input file may have changed since preflight, or its "
                                       "column header (NCOLS_ODD/NCOLS_EVEN) is inconsistent.",
                                       reader->getNumberOfElements(), totalCells));
  }

  // Adjust the values of the 'phase' data to correct for invalid values and assign the read Phase Data into the actual DataArray
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    auto& targetArray = m_DataStructure.getDataRefAs<Int32Array>(cellAttributeMatrixPath.createChildPath(ebsdlib::AngFile::Phases));
    int* phasePtr = reinterpret_cast<int32_t*>(reader->getPointerByName(ebsdlib::Ang::PhaseData));
    for(size_t i = 0; i < totalCells; i++)
    {
      if(phasePtr[i] < 1)
      {
        phasePtr[i] = 1;
      }
      targetArray[i] = phasePtr[i];
    }
  }

  // Condense the Euler Angles from 3 separate arrays into a single 1x3 array
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    const auto* fComp0 = reinterpret_cast<float*>(reader->getPointerByName(ebsdlib::Ang::Phi1));
    const auto* fComp1 = reinterpret_cast<float*>(reader->getPointerByName(ebsdlib::Ang::Phi));
    const auto* fComp2 = reinterpret_cast<float*>(reader->getPointerByName(ebsdlib::Ang::Phi2));

    auto& cellEulerAngles = m_DataStructure.getDataRefAs<Float32Array>(cellAttributeMatrixPath.createChildPath(ebsdlib::AngFile::EulerAngles));
    for(size_t i = 0; i < totalCells; i++)
    {
      cellEulerAngles[3 * i] = fComp0[i];
      cellEulerAngles[3 * i + 1] = fComp1[i];
      cellEulerAngles[3 * i + 2] = fComp2[i];
    }
  }

  if(m_ShouldCancel)
  {
    return {};
  }

  {
    auto* fComp0 = reinterpret_cast<float*>(reader->getPointerByName(ebsdlib::Ang::ImageQuality));
    auto& targetArray = m_DataStructure.getDataRefAs<Float32Array>(cellAttributeMatrixPath.createChildPath(ebsdlib::Ang::ImageQuality));
    std::copy(fComp0, fComp0 + totalCells, targetArray.begin());
  }

  {
    auto* fComp0 = reinterpret_cast<float*>(reader->getPointerByName(ebsdlib::Ang::ConfidenceIndex));
    auto& targetArray = m_DataStructure.getDataRefAs<Float32Array>(cellAttributeMatrixPath.createChildPath(ebsdlib::Ang::ConfidenceIndex));
    std::copy(fComp0, fComp0 + totalCells, targetArray.begin());
  }

  {
    auto* fComp0 = reinterpret_cast<float*>(reader->getPointerByName(ebsdlib::Ang::SEMSignal));
    auto& targetArray = m_DataStructure.getDataRefAs<Float32Array>(cellAttributeMatrixPath.createChildPath(ebsdlib::Ang::SEMSignal));
    std::copy(fComp0, fComp0 + totalCells, targetArray.begin());
  }

  {
    auto* fComp0 = reinterpret_cast<float*>(reader->getPointerByName(ebsdlib::Ang::Fit));
    auto& targetArray = m_DataStructure.getDataRefAs<Float32Array>(cellAttributeMatrixPath.createChildPath(ebsdlib::Ang::Fit));
    std::copy(fComp0, fComp0 + totalCells, targetArray.begin());
  }

  {
    auto* fComp0 = reinterpret_cast<float*>(reader->getPointerByName(ebsdlib::Ang::XPosition));
    auto& targetArray = m_DataStructure.getDataRefAs<Float32Array>(cellAttributeMatrixPath.createChildPath(ebsdlib::Ang::XPosition));
    std::copy(fComp0, fComp0 + totalCells, targetArray.begin());
  }

  {
    auto* fComp0 = reinterpret_cast<float*>(reader->getPointerByName(ebsdlib::Ang::YPosition));
    auto& targetArray = m_DataStructure.getDataRefAs<Float32Array>(cellAttributeMatrixPath.createChildPath(ebsdlib::Ang::YPosition));
    std::copy(fComp0, fComp0 + totalCells, targetArray.begin());
  }

  return {};
}
