//
// Created by Michael Jackson on 8/19/24.
//

#include "GrainMapper3DUtilities.hpp"

#include "OrientationAnalysis/utilities/EbsdReaderUtilities.hpp"

#include "H5Support/H5Lite.h"
#include "H5Support/H5ScopedSentinel.h"
#include "H5Support/H5Utilities.h"

#include "simplnx/Common/Types.hpp"

#include <EbsdLib/LaueOps/LaueOps.h>

#include <fmt/format.h>
#include <utility>

using namespace nx::core;
using namespace H5Support;

namespace GM3DConst = GrainMapper3DUtilities::Constants;

namespace
{
/**
 * @brief Reports a UnitCell value count other than six before a phase is published.
 */
constexpr int32 k_InvalidUnitCellError = -38609;
constexpr int32 k_InvalidLabDctExtent = -38610;
constexpr int32 k_InvalidLabDctSpacing = -38611;
constexpr int32 k_InvalidLabDctCenter = -38612;
constexpr int32 k_InvalidLabDctVirtualShift = -38613;
constexpr int32 k_InvalidAbsorptionExtent = -38614;
constexpr int32 k_InvalidAbsorptionSpacing = -38615;
constexpr int32 k_InvalidAbsorptionCenter = -38616;
constexpr int32 k_InvalidAbsorptionVirtualShift = -38617;
constexpr int32 k_InvalidCompletenessInfo = -38618;
constexpr int32 k_InvalidCompletenessRank = -38619;
constexpr int32 k_InvalidSpaceGroup = -67600;
constexpr int32 k_InvalidOriginHeader = -67621;
constexpr int32 k_InvalidOriginVectorSize = -67622;

/**
 * @brief Rejects a nonfinite header vector or computed origin.
 *
 * The header readers check the value count of each file vector first (-38610 to -38617), so the -67622 size check
 * here only guards the indexing below.
 */
Result<> ValidateOriginHeader(const std::string& file, const std::string& group, const std::string& key, const std::vector<float64>& values, bool requireFloat32Range, const std::string& geometryValue)
{
  if(values.size() != 3)
  {
    return MakeErrorResult(k_InvalidOriginVectorSize, fmt::format("File '{}' header '{}/{}' has {} values. Supply three values (X, Y, Z).", file, group, key, values.size()));
  }
  for(usize axis = 0; axis < 3; ++axis)
  {
    if(!std::isfinite(values[axis]) || (requireFloat32Range && std::abs(values[axis]) > std::numeric_limits<float32>::max()))
    {
      return MakeErrorResult(k_InvalidOriginHeader,
                             fmt::format("File '{}' header '{}/{}[{}]' = {} cannot define finite Image Geometry {}. Supply finite values that produce {} within the float32 range.", file, group, key,
                                         axis, values[axis], geometryValue, geometryValue));
    }
  }
  return {};
}
} // namespace

namespace GrainMapper3DUtilities
{
Result<> ValidateSpaceGroups(const std::vector<GrainMapperReader::GrainMapperPhase>& phases, const std::string& filePath)
{
  for(const auto& phase : phases)
  {
    if(phase.SpaceGroup < 1 || phase.SpaceGroup > 230 || ebsdlib::LaueOps::GetLaueOpsFromSpaceGroupNumber(static_cast<usize>(phase.SpaceGroup)) == nullptr)
    {
      return MakeErrorResult(k_InvalidSpaceGroup, fmt::format("File '{}' phase {} has invalid Space Group {}. Use a space group from 1 to 230.", filePath, phase.PhaseIndex, phase.SpaceGroup));
    }
  }
  return {};
}

const std::map<std::string, size_t> k_NameToCompDimMap = {{GM3DConst::k_CompletenessName, 1}, {GM3DConst::k_GrainIdName, 1},  {GM3DConst::k_MaskName, 1},     {GM3DConst::k_PhaseIdName, 1},
                                                          {GM3DConst::k_RodriguesName, 3},    {GM3DConst::k_EulerZXZName, 3}, {GM3DConst::k_EulerZYZName, 3}, {GM3DConst::k_QuaternionName, 4},
                                                          {GM3DConst::k_IPF001Name, 3},       {GM3DConst::k_IPF010Name, 3},   {GM3DConst::k_IPF100Name, 3}};

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
  if(extents.size() != 3)
  {
    return MakeErrorResult(k_InvalidLabDctExtent,
                           fmt::format("GrainMapperReader: File '{}', dataset '/LabDCT/Extent' has {} values; expected 3. Check that the file was written by GrainMapper3D and is not truncated.",
                                       m_FileName, extents.size()));
  }

  std::vector<double> spacing;
  error = H5Lite::readVectorDataset(labDctGid, Constants::k_SpacingName, spacing);
  if(error < 0)
  {
    return MakeErrorResult(-38604, "GrainMapperReader: Error reading data set /LabDCT/Spacing");
  }
  if(spacing.size() != 3)
  {
    return MakeErrorResult(k_InvalidLabDctSpacing,
                           fmt::format("GrainMapperReader: File '{}', dataset '/LabDCT/Spacing' has {} values; expected 3. Check that the file was written by GrainMapper3D and is not truncated.",
                                       m_FileName, spacing.size()));
  }
  m_LabDctSpacing = std::move(spacing);

  for(usize axis = 0; axis < 3; ++axis)
  {
    if(auto result = EbsdReaderUtilities::ValidateHeaderSpacing(m_FileName, fmt::format("LabDCT/Spacing[{}]", axis), m_LabDctSpacing[axis]); result.invalid())
    {
      return result;
    }
  }

  // Get the Image Geometry dimensions by looking for the "Completeness" data set
  // which according to Xnovo documentation is a required data set. So it **should** be
  // in the file. We used to calculate the dims from the spacing and extent but due
  // to round off during calculations that can lead to dimensions that do not line
  // up with the actual dimensions of the HDF5 data set.
  std::vector<hsize_t> dims;
  H5T_class_t classType;
  size_t sizeType;
  error = H5Lite::getDatasetInfo(labDctGid, "Data/Completeness", dims, classType, sizeType);
  if(error < 0)
  {
    return MakeErrorResult(k_InvalidCompletenessInfo, fmt::format("GrainMapperReader: File '{}', dataset '/LabDCT/Data/Completeness' is missing or unreadable (HDF5 error {}); expected 3 dimensions. "
                                                                  "Check that the file was written by GrainMapper3D and is not truncated.",
                                                                  m_FileName, error));
  }
  if(dims.size() != 3)
  {
    return MakeErrorResult(
        k_InvalidCompletenessRank,
        fmt::format("GrainMapperReader: File '{}', dataset '/LabDCT/Data/Completeness' has rank {}; expected 3. Check that the file was written by GrainMapper3D and is not truncated.", m_FileName,
                    dims.size()));
  }
  // Note that we need to flip the order of dimensions from ZYX to XYZ
  m_LabDctDimensions = {static_cast<size_t>(dims[2]), static_cast<size_t>(dims[1]), static_cast<size_t>(dims[0])};

  std::vector<double> center;
  error = H5Lite::readVectorDataset(labDctGid, Constants::k_CenterName, center);
  if(error < 0)
  {
    return MakeErrorResult(-38605, "GrainMapperReader: Error reading data set /LabDCT/Center");
  }
  if(center.size() != 3)
  {
    return MakeErrorResult(k_InvalidLabDctCenter,
                           fmt::format("GrainMapperReader: File '{}', dataset '/LabDCT/Center' has {} values; expected 3. Check that the file was written by GrainMapper3D and is not truncated.",
                                       m_FileName, center.size()));
  }

  std::vector<double> virtualShift;
  error = H5Lite::readVectorDataset(labDctGid, Constants::k_VirtualShift, virtualShift);
  if(error < 0)
  {
    return MakeErrorResult(-38608, "GrainMapperReader: Error reading data set /LabDCT/VirtualShift");
  }
  if(virtualShift.size() != 3)
  {
    return MakeErrorResult(k_InvalidLabDctVirtualShift,
                           fmt::format("GrainMapperReader: File '{}', dataset '/LabDCT/VirtualShift' has {} values; expected 3. Check that the file was written by GrainMapper3D and is not truncated.",
                                       m_FileName, virtualShift.size()));
  }

  if(auto result = ValidateOriginHeader(m_FileName, "LabDCT", "Extent", extents, false, "origin"); result.invalid())
  {
    return result;
  }
  if(auto result = ValidateOriginHeader(m_FileName, "LabDCT", "Center", center, false, "origin"); result.invalid())
  {
    return result;
  }
  if(auto result = ValidateOriginHeader(m_FileName, "LabDCT", "VirtualShift", virtualShift, false, "origin"); result.invalid())
  {
    return result;
  }

  m_LabDctOrigin[0] = (center[0] - (extents[0] * 0.5)) + virtualShift[0];
  m_LabDctOrigin[1] = (center[1] - (extents[1] * 0.5)) + virtualShift[1];
  m_LabDctOrigin[2] = (center[2] - (extents[2] * 0.5)) + virtualShift[2];
  if(auto result = ValidateOriginHeader(m_FileName, "LabDCT", "computed origin", m_LabDctOrigin, true, "origin"); result.invalid())
  {
    return result;
  }

  error = findAvailableDctDatasets(labDctGid);
  if(error < 0)
  {
    return MakeErrorResult(-38606, "GrainMapperReader: Error parsing available data sets");
  }
  error = readPhaseInfo(fileId);
  if(error == k_InvalidUnitCellError)
  {
    return MakeErrorResult(k_InvalidUnitCellError, m_ErrorMessage);
  }
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
    return MakeErrorResult(-38603, "GrainMapperReader: Error reading data set /AbsorptionCT/Extent");
  }
  if(extents.size() != 3)
  {
    return MakeErrorResult(k_InvalidAbsorptionExtent,
                           fmt::format("GrainMapperReader: File '{}', dataset '/AbsorptionCT/Extent' has {} values; expected 3. Check that the file was written by GrainMapper3D and is not truncated.",
                                       m_FileName, extents.size()));
  }

  std::vector<double> spacing;
  error = H5Lite::readVectorDataset(gid, Constants::k_SpacingName, spacing);
  if(error < 0)
  {
    return MakeErrorResult(-38604, "GrainMapperReader: Error reading data set /AbsorptionCT/Spacing");
  }
  if(spacing.size() != 3)
  {
    return MakeErrorResult(
        k_InvalidAbsorptionSpacing,
        fmt::format("GrainMapperReader: File '{}', dataset '/AbsorptionCT/Spacing' has {} values; expected 3. Check that the file was written by GrainMapper3D and is not truncated.", m_FileName,
                    spacing.size()));
  }
  m_AbsorptionCTSpacing = std::move(spacing);

  m_AbsorptionCTDimensions = std::vector<size_t>{static_cast<size_t>(extents[0] / m_AbsorptionCTSpacing[0]), static_cast<size_t>(extents[1] / m_AbsorptionCTSpacing[1]),
                                                 static_cast<size_t>(extents[2] / m_AbsorptionCTSpacing[2])};

  std::vector<double> center;
  error = H5Lite::readVectorDataset(gid, Constants::k_CenterName, center);
  if(error < 0)
  {
    return MakeErrorResult(-38605, "GrainMapperReader: Error reading data set /AbsorptionCT/Center");
  }
  if(center.size() != 3)
  {
    return MakeErrorResult(k_InvalidAbsorptionCenter,
                           fmt::format("GrainMapperReader: File '{}', dataset '/AbsorptionCT/Center' has {} values; expected 3. Check that the file was written by GrainMapper3D and is not truncated.",
                                       m_FileName, center.size()));
  }

  std::vector<double> virtualShift;
  error = H5Lite::readVectorDataset(gid, Constants::k_VirtualShift, virtualShift);
  if(error < 0)
  {
    return MakeErrorResult(-38608, "GrainMapperReader: Error reading data set /AbsorptionCT/VirtualShift");
  }
  if(virtualShift.size() != 3)
  {
    return MakeErrorResult(
        k_InvalidAbsorptionVirtualShift,
        fmt::format("GrainMapperReader: File '{}', dataset '/AbsorptionCT/VirtualShift' has {} values; expected 3. Check that the file was written by GrainMapper3D and is not truncated.", m_FileName,
                    virtualShift.size()));
  }

  if(auto result = ValidateOriginHeader(m_FileName, "AbsorptionCT", "Center", center, false, "origin"); result.invalid())
  {
    return result;
  }
  if(auto result = ValidateOriginHeader(m_FileName, "AbsorptionCT", "VirtualShift", virtualShift, false, "origin"); result.invalid())
  {
    return result;
  }

  m_AbsorptionCTOrigin[0] = (center[0] - (extents[0] * 0.5)) + virtualShift[0];
  m_AbsorptionCTOrigin[1] = (center[1] - (extents[1] * 0.5)) + virtualShift[1];
  m_AbsorptionCTOrigin[2] = (center[2] - (extents[2] * 0.5)) + virtualShift[2];
  if(auto result = ValidateOriginHeader(m_FileName, "AbsorptionCT", "computed origin", m_AbsorptionCTOrigin, true, "origin"); result.invalid())
  {
    return result;
  }

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
  hid_t dataGid = H5Gopen(labDctGid, Constants::k_DataGroupName.c_str(), H5P_DEFAULT);
  if(dataGid < 0)
  {
    return dataGid;
  }
  auto groupSentinel = H5Support::H5ScopedGroupSentinel(dataGid, true);

  for(auto& entry : m_NameToDataTypeMap)
  {
    if(H5Lite::datasetExists(dataGid, entry.first))
    {
      m_AvailableDCTDatasets.push_back(entry.first);
      hid_t dataTypeIdentifier = H5Lite::getDatasetType(dataGid, entry.first);

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

const std::string& GrainMapperReader::getErrorMessage() const
{
  return m_ErrorMessage;
}

herr_t GrainMapperReader::readPhaseInfo(hid_t parentId)
{
  m_ErrorMessage.clear();
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
  // to keep them consistent. Yep, no one really thought through the parsing of this
  // or assumptions are being made about the order that HDF5 is going to give them back to you. Either
  // is bad.
  for(int i = 0; i < phaseNames.size(); i++)
  {
    std::string phaseName = fmt::format("Phase{:02}", i + 1);

    hid_t phaseGid = H5Gopen(phaseInfoGid, phaseName.c_str(), H5P_DEFAULT);
    auto phaseDGidSentinel = H5Support::H5ScopedGroupSentinel(phaseGid, true);

    GrainMapperPhase phase;
    phase.PhaseIndex = i;
    error = H5Lite::readStringDataset(phaseGid, Constants::k_Name, phase.Name);
    if(error < 0)
    {
      return error;
    }

    error = H5Lite::readStringDataset(phaseGid, Constants::k_UniversalHermannMauguinName, phase.UniversalHermannMauguin);
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

    if(phase.UnitCell.size() != 6)
    {
      m_ErrorMessage =
          fmt::format("GrainMapperReader: File '{}', phase group '/PhaseInfo/{}': UnitCell has {} values; expected 6. Check that the file was written by GrainMapper3D and is not truncated.",
                      m_FileName, phaseName, phase.UnitCell.size());
      return k_InvalidUnitCellError;
    }

    m_PhaseInfos.push_back(phase);
  }
  return 0;
}

} // namespace GrainMapper3DUtilities
