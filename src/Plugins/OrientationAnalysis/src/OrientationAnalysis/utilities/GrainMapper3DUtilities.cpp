#include "GrainMapper3DUtilities.hpp"

#include "H5Support/H5Lite.h"
#include "H5Support/H5Utilities.h"

#include <fmt/format.h>

using namespace nx::core;
using namespace H5Support;

namespace GM3DConst = GrainMapper3DUtilities::Constants;

namespace GrainMapper3DUtilities
{
const std::map<std::string, size_t> k_NameToCompDimMap = {{GM3DConst::k_CompletenessName, 1}, {GM3DConst::k_GrainIdName, 1},  {GM3DConst::k_MaskName, 1},     {GM3DConst::k_PhaseIdName, 1},
                                                          {GM3DConst::k_RodriguesName, 3},    {GM3DConst::k_EulerZXZName, 3}, {GM3DConst::k_EulerZYZName, 3}, {GM3DConst::k_QuaternionName, 4},
                                                          {GM3DConst::k_IPF001Name, 3},       {GM3DConst::k_IPF010Name, 3},   {GM3DConst::k_IPF100Name, 3}};

int32_t GetLaueIndexFromSpaceGroup(int32_t spaceGroupId)
{
  // clang-format off
  std::array<size_t, 32> spaceGroup =   {1, 2, 3, 6, 10, 16, 25, 47, 75, 81, 83, 89, 99, 111, 123, 143, 147, 149, 156, 162, 168, 174, 175, 177, 183, 187, 191, 195, 200, 207, 215, 221};
  std::array<size_t, 32> pointGroupLaue = {1, 1, 2, 2, 2,  22, 22, 22, 4,  4,  4,  42, 42, 42,  42,  3,   3,   32,  32,  32,  6,   6,   6,   62,  62,  62,  62,  23,  23,  43,  43,  43};
  // clang-format on
  size_t pgIndex = spaceGroup.size() - 1;
  for(size_t i = 0; i < spaceGroup.size(); i++)
  {
    if(spaceGroup[i] > spaceGroupId)
    {
      pgIndex = i - 1;
      break;
    }
  }

  size_t value = pointGroupLaue.at(pgIndex);
  switch(value)
  {
  case 1: // TriclinicOps
    return 4;
  case 2: // MonoclinicOps
    return 5;
  case 22: // OrthoRhombicOps
    return 6;
  case 4: // TetragonalLowOps
    return 7;
  case 42: // TetragonalOps
    return 8;
  case 3: // TrigonalLowOps
    return 9;
  case 32: // TrigonalOps
    return 10;
  case 6: // HexagonalLowOps
    return 2;
  case 62: // HexagonalOps
    return 0;
  case 23: // CubicLowOps
    return 3;
  case 43: // CubicOps
    return 1;
  default:
    return 999;
  }
}

GrainMapperReader::GrainMapperReader(const std::string& filePath, bool readDctData, bool readAbsorptionData)
: m_ReadDctData(readDctData)
, m_ReadAbsorptionData(readAbsorptionData)
, m_FileName(filePath)
{
  m_NameToDataTypeMap = {{GM3DConst::k_CompletenessName, DataType::float32}, {GM3DConst::k_GrainIdName, DataType::int32},      {GM3DConst::k_MaskName, DataType::uint8},
                         {GM3DConst::k_PhaseIdName, DataType::uint8},        {GM3DConst::k_RodriguesName, DataType::float32},  {GM3DConst::k_EulerZXZName, DataType::float32},
                         {GM3DConst::k_EulerZYZName, DataType::float32},     {GM3DConst::k_QuaternionName, DataType::float32}, {GM3DConst::k_IPF001Name, DataType::uint8},
                         {GM3DConst::k_IPF010Name, DataType::uint8},         {GM3DConst::k_IPF100Name, DataType::uint8}};
}

GrainMapperReader::~GrainMapperReader() = default;

int GrainMapperReader::getNumColumns() const
{
  return m_LabDctDimensions[0];
}

int GrainMapperReader::getNumRows() const
{
  return m_LabDctDimensions[1];
}

float GrainMapperReader::getXStep() const
{
  return static_cast<float>(m_LabDctSpacing[0]);
}

float GrainMapperReader::getYStep() const
{
  return static_cast<float>(m_LabDctSpacing[1]);
}

std::vector<size_t> GrainMapperReader::getLabDCTDimensions() const
{
  return m_LabDctDimensions;
}

std::vector<float> GrainMapperReader::getLabDCTSpacing() const
{
  return {static_cast<float>(m_LabDctSpacing[0]), static_cast<float>(m_LabDctSpacing[1]), static_cast<float>(m_LabDctSpacing[2])};
}

std::vector<float> GrainMapperReader::getLabDCTOrigin() const
{
  return {static_cast<float>(m_LabDctOrigin[0]), static_cast<float>(m_LabDctOrigin[1]), static_cast<float>(m_LabDctOrigin[2])};
}

std::vector<size_t> GrainMapperReader::getAbsorptionCTDimensions() const
{
  return m_AbsorptionCTDimensions;
}

std::vector<float> GrainMapperReader::getAbsorptionCTSpacing() const
{
  return {static_cast<float>(m_AbsorptionCTSpacing[0]), static_cast<float>(m_AbsorptionCTSpacing[1]), static_cast<float>(m_AbsorptionCTSpacing[2])};
}

std::vector<float> GrainMapperReader::getAbsorptionCTOrigin() const
{
  return {static_cast<float>(m_AbsorptionCTOrigin[0]), static_cast<float>(m_AbsorptionCTOrigin[1]), static_cast<float>(m_AbsorptionCTOrigin[2])};
}

std::map<std::string, DataType> GrainMapperReader::getNameToDataTypeMap() const
{
  return m_NameToDataTypeMap;
}

std::map<std::string, size_t> GrainMapperReader::getNameToCompDimMap() const
{
  return GrainMapper3DUtilities::k_NameToCompDimMap;
}

std::vector<std::string> GrainMapperReader::getDctDatasetNames() const
{
  return m_AvailableDCTDatasets;
}

std::vector<GrainMapperReader::GrainMapperPhase> GrainMapperReader::getPhaseVector() const
{
  return m_PhaseInfos;
}

nx::core::Result<> GrainMapperReader::readLabDCTHeader(hid_t fileId)
{
  if(!m_ReadDctData)
  {
    return {};
  }
  // Get the LabDCT Image Geometry Dimensions
  H5GroupCloser labDctGid(H5Gopen(fileId, GM3DConst::k_LabDCTGroupName.c_str(), H5P_DEFAULT));
  if(labDctGid.invalid())
  {
    return MakeErrorResult(-38602, "GrainMapperReader: Error opening group /LabDCT");
  }

  std::vector<double> extents;
  herr_t error = H5Lite::readVectorDataset(labDctGid.id, Constants::k_ExtentName, extents);
  if(error < 0)
  {
    return MakeErrorResult(-38603, "GrainMapperReader: Error reading data set /LabDCT/Extent");
  }

  error = H5Lite::readVectorDataset(labDctGid.id, Constants::k_SpacingName, m_LabDctSpacing);
  if(error < 0)
  {
    return MakeErrorResult(-38604, "GrainMapperReader: Error reading data set /LabDCT/Spacing");
  }

  m_LabDctDimensions =
      std::vector<size_t>{static_cast<size_t>(extents[0] / m_LabDctSpacing[0]), static_cast<size_t>(extents[1] / m_LabDctSpacing[1]), static_cast<size_t>(extents[2] / m_LabDctSpacing[2])};

  std::vector<double> center;
  error = H5Lite::readVectorDataset(labDctGid.id, Constants::k_CenterName, center);
  if(error < 0)
  {
    return MakeErrorResult(-38605, "GrainMapperReader: Error reading data set /LabDCT/Center");
  }

  std::vector<double> virtualShift;
  error = H5Lite::readVectorDataset(labDctGid.id, Constants::k_VirtualShift, virtualShift);
  if(error < 0)
  {
    return MakeErrorResult(-38608, "GrainMapperReader: Error reading data set /LabDCT/VirtualShift");
  }

  m_LabDctOrigin[0] = (center[0] - (extents[0] * 0.5)) + virtualShift[0];
  m_LabDctOrigin[1] = (center[1] - (extents[1] * 0.5)) + virtualShift[1];
  m_LabDctOrigin[2] = (center[2] - (extents[2] * 0.5)) + virtualShift[2];

  error = findAvailableDctDatasets(labDctGid.id);
  if(error < 0)
  {
    return MakeErrorResult(-38606, "GrainMapperReader: Error parsing available data sets");
  }
  error = readPhaseInfo(fileId);
  if(error < 0)
  {
    return MakeErrorResult(-38607, fmt::format("GrainMapperReader: Error reading /PhaseInfo"));
  }
  return {};
}

nx::core::Result<> GrainMapperReader::readAbsorptionHeader(hid_t fileId)
{
  if(!m_ReadAbsorptionData)
  {
    return {};
  }

  H5GroupCloser gid(H5Gopen(fileId, GM3DConst::k_AbsorptionCTName.c_str(), H5P_DEFAULT));
  if(gid.invalid())
  {
    return MakeErrorResult(-38602, "GrainMapperReader: Error opening group /AbsorptionCT");
  }

  std::vector<double> extents;
  herr_t error = H5Lite::readVectorDataset(gid.id, Constants::k_ExtentName, extents);
  if(error < 0)
  {
    return MakeErrorResult(-38603, "GrainMapperReader: Error reading data set /LabDCT/Extent");
  }

  error = H5Lite::readVectorDataset(gid.id, Constants::k_SpacingName, m_AbsorptionCTSpacing);
  if(error < 0)
  {
    return MakeErrorResult(-38604, "GrainMapperReader: Error reading data set /LabDCT/Spacing");
  }

  m_AbsorptionCTDimensions = std::vector<size_t>{static_cast<size_t>(extents[0] / m_AbsorptionCTSpacing[0]), static_cast<size_t>(extents[1] / m_AbsorptionCTSpacing[1]),
                                                 static_cast<size_t>(extents[2] / m_AbsorptionCTSpacing[2])};

  std::vector<double> center;
  error = H5Lite::readVectorDataset(gid.id, Constants::k_CenterName, center);
  if(error < 0)
  {
    return MakeErrorResult(-38605, "GrainMapperReader: Error reading data set /AbsorptionCT/Center");
  }

  std::vector<double> virtualShift;
  error = H5Lite::readVectorDataset(gid.id, Constants::k_VirtualShift, virtualShift);
  if(error < 0)
  {
    return MakeErrorResult(-38608, "GrainMapperReader: Error reading data set /AbsorptionCT/VirtualShift");
  }

  m_AbsorptionCTOrigin[0] = (center[0] - (extents[0] * 0.5)) + virtualShift[0];
  m_AbsorptionCTOrigin[1] = (center[1] - (extents[1] * 0.5)) + virtualShift[1];
  m_AbsorptionCTOrigin[2] = (center[2] - (extents[2] * 0.5)) + virtualShift[2];

  return {};
}

Result<> GrainMapperReader::readHeaderOnly()
{
  Result<> result;

  H5FileCloser fileId(H5Support::H5Utilities::openFile(m_FileName, true));
  if(fileId.invalid())
  {
    return MakeErrorResult(-39600, fmt::format("Grain Mapper 3D File '{}' could not be opened.", m_FileName));
  }
  result = readLabDCTHeader(fileId.id);
  if(result.invalid())
  {
    return result;
  }

  result = readAbsorptionHeader(fileId.id);
  if(result.invalid())
  {
    return result;
  }

  return {};
}

herr_t GrainMapperReader::findAvailableDctDatasets(hid_t labDctGid)
{
  // Now check that each of the known data sets exist
  // Get the Image Geometry Dimensions
  H5GroupCloser dataGid(H5Gopen(labDctGid, Constants::k_DataGroupName.c_str(), H5P_DEFAULT));
  if(dataGid.invalid())
  {
    return dataGid.id;
  }

  for(auto& entry : m_NameToDataTypeMap)
  {
    if(H5Lite::datasetExists(dataGid.id, entry.first))
    {
      m_AvailableDCTDatasets.push_back(entry.first);
      hid_t dataTypeIdentifier = H5Lite::getDatasetType(dataGid.id, entry.first);

      // These are slightly out of the normal order for optimization reasons. We know
      // as of this implementation that the more prevalent data types in the GrainMapper
      // file are float, int32 and uint8. The others are just here for completeness.
      if(H5Tequal(dataTypeIdentifier, H5T_NATIVE_FLOAT) > 0)
      {
        entry.second = DataType::float32;
      }
      else if(H5Tequal(dataTypeIdentifier, H5T_NATIVE_UINT8) > 0 || dataTypeIdentifier == H5T_STRING)
      {
        entry.second = DataType::uint8;
      }
      else if(H5Tequal(dataTypeIdentifier, H5T_NATIVE_INT32) > 0)
      {
        entry.second = DataType::int32;
      }
      else if(H5Tequal(dataTypeIdentifier, H5T_NATIVE_INT8) > 0)
      {
        entry.second = DataType::int8;
      }
      else if(H5Tequal(dataTypeIdentifier, H5T_NATIVE_INT16) > 0)
      {
        entry.second = DataType::int16;
      }
      else if(H5Tequal(dataTypeIdentifier, H5T_NATIVE_UINT16) > 0)
      {
        entry.second = DataType::uint16;
      }
      else if(H5Tequal(dataTypeIdentifier, H5T_NATIVE_UINT32) > 0)
      {
        entry.second = DataType::uint32;
      }
      else if(H5Tequal(dataTypeIdentifier, H5T_NATIVE_INT64) > 0)
      {
        entry.second = DataType::int64;
      }
      else if(H5Tequal(dataTypeIdentifier, H5T_NATIVE_UINT64) > 0)
      {
        entry.second = DataType::uint64;
      }
      else if(H5Tequal(dataTypeIdentifier, H5T_NATIVE_DOUBLE) > 0)
      {
        entry.second = DataType::float64;
      }
    }
  }
  return 0;
}

herr_t GrainMapperReader::readPhaseInfo(hid_t parentId)
{
  // Get the Phase Information
  H5GroupCloser phaseInfoGid(H5Gopen(parentId, Constants::k_PhaseInfoName.c_str(), H5P_DEFAULT));
  if(phaseInfoGid.invalid())
  {
    return phaseInfoGid.id;
  }
  std::list<std::string> phaseNames;
  herr_t error = H5Utilities::getGroupObjects(phaseInfoGid.id, H5Utilities::CustomHDFDataTypes::Group, phaseNames);
  if(error < 0)
  {
    return error;
  }
  m_PhaseInfos.clear();

  // Now we know how many phases we have, we need to programmatically generate those phase names
  // in order to keep them consistent. Yep, someone didn't really think through the parsing of this
  // or assumptions are being made about the order that HDF5 is going to give them back to you. Either
  // is bad.
  for(int i = 0; i < phaseNames.size(); i++)
  {
    std::string phaseName = fmt::format("Phase{:02}", i + 1);

    H5GroupCloser phaseGid(H5Gopen(phaseInfoGid.id, phaseName.c_str(), H5P_DEFAULT));

    GrainMapperPhase phase;
    error = H5Lite::readStringDataset(phaseGid.id, Constants::k_Name, phase.Name);
    if(error < 0)
    {
      return error;
    }

    error = H5Lite::readStringDataset(phaseGid.id, Constants::k_Name, phase.UniversalHermannMauguin);
    if(error < 0)
    {
      return error;
    }

    error = H5Lite::readScalarDataset(phaseGid.id, Constants::k_SpaceGroupName, phase.SpaceGroup);
    if(error < 0)
    {
      return error;
    }

    error = H5Lite::readVectorDataset(phaseGid.id, Constants::k_UnitCellName, phase.UnitCell);
    if(error < 0)
    {
      return error;
    }

    m_PhaseInfos.push_back(phase);
  }
  return 0;
}

} // namespace GrainMapper3DUtilities
