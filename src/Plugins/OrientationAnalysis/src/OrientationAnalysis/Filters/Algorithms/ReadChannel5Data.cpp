#include "ReadChannel5Data.hpp"

#include "EbsdStepMetadata.hpp"

#include "OrientationAnalysis/utilities/EbsdReaderUtilities.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <algorithm>
#include <vector>

using namespace nx::core;

namespace
{
constexpr usize k_ChunkTuples = 65536;

/**
 * @brief Copies one parsed Channel 5 array through bounded store writes.
 * @tparam T Array value type.
 * @param numElements Number of values to copy.
 * @param dataStructure Contains the destination array.
 * @param reader Owns the parsed source values.
 * @param name Identifies the reader field.
 * @param dataArrayPath Identifies the destination array.
 * @param shouldCancel Signals cancellation between chunks.
 * @return True after all chunks copy, false after cancellation, or the first store write error.
 */
template <typename T>
Result<bool> CopyRawData(usize numElements, DataStructure& dataStructure, ebsdlib::CprReader& reader, const std::string& name, const DataPath& dataArrayPath, const std::atomic_bool& shouldCancel)
{
  auto& dataStore = dataStructure.getDataRefAs<DataArray<T>>(dataArrayPath).getDataStoreRef();
  const auto* rawDataPtr = reinterpret_cast<const T*>(reader.getPointerByName(name));

  // The reader already owns the complete source array. Limit each DataStore transfer
  // without allocating a second cell-sized buffer.
  for(usize offset = 0; offset < numElements; offset += k_ChunkTuples)
  {
    if(shouldCancel)
    {
      return {false};
    }

    const usize count = std::min(k_ChunkTuples, numElements - offset);
    if(Result<> ioResult = dataStore.copyFromBuffer(offset, nonstd::span<const T>(rawDataPtr + offset, count)); ioResult.invalid())
    {
      return ConvertInvalidResult<bool>(std::move(ioResult));
    }
  }

  return {true};
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

// -----------------------------------------------------------------------------
Result<> ReadChannel5Data::operator()()
{
  ebsdlib::CprReader reader;
  if(m_InputValues->spacing_override.has_value())
  {
    const auto& spacing = *m_InputValues->spacing_override;
    static_cast<void>(ebsd_step_metadata::Observe<ebsdlib::CprReader::CtfFloatHeaderType>(reader, ebsdlib::Ctf::GridDistX, spacing[0]));
    static_cast<void>(ebsd_step_metadata::Observe<ebsdlib::CprReader::CtfFloatHeaderType>(reader, ebsdlib::Ctf::GridDistY, spacing[1]));
  }
  reader.setFileName(m_InputValues->input_file.string());
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

  return copyRawEbsdData(&reader);
}

// -----------------------------------------------------------------------------
std::pair<int32, std::string> ReadChannel5Data::loadMaterialInfo(ebsdlib::CprReader* reader) const
{
  const std::vector<ebsdlib::CtfPhase::Pointer> phases = reader->getPhaseVector();
  if(phases.empty())
  {
    return {reader->getErrorCode(), reader->getErrorMessage()};
  }

  const DataPath cellEnsembleAttributeMatrixPath = m_InputValues->data_container_name.createChildPath(m_InputValues->cell_ensemble_attribute_matrix_name);

  auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32ArrayType>(cellEnsembleAttributeMatrixPath.createChildPath(ebsdlib::CtfFile::CrystalStructures));

  auto& materialNames = m_DataStructure.getDataRefAs<StringArray>(cellEnsembleAttributeMatrixPath.createChildPath(ebsdlib::CtfFile::MaterialName));

  auto& latticeConstants = m_DataStructure.getDataRefAs<Float32ArrayType>(cellEnsembleAttributeMatrixPath.createChildPath(ebsdlib::CtfFile::LatticeConstants));

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
    materialName = nx::core::string_utilities::Replace(materialName, "MaterialName", "");
    materialName = nx::core::string_utilities::Trimmed(materialName);
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
Result<> ReadChannel5Data::copyRawEbsdData(ebsdlib::CprReader* reader) const
{
  const DataPath cellAttributeMatrixPath = m_InputValues->data_container_name.createChildPath(m_InputValues->cell_attribute_matrix_name);

  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->data_container_name);
  const usize totalCells = imageGeom.getNumberOfCells();
  const std::vector<ebsdlib::CrcDataParser> fieldParsers = reader->createFieldParsers(m_InputValues->input_file.string());

  for(const auto& parser : fieldParsers)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    const std::string fieldName = parser.FieldDefinition.FieldName;
    DataPath dataArrayPath = cellAttributeMatrixPath.createChildPath(fieldName);

    if(parser.FieldDefinition.numericType == ebsdlib::NumericTypes::Type::Int32)
    {
      Result<bool> copyResult = CopyRawData<int32>(totalCells, m_DataStructure, *reader, fieldName, dataArrayPath, m_ShouldCancel);
      if(copyResult.invalid())
      {
        return ConvertResult(std::move(copyResult));
      }
      if(!copyResult.value())
      {
        return {};
      }
    }
    else if(parser.FieldDefinition.numericType == ebsdlib::NumericTypes::Type::Float)
    {
      Result<bool> copyResult = CopyRawData<float32>(totalCells, m_DataStructure, *reader, fieldName, dataArrayPath, m_ShouldCancel);
      if(copyResult.invalid())
      {
        return ConvertResult(std::move(copyResult));
      }
      if(!copyResult.value())
      {
        return {};
      }
    }
    else if(parser.FieldDefinition.numericType == ebsdlib::NumericTypes::Type::UInt8)
    {
      Result<bool> copyResult = CopyRawData<uint8>(totalCells, m_DataStructure, *reader, fieldName, dataArrayPath, m_ShouldCancel);
      if(copyResult.invalid())
      {
        return ConvertResult(std::move(copyResult));
      }
      if(!copyResult.value())
      {
        return {};
      }
    }
  }

  const auto* rawPhasePtr = reinterpret_cast<const uint8*>(reader->getPointerByName(ebsdlib::Ctf::Phase));
  const auto* rawPhi2Ptr = reinterpret_cast<const float32*>(reader->getPointerByName(ebsdlib::Ctf::phi2));
  std::vector<uint32> crystalStructures;
  if(m_InputValues->edax_hexagonal_alignment)
  {
    const DataPath cellEnsembleAttributeMatrixPath = m_InputValues->data_container_name.createChildPath(m_InputValues->cell_ensemble_attribute_matrix_name);
    const auto& crystalStructuresRef = m_DataStructure.getDataRefAs<UInt32ArrayType>(cellEnsembleAttributeMatrixPath.createChildPath(ebsdlib::CtfFile::CrystalStructures));
    crystalStructures.resize(crystalStructuresRef.getSize());
    if(Result<> ioResult = crystalStructuresRef.getDataStoreRef().copyIntoBuffer(0, nonstd::span<uint32>(crystalStructures.data(), crystalStructures.size())); ioResult.invalid())
    {
      return ioResult;
    }
  }

  const auto alignedPhi2 = [this, &crystalStructures](uint8 phase, float32 phi2) -> float32 {
    if(m_InputValues->edax_hexagonal_alignment && phase < crystalStructures.size() && ebsd_reader_utilities::IsHexagonalCrystalStructure(crystalStructures[phase]))
    {
      // Channel 5 Euler angles are already in radians; retain the unwrapped result.
      return static_cast<float32>(static_cast<float64>(phi2) + ebsd_reader_utilities::k_HexagonalAlignmentRadiansD);
    }
    return phi2;
  };

  if(m_InputValues->edax_hexagonal_alignment)
  {
    auto& phi2StoreRef = m_DataStructure.getDataRefAs<Float32ArrayType>(cellAttributeMatrixPath.createChildPath(ebsdlib::Ctf::phi2)).getDataStoreRef();
    std::vector<float32> phi2Buffer(k_ChunkTuples);

    for(usize offset = 0; offset < totalCells; offset += k_ChunkTuples)
    {
      if(m_ShouldCancel)
      {
        return {};
      }

      const usize count = std::min(k_ChunkTuples, totalCells - offset);
      for(usize i = 0; i < count; i++)
      {
        phi2Buffer[i] = alignedPhi2(rawPhasePtr[offset + i], rawPhi2Ptr[offset + i]);
      }
      if(Result<> ioResult = phi2StoreRef.copyFromBuffer(offset, nonstd::span<const float32>(phi2Buffer.data(), count)); ioResult.invalid())
      {
        return ioResult;
      }
    }
  }

  if(m_InputValues->create_compatible_arrays)
  {
    auto& targetArray = m_DataStructure.getDataRefAs<Int32ArrayType>(cellAttributeMatrixPath.createChildPath(ebsdlib::CtfFile::Phases));
    auto& phaseStore = targetArray.getDataStoreRef();
    std::vector<int32> phaseBuffer(k_ChunkTuples);

    for(usize offset = 0; offset < totalCells; offset += k_ChunkTuples)
    {
      if(m_ShouldCancel)
      {
        return {};
      }

      const usize count = std::min(k_ChunkTuples, totalCells - offset);
      for(usize i = 0; i < count; i++)
      {
        phaseBuffer[i] = rawPhasePtr[offset + i];
      }
      if(Result<> ioResult = phaseStore.copyFromBuffer(offset, nonstd::span<const int32>(phaseBuffer.data(), count)); ioResult.invalid())
      {
        return ConvertResult(std::move(ioResult));
      }
    }
  }

  if(m_InputValues->create_compatible_arrays)
  {
    const auto* rawPhi1Ptr = reinterpret_cast<const float32*>(reader->getPointerByName(ebsdlib::Ctf::phi1));
    const auto* rawPhiPtr = reinterpret_cast<const float32*>(reader->getPointerByName(ebsdlib::Ctf::Phi));

    auto& cellEulerAngles = m_DataStructure.getDataRefAs<Float32ArrayType>(cellAttributeMatrixPath.createChildPath(ebsdlib::CtfFile::EulerAngles));
    auto& eulerStore = cellEulerAngles.getDataStoreRef();
    std::vector<float32> eulerBuffer(k_ChunkTuples * 3);

    for(usize offset = 0; offset < totalCells; offset += k_ChunkTuples)
    {
      if(m_ShouldCancel)
      {
        return {};
      }

      const usize count = std::min(k_ChunkTuples, totalCells - offset);
      for(usize i = 0; i < count; i++)
      {
        eulerBuffer[3 * i] = rawPhi1Ptr[offset + i];
        eulerBuffer[3 * i + 1] = rawPhiPtr[offset + i];
        eulerBuffer[3 * i + 2] = alignedPhi2(rawPhasePtr[offset + i], rawPhi2Ptr[offset + i]);
      }
      if(Result<> ioResult = eulerStore.copyFromBuffer(offset * 3, nonstd::span<const float32>(eulerBuffer.data(), count * 3)); ioResult.invalid())
      {
        return ConvertResult(std::move(ioResult));
      }
    }
  }
  return {};
}
