//
// Created by Michael Jackson on 8/19/24.
//

#include "GrainMapper3DUtilities.hpp"

#include "H5Support/H5Lite.h"
#include "H5Support/H5ScopedSentinel.h"
#include "H5Support/H5Utilities.h"

#include <fmt/format.h>

using namespace nx::core;
using namespace H5Support;

namespace GM3DConst = GrainMapper3DUtilities::Constants;

namespace GrainMapper3DUtilities
{

const std::map<std::string, DataType> k_NameToDataTypeMap = {
    {GM3DConst::k_CompletenessName, DataType::float32}, {GM3DConst::k_GrainIdName, DataType::int32},      {GM3DConst::k_MaskName, DataType::uint8},
    {GM3DConst::k_PhaseIdName, DataType::uint8},        {GM3DConst::k_RodriguesName, DataType::float32},  {GM3DConst::k_EulerZXZName, DataType::float32},
    {GM3DConst::k_EulerZYZName, DataType::float32},     {GM3DConst::k_QuaternionName, DataType::float32}, {GM3DConst::k_IPF001Name, DataType::uint8},
    {GM3DConst::k_IPF010Name, DataType::uint8},         {GM3DConst::k_IPF100Name, DataType::uint8}};

const std::map<std::string, size_t> k_NameToCompDimMap = {{GM3DConst::k_CompletenessName, 1}, {GM3DConst::k_GrainIdName, 1},  {GM3DConst::k_MaskName, 1},     {GM3DConst::k_PhaseIdName, 1},
                                                          {GM3DConst::k_RodriguesName, 3},    {GM3DConst::k_EulerZXZName, 3}, {GM3DConst::k_EulerZYZName, 3}, {GM3DConst::k_QuaternionName, 4},
                                                          {GM3DConst::k_IPF001Name, 3},       {GM3DConst::k_IPF010Name, 3},   {GM3DConst::k_IPF100Name, 3}};

int32_t GetLaueIndexFromSpaceGroup(int32_t spaceGroupId)
{
  // clang-format off
  std::array<size_t, 32> sgpg =   {1, 2, 3, 6, 10, 16, 25, 47, 75, 81, 83, 89, 99, 111, 123, 143, 147, 149, 156, 162, 168, 174, 175, 177, 183, 187, 191, 195, 200, 207, 215, 221};
  std::array<size_t, 32> pgLaue = {1, 1, 2, 2, 2,  22, 22, 22, 4,  4,  4,  42, 42, 42,  42,  3,   3,   32,  32,  32,  6,   6,   6,   62,  62,  62,  62,  23,  23,  43,  43,  43};
  // clang-format on
  size_t pgIndex = sgpg.size() - 1;
  for(size_t i = 0; i < sgpg.size(); i++)
  {
    if(sgpg[i] > spaceGroupId)
    {
      pgIndex = i - 1;
      break;
    }
  }

  size_t value = pgLaue.at(pgIndex);
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
: m_FileName(filePath)
, m_ReadDctData(readDctData)
, m_ReadAbsorptionData(readAbsorptionData)
{
}

GrainMapperReader::~GrainMapperReader() = default;

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
  return GrainMapper3DUtilities::k_NameToDataTypeMap;
}

std::map<std::string, size_t> GrainMapperReader::getNameToCompDimMap() const
{
  return GrainMapper3DUtilities::k_NameToCompDimMap;
}

std::vector<std::string> GrainMapperReader::getDctDatasetNames() const
{
  return m_AvailableDCTDatasets;
}

std::vector<GrainMapperReader::GrainMapperPhase> GrainMapperReader::getPhaseInformation() const
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
  hid_t labDctGid = H5Gopen(fileId, Constants::k_LabDCTGroupName.c_str(), H5P_DEFAULT);
  if(labDctGid < 0)
  {
    return MakeErrorResult(-38602, "GrainMapperReader: Error opening group /LabDCT");
  }
  H5ScopedGroupSentinel sentinel(labDctGid, true);

  std::vector<double> extents;
  herr_t error = H5Lite::readVectorDataset(labDctGid, Constants::k_ExtentName, extents);
  if(error < 0)
  {
    return MakeErrorResult(-38603, "GrainMapperReader: Error reading data set /LabDCT/Extent");
  }

  error = H5Lite::readVectorDataset(labDctGid, Constants::k_SpacingName, m_LabDctSpacing);
  if(error < 0)
  {
    return MakeErrorResult(-38604, "GrainMapperReader: Error reading data set /LabDCT/Spacing");
  }

  m_LabDctDimensions =
      std::vector<size_t>{static_cast<size_t>(extents[0] / m_LabDctSpacing[0]), static_cast<size_t>(extents[1] / m_LabDctSpacing[1]), static_cast<size_t>(extents[2] / m_LabDctSpacing[2])};

  std::vector<double> center;
  error = H5Lite::readVectorDataset(labDctGid, Constants::k_CenterName, center);
  if(error < 0)
  {
    return MakeErrorResult(-38605, "GrainMapperReader: Error reading data set /LabDCT/Center");
  }

  std::vector<double> virtualShift;
  error = H5Lite::readVectorDataset(labDctGid, Constants::k_VirtualShift, virtualShift);
  if(error < 0)
  {
    return MakeErrorResult(-38608, "GrainMapperReader: Error reading data set /LabDCT/VirtualShift");
  }

  m_LabDctOrigin[0] = (center[0] - (extents[0] * 0.5)) + virtualShift[0];
  m_LabDctOrigin[1] = (center[1] - (extents[1] * 0.5)) + virtualShift[1];
  m_LabDctOrigin[2] = (center[2] - (extents[2] * 0.5)) + virtualShift[2];

  error = findAvailableDctDatasets(labDctGid);
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

  hid_t gid = H5Gopen(fileId, Constants::k_AbsorptionCTName.c_str(), H5P_DEFAULT);
  if(gid < 0)
  {
    return MakeErrorResult(-38602, "GrainMapperReader: Error opening group /AbsorptionCT");
  }
  H5ScopedGroupSentinel sentinel(gid, true);

  std::vector<double> extents;
  herr_t error = H5Lite::readVectorDataset(gid, Constants::k_ExtentName, extents);
  if(error < 0)
  {
    return MakeErrorResult(-38603, "GrainMapperReader: Error reading data set /LabDCT/Extent");
  }

  error = H5Lite::readVectorDataset(gid, Constants::k_SpacingName, m_AbsorptionCTSpacing);
  if(error < 0)
  {
    return MakeErrorResult(-38604, "GrainMapperReader: Error reading data set /LabDCT/Spacing");
  }

  m_AbsorptionCTDimensions = std::vector<size_t>{static_cast<size_t>(extents[0] / m_AbsorptionCTSpacing[0]), static_cast<size_t>(extents[1] / m_AbsorptionCTSpacing[1]),
                                                 static_cast<size_t>(extents[2] / m_AbsorptionCTSpacing[2])};

  std::vector<double> center;
  error = H5Lite::readVectorDataset(gid, Constants::k_CenterName, center);
  if(error < 0)
  {
    return MakeErrorResult(-38605, "GrainMapperReader: Error reading data set /AbsorptionCT/Center");
  }

  std::vector<double> virtualShift;
  error = H5Lite::readVectorDataset(gid, Constants::k_VirtualShift, virtualShift);
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

  hid_t fileId = H5Support::H5Utilities::openFile(m_FileName, true);
  if(fileId < 0)
  {
    return MakeErrorResult(-39600, fmt::format("Grain Mapper 3D File '{}' could not be opened.", m_FileName));
  }
  auto sentinel = H5Support::H5ScopedFileSentinel(fileId, false);

  result = readLabDCTHeader(fileId);
  if(result.invalid())
  {
    return result;
  }

  result = readAbsorptionHeader(fileId);
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
  hid_t dataGid = H5Gopen(labDctGid, Constants::k_DataGroupName.c_str(), H5P_DEFAULT);
  if(dataGid < 0)
  {
    return dataGid;
  }
  auto groupSentinel = H5Support::H5ScopedGroupSentinel(dataGid, true);

  for(const auto& entry : GrainMapper3DUtilities::k_NameToDataTypeMap)
  {
    if(H5Lite::datasetExists(dataGid, entry.first))
    {
      m_AvailableDCTDatasets.push_back(entry.first);
    }
  }
  return 0;
}

herr_t GrainMapperReader::readPhaseInfo(hid_t parentId)
{
  // Get the Phase Information
  hid_t phaseInfoGid = H5Gopen(parentId, Constants::k_PhaseInfoName.c_str(), H5P_DEFAULT);
  if(phaseInfoGid < 0)
  {
    return phaseInfoGid;
  }
  auto groupSentinel = H5Support::H5ScopedGroupSentinel(phaseInfoGid, true);
  std::list<std::string> phaseNames;
  herr_t error = H5Utilities::getGroupObjects(phaseInfoGid, H5Utilities::CustomHDFDataTypes::Group, phaseNames);
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

    hid_t phaseGid = H5Gopen(phaseInfoGid, phaseName.c_str(), H5P_DEFAULT);
    auto phaseDGidSentinel = H5Support::H5ScopedGroupSentinel(phaseGid, true);

    GrainMapperPhase phase;
    error = H5Lite::readStringDataset(phaseGid, Constants::k_Name, phase.Name);
    if(error < 0)
    {
      return error;
    }

    error = H5Lite::readStringDataset(phaseGid, Constants::k_Name, phase.UniversalHermannMauguin);
    if(error < 0)
    {
      return error;
    }

    error = H5Lite::readScalarDataset(phaseGid, Constants::k_SpaceGroupName, phase.SpaceGroup);
    if(error < 0)
    {
      return error;
    }

    error = H5Lite::readVectorDataset(phaseGid, Constants::k_UnitCellName, phase.UnitCell);
    if(error < 0)
    {
      return error;
    }

    m_PhaseInfos.push_back(phase);
  }
  return 0;
}

} // namespace GrainMapper3DUtilities
