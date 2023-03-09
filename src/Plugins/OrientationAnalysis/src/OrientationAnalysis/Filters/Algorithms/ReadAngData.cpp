#include "ReadAngData.hpp"

#include "complex/Common/RgbColor.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Utilities/Math/MatrixMath.hpp"

#include "EbsdLib/Core/Orientation.hpp"

using namespace complex;

using FloatVec3Type = std::vector<float>;

// -----------------------------------------------------------------------------
ReadAngData::ReadAngData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadAngDataInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ReadAngData::~ReadAngData() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ReadAngData::operator()()
{
  AngReader reader;
  reader.setFileName(m_InputValues->InputFile.string());
  int32_t err = reader.readFile();
  if(err < 0)
  {
    return {MakeErrorResult<>(reader.getErrorCode(), reader.getErrorMessage())};
  }

  auto result = loadMaterialInfo(&reader);
  if(result.first < 0)
  {
    return {MakeErrorResult<>(result.first, result.second)};
  }

  copyRawEbsdData(&reader);

  return {};
}

// -----------------------------------------------------------------------------
std::pair<int32, std::string> ReadAngData::loadMaterialInfo(AngReader* reader) const
{
  std::vector<AngPhase::Pointer> phases = reader->getPhaseVector();
  if(phases.empty())
  {
    return {reader->getErrorCode(), reader->getErrorMessage()};
  }

  DataPath CellEnsembleAttributeMatrixPath = m_InputValues->DataContainerName.createChildPath(m_InputValues->CellEnsembleAttributeMatrixName);

  auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(CellEnsembleAttributeMatrixPath.createChildPath(EbsdLib::AngFile::CrystalStructures));

  auto& materialNames = m_DataStructure.getDataRefAs<Int8Array>(CellEnsembleAttributeMatrixPath.createChildPath(EbsdLib::AngFile::MaterialName));
  materialNames.fill(0); // ensure the strings are all null terminated by splatting 0 across all the values.
  auto& latticeConstants = m_DataStructure.getDataRefAs<Float32Array>(CellEnsembleAttributeMatrixPath.createChildPath(EbsdLib::AngFile::LatticeConstants));

  std::string k_InvalidPhase = "Invalid Phase";

  // Initialize the zero'th element to unknowns. The other elements will
  // be filled in based on values from the data file
  crystalStructures[0] = EbsdLib::CrystalStructure::UnknownCrystalStructure;
  for(size_t i = 0; i < k_InvalidPhase.size(); i++)
  {
    materialNames.getDataStoreRef().setComponent(0, i, k_InvalidPhase[i]);
  }
  for(size_t i = 0; i < 6; i++)
  {
    latticeConstants.getDataStoreRef().setComponent(0, i, 0.0F);
  }

  for(const AngPhase::Pointer& phase : phases)
  {
    int32_t phaseID = phase->getPhaseIndex();
    crystalStructures[phaseID] = phase->determineLaueGroup();
    std::string materialName = phase->getMaterialName();
    for(size_t i = 0; i < materialName.size(); i++)
    {
      materialNames.getDataStoreRef().setComponent(0, i, materialName[i]);
    }

    std::vector<float> lattConst = phase->getLatticeConstants();

    for(size_t i = 0; i < 6; i++)
    {
      latticeConstants.getDataStoreRef().setComponent(phaseID, i, lattConst[i]);
    }
  }
  return {0, ""};
}

// -----------------------------------------------------------------------------
void ReadAngData::copyRawEbsdData(AngReader* reader) const
{
  DataPath CellAttributeMatrixPath = m_InputValues->DataContainerName.createChildPath(m_InputValues->CellAttributeMatrixName);

  std::vector<size_t> cDims = {1};

  auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->DataContainerName);
  size_t totalCells = imageGeom.getNumberOfCells();

  // Prepare the Cell Attribute Matrix with the correct number of tuples based on the total Cells being read from the file.
  std::vector<size_t> tDims = {imageGeom.getNumXCells(), imageGeom.getNumYCells(), imageGeom.getNumZCells()};

  // Adjust the values of the 'phase' data to correct for invalid values and assign the read Phase Data into the actual DataArray
  {
    auto& targetArray = m_DataStructure.getDataRefAs<Int32Array>(CellAttributeMatrixPath.createChildPath(EbsdLib::AngFile::Phases));
    int* phasePtr = reinterpret_cast<int32_t*>(reader->getPointerByName(EbsdLib::Ang::PhaseData));
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
    auto* fComp0 = reinterpret_cast<float*>(reader->getPointerByName(EbsdLib::Ang::Phi1));
    auto* fComp1 = reinterpret_cast<float*>(reader->getPointerByName(EbsdLib::Ang::Phi));
    auto* fComp2 = reinterpret_cast<float*>(reader->getPointerByName(EbsdLib::Ang::Phi2));
    cDims[0] = 3;

    auto& cellEulerAngles = m_DataStructure.getDataRefAs<Float32Array>(CellAttributeMatrixPath.createChildPath(EbsdLib::AngFile::EulerAngles));
    for(size_t i = 0; i < totalCells; i++)
    {
      cellEulerAngles[3 * i] = fComp0[i];
      cellEulerAngles[3 * i + 1] = fComp1[i];
      cellEulerAngles[3 * i + 2] = fComp2[i];
    }
  }

  cDims[0] = 1;
  {
    auto* fComp0 = reinterpret_cast<float*>(reader->getPointerByName(EbsdLib::Ang::ImageQuality));
    auto& targetArray = m_DataStructure.getDataRefAs<Float32Array>(CellAttributeMatrixPath.createChildPath(EbsdLib::Ang::ImageQuality));
    std::copy(fComp0, fComp0 + totalCells, targetArray.begin());
  }

  {
    auto* fComp0 = reinterpret_cast<float*>(reader->getPointerByName(EbsdLib::Ang::ConfidenceIndex));
    auto& targetArray = m_DataStructure.getDataRefAs<Float32Array>(CellAttributeMatrixPath.createChildPath(EbsdLib::Ang::ConfidenceIndex));
    std::copy(fComp0, fComp0 + totalCells, targetArray.begin());
  }

  {
    auto* fComp0 = reinterpret_cast<float*>(reader->getPointerByName(EbsdLib::Ang::SEMSignal));
    auto& targetArray = m_DataStructure.getDataRefAs<Float32Array>(CellAttributeMatrixPath.createChildPath(EbsdLib::Ang::SEMSignal));
    std::copy(fComp0, fComp0 + totalCells, targetArray.begin());
  }

  {
    auto* fComp0 = reinterpret_cast<float*>(reader->getPointerByName(EbsdLib::Ang::Fit));
    auto& targetArray = m_DataStructure.getDataRefAs<Float32Array>(CellAttributeMatrixPath.createChildPath(EbsdLib::Ang::Fit));
    std::copy(fComp0, fComp0 + totalCells, targetArray.begin());
  }

  {
    auto* fComp0 = reinterpret_cast<float*>(reader->getPointerByName(EbsdLib::Ang::XPosition));
    auto& targetArray = m_DataStructure.getDataRefAs<Float32Array>(CellAttributeMatrixPath.createChildPath(EbsdLib::Ang::XPosition));
    std::copy(fComp0, fComp0 + totalCells, targetArray.begin());
  }

  {
    auto* fComp0 = reinterpret_cast<float*>(reader->getPointerByName(EbsdLib::Ang::YPosition));
    auto& targetArray = m_DataStructure.getDataRefAs<Float32Array>(CellAttributeMatrixPath.createChildPath(EbsdLib::Ang::YPosition));
    std::copy(fComp0, fComp0 + totalCells, targetArray.begin());
  }
}
