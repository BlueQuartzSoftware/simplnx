#include "ReadGrainMapper3D.hpp"

#include "OrientationAnalysis/utilities/GrainMapper3DUtilities.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5DataStore.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/DatasetIO.hpp"

#include "H5Support/H5ScopedSentinel.h"
#include "H5Support/H5Utilities.h"

#include <fmt/format.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

using namespace nx::core;
using namespace H5Support;
using namespace GrainMapper3DUtilities;
namespace GM3DConst = GrainMapper3DUtilities::Constants;

namespace
{
constexpr usize k_MaximumTransferValues = 65536;

/**
 * @brief Reads an HDF5 dataset in bounded C-order hyperslabs.
 * @tparam T Dataset value type.
 * @tparam Callback Batch consumer type.
 * @param datasetReader Source dataset.
 * @param datasetName Dataset name for diagnostics.
 * @param shouldCancel Signals cancellation between transfers.
 * @param maximumValues Maximum values in one transfer. The limit is 65,536.
 * @param callback Consumes each batch and its flat dataset offset.
 * @return Read or callback errors. Cancellation returns success after the last completed batch.
 *
 * The callback consumes each flat batch immediately. Phase, Rodrigues, IPF,
 * quaternion, and absorption conversions can write bounded output pages without
 * materializing a complete volume dataset.
 */
template <typename T, typename Callback>
Result<> ReadDatasetInBatches(const nx::core::HDF5::DatasetIO& datasetReader, const std::string& datasetName, const std::atomic_bool& shouldCancel, usize maximumValues, Callback&& callback)
{
  const auto dimensions = datasetReader.getDimensions();
  if(dimensions.empty())
  {
    return MakeErrorResult(-89360, fmt::format("ReadGrainMapper3D: Unable to determine the dimensions of '/LabDCT/Data/{}'.", datasetName));
  }
  if(maximumValues == 0 || maximumValues > k_MaximumTransferValues)
  {
    return MakeErrorResult(-89361, fmt::format("ReadGrainMapper3D: Invalid bounded transfer size for '/LabDCT/Data/{}'.", datasetName));
  }

  // Select a batch dimension whose trailing dimensions fit in the buffer. This
  // avoids a full XY plane allocation for vector-valued volumes.
  const usize rank = dimensions.size();
  usize totalValues = 1;
  for(const usize dimensionSize : dimensions)
  {
    if(dimensionSize == 0 || totalValues > std::numeric_limits<usize>::max() / dimensionSize)
    {
      return MakeErrorResult(-89362, fmt::format("ReadGrainMapper3D: Invalid or overflowing dimensions for '/LabDCT/Data/{}'.", datasetName));
    }
    totalValues *= dimensionSize;
  }

  // Exclude the batch dimension from the trailing product. Including it would
  // count that extent twice when the full dataset fits in one transfer.
  usize trailingValues = 1;
  usize batchDimension = 0;
  for(usize dimension = rank - 1; dimension > 0; dimension--)
  {
    if(trailingValues > maximumValues / dimensions[dimension])
    {
      batchDimension = dimension;
      break;
    }
    trailingValues *= dimensions[dimension];
    batchDimension = dimension - 1;
  }

  usize outerCount = 1;
  for(usize dimension = 0; dimension < batchDimension; dimension++)
  {
    if(outerCount > std::numeric_limits<usize>::max() / dimensions[dimension])
    {
      return MakeErrorResult(-89362, fmt::format("ReadGrainMapper3D: Dimensions overflow the transfer iterator for '/LabDCT/Data/{}'.", datasetName));
    }
    outerCount *= dimensions[dimension];
  }

  const usize batchRows = std::max(static_cast<usize>(1), maximumValues / trailingValues);
  std::vector<T> buffer(maximumValues);
  std::vector<uint64> start(rank);
  std::vector<uint64> count(rank, 1);
  usize flatOffset = 0;
  for(usize outer = 0; outer < outerCount; outer++)
  {
    if(shouldCancel)
    {
      return {};
    }

    usize coordinate = outer;
    for(usize dimension = batchDimension; dimension-- > 0;)
    {
      start[dimension] = coordinate % dimensions[dimension];
      coordinate /= dimensions[dimension];
    }
    for(usize dimension = batchDimension + 1; dimension < rank; dimension++)
    {
      start[dimension] = 0;
      count[dimension] = dimensions[dimension];
    }

    for(usize batchStart = 0; batchStart < dimensions[batchDimension]; batchStart += batchRows)
    {
      if(shouldCancel)
      {
        return {};
      }
      const usize rowCount = std::min(batchRows, dimensions[batchDimension] - batchStart);
      const usize valueCount = rowCount * trailingValues;
      start[batchDimension] = batchStart;
      count[batchDimension] = rowCount;
      auto readResult = datasetReader.readIntoSpan<T>(nonstd::span<T>(buffer.data(), valueCount), start, count);
      if(readResult.invalid())
      {
        return MakeErrorResult(-89363, fmt::format("ReadGrainMapper3D: Error reading '/LabDCT/Data/{}': {}", datasetName, readResult.errors()[0].message));
      }
      auto callbackResult = callback(nonstd::span<const T>(buffer.data(), valueCount), flatOffset);
      if(callbackResult.invalid())
      {
        return callbackResult;
      }
      flatOffset += valueCount;
    }
  }

  if(flatOffset != totalValues || flatOffset != datasetReader.getNumElements())
  {
    return MakeErrorResult(-89364, fmt::format("ReadGrainMapper3D: Bounded transfer size does not match '/LabDCT/Data/{}'.", datasetName));
  }
  return {};
}

} // namespace

namespace ebsdlib::CrystalStructure
{
inline constexpr uint32_t UnknownCrystalStructure = 999; // Sentinel for an invalid ensemble entry.
}

ReadGrainMapper3D::ReadGrainMapper3D(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadGrainMapper3DInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

const std::atomic_bool& ReadGrainMapper3D::getCancel()
{
  return m_ShouldCancel;
}

Result<> ReadGrainMapper3D::copyPhaseInformation(GrainMapperReader& reader, hid_t fileId) const
{
  if(!m_InputValues->ReadDctData)
  {
    return {};
  }
  herr_t error = reader.readPhaseInfo(fileId);
  if(error < 0)
  {
    return MakeErrorResult(-39801, fmt::format("Error reading phase info"));
  }

  auto phases = reader.getPhaseVector();
  DataPath cellEnsembleAMPath = m_InputValues->DctImageGeometryPath.createChildPath(m_InputValues->DctCellEnsembleAttributeMatrixName);

  // Use the standard EBSD ensemble names that downstream orientation filters expect.
  auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(cellEnsembleAMPath.createChildPath(GM3DConstants::k_CrystalStructures));
  auto& materialNames = m_DataStructure.getDataRefAs<StringArray>(cellEnsembleAMPath.createChildPath(GM3DConstants::k_MaterialName));
  auto& latticeConstantsArray = m_DataStructure.getDataRefAs<Float32Array>(cellEnsembleAMPath.createChildPath(GM3DConstants::k_LatticeConstants));
  Float32Array::store_type* latticeConstants = latticeConstantsArray.getDataStore();
  auto& universalHermannMauguin = m_DataStructure.getDataRefAs<StringArray>(cellEnsembleAMPath.createChildPath(GM3DConstants::k_UniversalHermannMauguin));

  crystalStructures[0] = ebsdlib::CrystalStructure::UnknownCrystalStructure;
  materialNames[0] = "Invalid Phase";
  latticeConstants->setComponent(0, 0, 0.0f);
  latticeConstants->setComponent(0, 1, 0.0f);
  latticeConstants->setComponent(0, 2, 0.0f);
  latticeConstants->setComponent(0, 3, 0.0f);
  latticeConstants->setComponent(0, 4, 0.0f);
  latticeConstants->setComponent(0, 5, 0.0f);
  universalHermannMauguin[0] = "Invalid Phase";
  int32 index = 1;
  for(const auto& phase : phases)
  {
    const int32 phaseId = index++;
    crystalStructures[phaseId] = GrainMapper3DUtilities::GetLaueIndexFromSpaceGroup(phase.SpaceGroup);
    materialNames[phaseId] = phase.Name;
    std::vector<double> lc = phase.UnitCell;

    latticeConstants->setComponent(phaseId, 0, static_cast<float32>(lc[0]));
    latticeConstants->setComponent(phaseId, 1, static_cast<float32>(lc[1]));
    latticeConstants->setComponent(phaseId, 2, static_cast<float32>(lc[2]));
    latticeConstants->setComponent(phaseId, 3, static_cast<float32>(lc[3]));
    latticeConstants->setComponent(phaseId, 4, static_cast<float32>(lc[4]));
    latticeConstants->setComponent(phaseId, 5, static_cast<float32>(lc[5]));

    universalHermannMauguin[phaseId] = phase.UniversalHermannMauguin;
  }

  return {};
}

Result<> ReadGrainMapper3D::copyDctData(GrainMapperReader& reader, hid_t fileId) const
{
  if(!m_InputValues->ReadDctData)
  {
    return {};
  }

  hid_t labDctGid = H5Gopen(fileId, GM3DConst::k_LabDCTGroupName.c_str(), H5P_DEFAULT);
  if(labDctGid < 0)
  {
    return MakeErrorResult(-89300, fmt::format("ReadGrainMapper3D: Error opening '{}' group.", GM3DConst::k_LabDCTGroupName));
  }
  auto groupSentinel = H5Support::H5ScopedGroupSentinel(labDctGid, true);

  hid_t dataGid = H5Gopen(labDctGid, GM3DConst::k_DataGroupName.c_str(), H5P_DEFAULT);
  if(dataGid < 0)
  {
    return MakeErrorResult(-89301, fmt::format("ReadGrainMapper3D: Error opening '/LabDCT/{}' group.", GM3DConst::k_DataGroupName));
  }
  groupSentinel.addGroupId(dataGid);

  reader.findAvailableDctDatasets(labDctGid);
  auto dctDataSets = reader.getDctDatasetNames();

  std::set<std::string> floatDataSets;
  std::set<std::string> in32DataSets;
  std::set<std::string> uint8DataSets;

  auto nameToDataTypeMap = reader.getNameToDataTypeMap();
  for(const auto& entry : nameToDataTypeMap)
  {
    if(entry.second == DataType::float32)
    {
      floatDataSets.insert(entry.first);
    }
    else if(entry.second == DataType::uint8)
    {
      uint8DataSets.insert(entry.first);
    }
    else if(entry.second == DataType::int32)
    {
      in32DataSets.insert(entry.first);
    }
  }

  Result<> result;

  // Convert phase IDs from the file's uint8 representation to the requested int32 array.
  if(m_InputValues->ConvertPhaseData && (std::count(dctDataSets.begin(), dctDataSets.end(), GM3DConst::k_PhaseIdName) > 0))
  {
    uint8DataSets.erase(GM3DConst::k_PhaseIdName);
    DataPath dataArrayPath = m_InputValues->DctImageGeometryPath.createChildPath(m_InputValues->DctCellAttributeMatrixName).createChildPath(GM3DConst::k_PhaseIdName);
    auto& phaseI32 = m_DataStructure.getDataRefAs<Int32Array>(dataArrayPath).getDataStoreRef();
    nx::core::HDF5::DatasetIO datasetReader(dataGid, GM3DConst::k_PhaseIdName);
    auto conversionResult = ReadDatasetInBatches<uint8>(datasetReader, GM3DConst::k_PhaseIdName, m_ShouldCancel, k_MaximumTransferValues,
                                                        [&phaseI32, &dataArrayPath](nonstd::span<const uint8> source, usize offset) -> Result<> {
                                                          if(offset > phaseI32.getSize() || source.size() > phaseI32.getSize() - offset)
                                                          {
                                                            return MakeErrorResult(-89365, fmt::format("ReadGrainMapper3D: Destination range exceeds '{}'.", dataArrayPath.toString()));
                                                          }
                                                          std::vector<int32> converted(source.size());
                                                          std::transform(source.begin(), source.end(), converted.begin(), [](uint8 value) { return static_cast<int32>(value); });
                                                          return phaseI32.copyFromBuffer(offset, nonstd::span<const int32>(converted.data(), converted.size()));
                                                        });
    if(conversionResult.invalid())
    {
      return conversionResult;
    }
  }

  // Convert each Rodrigues triple to a unit axis and magnitude.
  if(m_InputValues->ConvertOrientationData && (std::count(dctDataSets.begin(), dctDataSets.end(), GM3DConst::k_RodriguesName) > 0))
  {
    floatDataSets.erase(GM3DConst::k_RodriguesName);
    DataPath dataArrayPath = m_InputValues->DctImageGeometryPath.createChildPath(m_InputValues->DctCellAttributeMatrixName).createChildPath(GM3DConst::k_RodriguesName);
    auto& rodData = m_DataStructure.getDataRefAs<Float32Array>(dataArrayPath).getDataStoreRef();
    nx::core::HDF5::DatasetIO datasetReader(dataGid, GM3DConst::k_RodriguesName);
    constexpr usize k_RodriguesSourceBatchValues = (k_MaximumTransferValues / 4) * 3;
    auto conversionResult = ReadDatasetInBatches<float32>(
        datasetReader, GM3DConst::k_RodriguesName, m_ShouldCancel, k_RodriguesSourceBatchValues, [&rodData, &dataArrayPath](nonstd::span<const float32> source, usize sourceOffset) -> Result<> {
          if(source.size() % 3 != 0)
          {
            return MakeErrorResult(-89366, fmt::format("ReadGrainMapper3D: '/LabDCT/Data/{}' does not contain 3-component Rodrigues values.", dataArrayPath.getTargetName()));
          }
          const usize destinationOffset = (sourceOffset / 3) * 4;
          const usize tupleCount = source.size() / 3;
          if(destinationOffset > rodData.getSize() || tupleCount > (rodData.getSize() - destinationOffset) / 4)
          {
            return MakeErrorResult(-89365, fmt::format("ReadGrainMapper3D: Destination range exceeds '{}'.", dataArrayPath.toString()));
          }
          std::vector<float32> converted(tupleCount * 4);
          for(usize tuple = 0; tuple < tupleCount; tuple++)
          {
            const float32 r0 = source[tuple * 3] * -1.0f;
            const float32 r1 = source[tuple * 3 + 1] * -1.0f;
            const float32 r2 = source[tuple * 3 + 2] * -1.0f;
            // The file format must supply a nonzero Rodrigues vector. A zero
            // vector produces a nonfinite axis in the current implementation.
            const float length = sqrtf(r0 * r0 + r1 * r1 + r2 * r2);
            converted[tuple * 4] = r0 / length;
            converted[tuple * 4 + 1] = r1 / length;
            converted[tuple * 4 + 2] = r2 / length;
            converted[tuple * 4 + 3] = length;
          }
          return rodData.copyFromBuffer(destinationOffset, nonstd::span<const float32>(converted.data(), converted.size()));
        });
    if(conversionResult.invalid())
    {
      return conversionResult;
    }
  }

  if(m_InputValues->ConvertIPFColors && nameToDataTypeMap[GM3DConst::k_IPF001Name] == DataType::float32)
  {
    std::vector<std::string> ipfDataSets = {GM3DConst::k_IPF001Name, GM3DConst::k_IPF010Name, GM3DConst::k_IPF100Name};
    for(const auto& dataSetName : ipfDataSets)
    {
      floatDataSets.erase(dataSetName);
      DataPath dataArrayPath = m_InputValues->DctImageGeometryPath.createChildPath(m_InputValues->DctCellAttributeMatrixName).createChildPath(dataSetName);
      auto& ipfUint8 = m_DataStructure.getDataRefAs<UInt8Array>(dataArrayPath).getDataStoreRef();
      nx::core::HDF5::DatasetIO datasetReader(dataGid, dataSetName);
      constexpr usize k_IpfSourceBatchValues = (k_MaximumTransferValues / 3) * 3;
      auto conversionResult =
          ReadDatasetInBatches<float32>(datasetReader, dataSetName, m_ShouldCancel, k_IpfSourceBatchValues, [&ipfUint8, &dataArrayPath](nonstd::span<const float32> source, usize offset) -> Result<> {
            if(source.size() % 3 != 0)
            {
              return MakeErrorResult(-89367, fmt::format("ReadGrainMapper3D: '/LabDCT/Data/{}' does not contain 3-component IPF colors.", dataArrayPath.getTargetName()));
            }
            if(offset > ipfUint8.getSize() || source.size() > ipfUint8.getSize() - offset)
            {
              return MakeErrorResult(-89365, fmt::format("ReadGrainMapper3D: Destination range exceeds '{}'.", dataArrayPath.toString()));
            }
            // The file format must keep IPF components in [0, 1]. The current
            // conversion does not clamp values before the uint8 cast.
            std::vector<uint8> converted(source.size());
            std::transform(source.begin(), source.end(), converted.begin(), [](float32 value) { return static_cast<uint8>(value * 255.0f); });
            return ipfUint8.copyFromBuffer(offset, nonstd::span<const uint8>(converted.data(), converted.size()));
          });
      if(conversionResult.invalid())
      {
        return conversionResult;
      }
    }
  }

  if(m_InputValues->ConvertOrientationData && (std::count(dctDataSets.begin(), dctDataSets.end(), GM3DConst::k_QuaternionName) > 0))
  {
    floatDataSets.erase(GM3DConst::k_QuaternionName);
    DataPath dataArrayPath = m_InputValues->DctImageGeometryPath.createChildPath(m_InputValues->DctCellAttributeMatrixName).createChildPath(GM3DConst::k_QuaternionName);
    auto& quatData = m_DataStructure.getDataRefAs<Float32Array>(dataArrayPath).getDataStoreRef();
    nx::core::HDF5::DatasetIO datasetReader(dataGid, GM3DConst::k_QuaternionName);
    auto conversionResult = ReadDatasetInBatches<float32>(datasetReader, GM3DConst::k_QuaternionName, m_ShouldCancel, k_MaximumTransferValues,
                                                          [&quatData, &dataArrayPath](nonstd::span<const float32> source, usize offset) -> Result<> {
                                                            if(source.size() % 4 != 0 || offset > quatData.getSize() || source.size() > quatData.getSize() - offset)
                                                            {
                                                              return MakeErrorResult(-89365, fmt::format("ReadGrainMapper3D: Destination range exceeds '{}'.", dataArrayPath.toString()));
                                                            }
                                                            std::vector<float32> converted(source.size());
                                                            for(usize tuple = 0; tuple < source.size() / 4; tuple++)
                                                            {
                                                              const float32 w = source[tuple * 4];
                                                              converted[tuple * 4] = source[tuple * 4 + 1] * -1.0f;
                                                              converted[tuple * 4 + 1] = source[tuple * 4 + 2] * -1.0f;
                                                              converted[tuple * 4 + 2] = source[tuple * 4 + 3] * -1.0f;
                                                              converted[tuple * 4 + 3] = w;
                                                            }
                                                            return quatData.copyFromBuffer(offset, nonstd::span<const float32>(converted.data(), converted.size()));
                                                          });
    if(conversionResult.invalid())
    {
      return conversionResult;
    }
  }

  // Stream datasets that do not require representation conversion.
  for(const auto& dataSetName : dctDataSets)
  {
    DataPath dataArrayPath = m_InputValues->DctImageGeometryPath.createChildPath(m_InputValues->DctCellAttributeMatrixName).createChildPath(dataSetName);

    nx::core::HDF5::DatasetIO datasetReader(dataGid, dataSetName);

    if(std::count(floatDataSets.begin(), floatDataSets.end(), dataSetName) > 0)
    {
      result = nx::core::HDF5::Support::FillDataArray<float32>(m_DataStructure, dataArrayPath, datasetReader, std::nullopt, std::nullopt, &m_ShouldCancel);
    }
    else if(std::count(in32DataSets.begin(), in32DataSets.end(), dataSetName) > 0)
    {
      result = nx::core::HDF5::Support::FillDataArray<int32>(m_DataStructure, dataArrayPath, datasetReader, std::nullopt, std::nullopt, &m_ShouldCancel);
    }
    else if(std::count(uint8DataSets.begin(), uint8DataSets.end(), dataSetName) > 0)
    {
      result = nx::core::HDF5::Support::FillDataArray<uint8>(m_DataStructure, dataArrayPath, datasetReader, std::nullopt, std::nullopt, &m_ShouldCancel);
    }
    if(result.invalid())
    {
      return result;
    }
  }

  return {};
}

Result<> ReadGrainMapper3D::copyAbsorptionData(GrainMapperReader& reader, hid_t fileId) const
{
  if(!m_InputValues->ReadAbsorptionData)
  {
    return {};
  }
  hid_t gid = H5Gopen(fileId, GM3DConst::k_AbsorptionCTName.c_str(), H5P_DEFAULT);
  if(gid < 0)
  {
    return MakeErrorResult(-89350, fmt::format("ReadGrainMapper3D: Error opening '{}' group.", GM3DConst::k_AbsorptionCTName));
  }
  auto groupSentinel = H5Support::H5ScopedGroupSentinel(gid, true);

  DataPath dataArrayPath =
      m_InputValues->AbsorptionImageGeometryPath.createChildPath(m_InputValues->AbsorptionCellAttributeMatrixName).createChildPath(GrainMapper3DUtilities::Constants::k_DataGroupName);

  nx::core::HDF5::DatasetIO datasetReader(gid, GM3DConst::k_DataGroupName);

  return nx::core::HDF5::Support::FillDataArray<uint16>(m_DataStructure, dataArrayPath, datasetReader, std::nullopt, std::nullopt, &m_ShouldCancel);
}

Result<> ReadGrainMapper3D::operator()()
{
  GrainMapperReader reader(m_InputValues->InputFile.string(), m_InputValues->ReadDctData, m_InputValues->ReadAbsorptionData);

  hid_t fileId = H5Support::H5Utilities::openFile(m_InputValues->InputFile.string(), true);
  if(fileId < 0)
  {
    return MakeErrorResult(-89350, fmt::format("Grain Mapper 3D File '{}' could not be opened.", m_InputValues->InputFile.string()));
  }
  auto sentinel = H5Support::H5ScopedFileSentinel(fileId, false);

  Result<> result = copyPhaseInformation(reader, fileId);
  if(result.invalid())
  {
    return result;
  }

  result = copyDctData(reader, fileId);
  if(result.invalid())
  {
    return result;
  }

  result = copyAbsorptionData(reader, fileId);
  if(result.invalid())
  {
    return result;
  }

  return {};
}
