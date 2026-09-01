#include "ReadChannel5Data.hpp"

#include "simplnx/Common/RgbColor.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <EbsdLib/Core/Orientation.hpp>

#include <algorithm>
#include <memory>

using namespace nx::core;

namespace
{
constexpr usize k_ChunkTuples = 65536;

template <typename T>
bool CopyRawData(usize numElements, DataStructure& dataStructure, ebsdlib::CprReader& reader, const std::string& name, const DataPath& dataArrayPath, const std::atomic_bool& shouldCancel)
{
  auto& dataStore = dataStructure.getDataRefAs<DataArray<T>>(dataArrayPath).getDataStoreRef();
  const auto* rawData = reinterpret_cast<const T*>(reader.getPointerByName(name));

  // The reader already owns the complete source array. Limit each DataStore transfer
  // without allocating a second cell-sized buffer.
  for(usize offset = 0; offset < numElements; offset += k_ChunkTuples)
  {
    if(shouldCancel)
    {
      return false;
    }

    const usize count = std::min(k_ChunkTuples, numElements - offset);
    dataStore.copyFromBuffer(offset, nonstd::span<const T>(rawData + offset, count));
  }

  return true;
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

  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->DataContainerName);
  const usize totalCells = imageGeom.getNumberOfCells();
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
      if(!CopyRawData<int32>(totalCells, m_DataStructure, *reader, fieldName, dataArrayPath, m_ShouldCancel))
      {
        return;
      }
    }
    else if(parser.FieldDefinition.numericType == ebsdlib::NumericTypes::Type::Float)
    {
      if(!CopyRawData<float32>(totalCells, m_DataStructure, *reader, fieldName, dataArrayPath, m_ShouldCancel))
      {
        return;
      }
    }
    else if(parser.FieldDefinition.numericType == ebsdlib::NumericTypes::Type::UInt8)
    {
      if(!CopyRawData<uint8>(totalCells, m_DataStructure, *reader, fieldName, dataArrayPath, m_ShouldCancel))
      {
        return;
      }
    }
  }

  if(m_InputValues->CreateCompatibleArrays)
  {
    auto& targetArray = m_DataStructure.getDataRefAs<Int32Array>(cellAttributeMatrixPath.createChildPath(ebsdlib::CtfFile::Phases));
    auto& phaseStore = targetArray.getDataStoreRef();
    const auto* phasePtr = reinterpret_cast<const uint8*>(reader->getPointerByName(ebsdlib::Ctf::Phase));
    auto phaseBuffer = std::make_unique<int32[]>(k_ChunkTuples);

    for(usize offset = 0; offset < totalCells; offset += k_ChunkTuples)
    {
      if(m_ShouldCancel)
      {
        return;
      }

      const usize count = std::min(k_ChunkTuples, totalCells - offset);
      for(usize i = 0; i < count; i++)
      {
        phaseBuffer[i] = phasePtr[offset + i];
      }
      phaseStore.copyFromBuffer(offset, nonstd::span<const int32>(phaseBuffer.get(), count));
    }
  }

  if(m_InputValues->CreateCompatibleArrays)
  {
    const auto* fComp0Ptr = reinterpret_cast<const float32*>(reader->getPointerByName(ebsdlib::Ctf::phi1));
    const auto* fComp1Ptr = reinterpret_cast<const float32*>(reader->getPointerByName(ebsdlib::Ctf::Phi));
    const auto* fComp2Ptr = reinterpret_cast<const float32*>(reader->getPointerByName(ebsdlib::Ctf::phi2));

    auto& cellEulerAngles = m_DataStructure.getDataRefAs<Float32Array>(cellAttributeMatrixPath.createChildPath(ebsdlib::CtfFile::EulerAngles));
    auto& eulerStore = cellEulerAngles.getDataStoreRef();
    auto eulerBuffer = std::make_unique<float32[]>(k_ChunkTuples * 3);

    for(usize offset = 0; offset < totalCells; offset += k_ChunkTuples)
    {
      if(m_ShouldCancel)
      {
        return;
      }

      const usize count = std::min(k_ChunkTuples, totalCells - offset);
      for(usize i = 0; i < count; i++)
      {
        eulerBuffer[3 * i] = fComp0Ptr[offset + i];
        eulerBuffer[3 * i + 1] = fComp1Ptr[offset + i];
        eulerBuffer[3 * i + 2] = fComp2Ptr[offset + i];
      }
      eulerStore.copyFromBuffer(offset * 3, nonstd::span<const float32>(eulerBuffer.get(), count * 3));
    }
  }
}
