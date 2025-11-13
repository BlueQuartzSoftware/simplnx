#include "ReadChannel5Data.hpp"

#include "simplnx/Common/RgbColor.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Utilities/Math/MatrixMath.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <EbsdLib/Core/Orientation.hpp>

using namespace nx::core;

using FloatVec3Type = std::vector<float>;

namespace
{
template <typename T>
void copyRawData(const ReadChannel5DataInputValues* m_InputValues, size_t numElements, DataStructure& m_DataStructure, ebsdlib::CprReader& m_Reader, const std::string& name, DataPath& dataArrayPath)
{
  using ArrayType = DataArray<T>;
  auto& dataRef = m_DataStructure.getDataRefAs<ArrayType>(dataArrayPath);
  auto* dataStorePtr = dataRef.getDataStore();

  const nonstd::span<T> rawDataPtr(reinterpret_cast<T*>(m_Reader.getPointerByName(name)), numElements);
  // std::copy(rawDataPtr.begin(), rawDataPtr.end(), dataStorePtr->begin() + offset);
  for(size_t idx = 0; idx < numElements; idx++)
  {
    dataStorePtr->setValue(idx, rawDataPtr[idx]);
  }
}

} // namespace

// -----------------------------------------------------------------------------
ReadChannel5Data::ReadChannel5Data(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, ReadChannel5DataInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_MessageHandler(msgHandler)
, m_ShouldCancel(shouldCancel)
, m_InputValues(inputValues)
{
}

// -----------------------------------------------------------------------------
ReadChannel5Data::~ReadChannel5Data() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ReadChannel5Data::operator()()
{
  ebsdlib::CprReader reader;
  reader.setFileName(m_InputValues->InputFile.string());
  const int32_t err = reader.readFile();
  if(err < 0)
  {
    return MakeErrorResult(reader.getErrorCode(), reader.getErrorMessage());
  }
  if(m_ShouldCancel)
  {
    return {};
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
std::pair<int32, std::string> ReadChannel5Data::loadMaterialInfo(ebsdlib::CprReader* reader) const
{
  const std::vector<ebsdlib::CtfPhase::Pointer> phases = reader->getPhaseVector();
  if(phases.empty())
  {
    return {reader->getErrorCode(), reader->getErrorMessage()};
  }

  const DataPath cellEnsembleAttributeMatrixPath = m_InputValues->DataContainerName.createChildPath(m_InputValues->CellEnsembleAttributeMatrixName);

  auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(cellEnsembleAttributeMatrixPath.createChildPath(ebsdlib::CtfFile::CrystalStructures));

  auto& materialNames = m_DataStructure.getDataRefAs<StringArray>(cellEnsembleAttributeMatrixPath.createChildPath(ebsdlib::CtfFile::MaterialName));

  auto& latticeConstants = m_DataStructure.getDataRefAs<Float32Array>(cellEnsembleAttributeMatrixPath.createChildPath(ebsdlib::CtfFile::LatticeConstants));

  const std::string invalidPhase = "Invalid Phase";

  // Initialize the zero'th element to unknowns. The other elements will
  // be filled in based on values from the data file
  crystalStructures[0] = ebsdlib::CrystalStructure::UnknownCrystalStructure;
  materialNames[0] = invalidPhase;

  for(size_t i = 0; i < 6; i++)
  {
    latticeConstants.getDataStoreRef().setComponent(0, i, 0.0F);
  }

  for(const ebsdlib::CtfPhase::Pointer& phase : phases)
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
void ReadChannel5Data::copyRawEbsdData(ebsdlib::CprReader* reader) const
{
  const DataPath cellAttributeMatrixPath = m_InputValues->DataContainerName.createChildPath(m_InputValues->CellAttributeMatrixName);

  std::vector<size_t> cDims = {1};

  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->DataContainerName);
  const size_t totalCells = imageGeom.getNumberOfCells();

  // Prepare the Cell Attribute Matrix with the correct number of tuples based on the total Cells being read from the file.
  const std::vector<size_t> tDims = {imageGeom.getNumXCells(), imageGeom.getNumYCells(), imageGeom.getNumZCells()};

  const std::vector<ebsdlib::CrcDataParser> fieldParsers = reader->createFieldParsers(m_InputValues->InputFile.string());
  for(const auto& parser : fieldParsers)
  {
    if(m_ShouldCancel)
    {
      return;
    }

    const std::string fieldName = parser.FieldDefinition.FieldName;
    DataPath dataArrayPath = cellAttributeMatrixPath.createChildPath(fieldName);

    if(parser.FieldDefinition.numericType == ebsdlib::NumericTypes::Type::Int32)
    {
      copyRawData<int32_t>(m_InputValues, totalCells, m_DataStructure, *reader, fieldName, dataArrayPath);
    }
    else if(parser.FieldDefinition.numericType == ebsdlib::NumericTypes::Type::Float)
    {
      copyRawData<float32>(m_InputValues, totalCells, m_DataStructure, *reader, fieldName, dataArrayPath);
    }
    else if(parser.FieldDefinition.numericType == ebsdlib::NumericTypes::Type::UInt8)
    {
      copyRawData<uint8_t>(m_InputValues, totalCells, m_DataStructure, *reader, fieldName, dataArrayPath);
    }
  }

  // Copy the data from the 'Phase' array into the 'Phases' array
  if(m_ShouldCancel)
  {
    return;
  }
  if(m_InputValues->CreateCompatibleArrays)
  {
    auto& targetArray = m_DataStructure.getDataRefAs<Int32Array>(cellAttributeMatrixPath.createChildPath(ebsdlib::CtfFile::Phases));
    auto* phasePtr = reinterpret_cast<uint8_t*>(reader->getPointerByName(ebsdlib::Ctf::Phase));
    for(size_t i = 0; i < totalCells; i++)
    {
      targetArray[i] = phasePtr[i];
    }
  }

  // Condense the Euler Angles from 3 separate arrays into a single 1x3 array
  if(m_ShouldCancel)
  {
    return;
  }
  if(m_InputValues->CreateCompatibleArrays)
  {
    const auto* fComp0Ptr = reinterpret_cast<float*>(reader->getPointerByName(ebsdlib::Ctf::phi1));
    const auto* fComp1Ptr = reinterpret_cast<float*>(reader->getPointerByName(ebsdlib::Ctf::Phi));
    const auto* fComp2Ptr = reinterpret_cast<float*>(reader->getPointerByName(ebsdlib::Ctf::phi2));
    cDims[0] = 3;

    auto& cellEulerAngles = m_DataStructure.getDataRefAs<Float32Array>(cellAttributeMatrixPath.createChildPath(ebsdlib::CtfFile::EulerAngles));
    for(size_t i = 0; i < totalCells; i++)
    {
      cellEulerAngles[3 * i] = fComp0Ptr[i];
      cellEulerAngles[3 * i + 1] = fComp1Ptr[i];
      cellEulerAngles[3 * i + 2] = fComp2Ptr[i];
    }
  }
}
