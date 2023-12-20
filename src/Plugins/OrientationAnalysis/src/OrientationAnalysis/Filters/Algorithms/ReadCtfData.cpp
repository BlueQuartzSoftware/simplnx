#include "ReadCtfData.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/StringArray.hpp"

#include "EbsdLib/IO/HKL/CtfConstants.h"
#include "EbsdLib/Math/EbsdLibMath.h"

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
  CtfReader reader;
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
std::pair<int32, std::string> ReadCtfData::loadMaterialInfo(CtfReader* reader) const
{
  const DataPath cellEnsembleAttributeMatrixPath = m_InputValues->DataContainerName.createChildPath(m_InputValues->CellEnsembleAttributeMatrixName);

  const std::vector<CtfPhase::Pointer> phases = reader->getPhaseVector();
  if(phases.empty())
  {
    return {reader->getErrorCode(), reader->getErrorMessage()};
  }

  auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(cellEnsembleAttributeMatrixPath.createChildPath(EbsdLib::CtfFile::CrystalStructures));

  auto& materialNames = m_DataStructure.getDataRefAs<StringArray>(cellEnsembleAttributeMatrixPath.createChildPath(EbsdLib::CtfFile::MaterialName));
  auto& latticeConstants = m_DataStructure.getDataRefAs<Float32Array>(cellEnsembleAttributeMatrixPath.createChildPath(EbsdLib::CtfFile::LatticeConstants));

  const std::string k_InvalidPhase = "Invalid Phase";

  // Initialize the zero'th element to unknowns. The other elements will
  // be filled in based on values from the data file
  crystalStructures[0] = EbsdLib::CrystalStructure::UnknownCrystalStructure;
  materialNames[0] = k_InvalidPhase;

  for(size_t i = 0; i < 6; i++)
  {
    latticeConstants.getDataStoreRef().setComponent(0, i, 0.0F);
  }

  for(const CtfPhase::Pointer& phase : phases)
  {
    const int32_t phaseID = phase->getPhaseIndex();
    crystalStructures[phaseID] = phase->determineLaueGroup();
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
void ReadCtfData::copyRawEbsdData(CtfReader* reader) const
{
  const DataPath cellAttributeMatrixPath = m_InputValues->DataContainerName.createChildPath(m_InputValues->CellAttributeMatrixName);
  const DataPath cellEnsembleAttributeMatrixPath = m_InputValues->DataContainerName.createChildPath(m_InputValues->CellEnsembleAttributeMatrixName);
  std::vector<size_t> cDims = {1};

  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->DataContainerName);
  const size_t totalCells = imageGeom.getNumberOfCells();

  // Prepare the Cell Attribute Matrix with the correct number of tuples based on the total Cells being read from the file.
  std::vector<size_t> tDims = {imageGeom.getNumXCells(), imageGeom.getNumYCells(), imageGeom.getNumZCells()};

  // Adjust the values of the 'phase' data to correct for invalid values and assign the read Phase Data into the actual DataArray
  {
    /* Take from H5CtfVolumeReader.cpp
     * For HKL OIM Files if there is a single phase then the value of the phase
     * data is one (1). If there are 2 or more phases, the lowest value
     * of phase is also one (1). However, if there are "zero solutions" in the data
     * then those Cells are assigned a phase of zero.  Since those Cells can be identified
     * by other methods, the phase of these Cells should be changed to one since in the rest
     * of the reconstruction code we follow the convention that the lowest value is One (1)
     * even if there is only a single phase. The next if statement converts all zeros to ones
     * if there is a single phase in the OIM data.
     */
    auto& targetArray = m_DataStructure.getDataRefAs<Int32Array>(cellAttributeMatrixPath.createChildPath(EbsdLib::CtfFile::Phases));
    int* phasePtr = reinterpret_cast<int32_t*>(reader->getPointerByName(EbsdLib::Ctf::Phase));
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
    auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(cellEnsembleAttributeMatrixPath.createChildPath(EbsdLib::CtfFile::CrystalStructures));
    auto& cellPhases = m_DataStructure.getDataRefAs<Int32Array>(cellAttributeMatrixPath.createChildPath(EbsdLib::CtfFile::Phases));

    const auto* fComp0 = reinterpret_cast<float*>(reader->getPointerByName(EbsdLib::Ctf::Euler1));
    const auto* fComp1 = reinterpret_cast<float*>(reader->getPointerByName(EbsdLib::Ctf::Euler2));
    const auto* fComp2 = reinterpret_cast<float*>(reader->getPointerByName(EbsdLib::Ctf::Euler3));
    cDims[0] = 3;

    auto& cellEulerAngles = m_DataStructure.getDataRefAs<Float32Array>(cellAttributeMatrixPath.createChildPath(EbsdLib::CtfFile::EulerAngles));
    for(size_t i = 0; i < totalCells; i++)
    {
      cellEulerAngles[3 * i] = fComp0[i];
      cellEulerAngles[3 * i + 1] = fComp1[i];
      cellEulerAngles[3 * i + 2] = fComp2[i];
      if(crystalStructures[cellPhases[i]] == EbsdLib::CrystalStructure::Hexagonal_High && m_InputValues->EdaxHexagonalAlignment)
      {
        cellEulerAngles[3 * i + 2] = cellEulerAngles[3 * i + 2] + 30.0F; // See the documentation for this correction factor
      }
      // Now convert to radians if requested by the user
      if(m_InputValues->DegreesToRadians)
      {
        cellEulerAngles[3 * i] = cellEulerAngles[3 * i] * EbsdLib::Constants::k_PiOver180F;
        cellEulerAngles[3 * i + 1] = cellEulerAngles[3 * i + 1] * EbsdLib::Constants::k_PiOver180F;
        cellEulerAngles[3 * i + 2] = cellEulerAngles[3 * i + 2] * EbsdLib::Constants::k_PiOver180F;
      }
    }
  }

  cDims[0] = 1;
  {
    auto* fComp0 = reinterpret_cast<int32*>(reader->getPointerByName(EbsdLib::Ctf::Bands));
    auto& targetArray = m_DataStructure.getDataRefAs<Int32Array>(cellAttributeMatrixPath.createChildPath(EbsdLib::Ctf::Bands));
    std::copy(fComp0, fComp0 + totalCells, targetArray.begin());
  }

  {
    auto* fComp0 = reinterpret_cast<int32*>(reader->getPointerByName(EbsdLib::Ctf::Error));
    auto& targetArray = m_DataStructure.getDataRefAs<Int32Array>(cellAttributeMatrixPath.createChildPath(EbsdLib::Ctf::Error));
    std::copy(fComp0, fComp0 + totalCells, targetArray.begin());
  }

  {
    auto* fComp0 = reinterpret_cast<float*>(reader->getPointerByName(EbsdLib::Ctf::MAD));
    auto& targetArray = m_DataStructure.getDataRefAs<Float32Array>(cellAttributeMatrixPath.createChildPath(EbsdLib::Ctf::MAD));
    std::copy(fComp0, fComp0 + totalCells, targetArray.begin());
  }

  {
    auto* fComp0 = reinterpret_cast<int32*>(reader->getPointerByName(EbsdLib::Ctf::BC));
    auto& targetArray = m_DataStructure.getDataRefAs<Int32Array>(cellAttributeMatrixPath.createChildPath(EbsdLib::Ctf::BC));
    std::copy(fComp0, fComp0 + totalCells, targetArray.begin());
  }

  {
    auto* fComp0 = reinterpret_cast<int32*>(reader->getPointerByName(EbsdLib::Ctf::BS));
    auto& targetArray = m_DataStructure.getDataRefAs<Int32Array>(cellAttributeMatrixPath.createChildPath(EbsdLib::Ctf::BS));
    std::copy(fComp0, fComp0 + totalCells, targetArray.begin());
  }

  {
    auto* fComp0 = reinterpret_cast<float*>(reader->getPointerByName(EbsdLib::Ctf::X));
    auto& targetArray = m_DataStructure.getDataRefAs<Float32Array>(cellAttributeMatrixPath.createChildPath(EbsdLib::Ctf::X));
    std::copy(fComp0, fComp0 + totalCells, targetArray.begin());
  }

  {
    auto* fComp0 = reinterpret_cast<float*>(reader->getPointerByName(EbsdLib::Ctf::Y));
    auto& targetArray = m_DataStructure.getDataRefAs<Float32Array>(cellAttributeMatrixPath.createChildPath(EbsdLib::Ctf::Y));
    std::copy(fComp0, fComp0 + totalCells, targetArray.begin());
  }
}
