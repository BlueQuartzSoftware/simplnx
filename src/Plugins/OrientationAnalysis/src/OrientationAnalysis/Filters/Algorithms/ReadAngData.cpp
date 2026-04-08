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
void ReadAngData::copyRawEbsdData(ebsdlib::AngReader* reader) const
{
  const DataPath CellAttributeMatrixPath = m_InputValues->DataContainerName.createChildPath(m_InputValues->CellAttributeMatrixName);

  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->DataContainerName);
  const usize totalCells = imageGeom.getNumberOfCells();

  // Prepare the Cell Attribute Matrix with the correct number of tuples based on the total Cells being read from the file.
  std::vector<usize> tDims = {imageGeom.getNumXCells(), imageGeom.getNumYCells(), imageGeom.getNumZCells()};

  // Adjust the values of the 'phase' data to correct for invalid values and assign the read Phase Data into the actual DataArray
  {
    if(m_ShouldCancel)
    {
      return;
    }
    auto& targetArray = m_DataStructure.getDataRefAs<Int32Array>(CellAttributeMatrixPath.createChildPath(ebsdlib::AngFile::Phases));
    auto* phasePtr = reinterpret_cast<int32*>(reader->getPointerByName(ebsdlib::Ang::PhaseData));
    // Validate phases in-place (original code also modifies phasePtr)
    for(usize i = 0; i < totalCells; i++)
    {
      if(phasePtr[i] < 1)
      {
        phasePtr[i] = 1;
      }
    }
    targetArray.getDataStoreRef().copyFromBuffer(0, nonstd::span<const int32>(phasePtr, totalCells));
  }

  // Condense the Euler Angles from 3 separate arrays into a single 1x3 array
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
