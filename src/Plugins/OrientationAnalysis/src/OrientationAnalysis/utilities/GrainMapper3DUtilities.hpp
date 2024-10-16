#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"

#include <hdf5.h>

#include <cstdint>
#include <string>
#include <vector>

namespace GrainMapper3DUtilities
{

namespace Constants
{
const std::string k_LabDCTGroupName("LabDCT");
const std::string k_AbsorptionCTName("AbsorptionCT");
const std::string k_ProjectInfoName("ProjectInfo");
const std::string k_VersionName("Version");

const std::string k_ExtentName("Extent");
const std::string k_SpacingName("Spacing");
const std::string k_CenterName("Center");
const std::string k_VirtualShift("VirtualShift");

const std::string k_DataGroupName("Data");
const std::string k_CompletenessName("Completeness");
const std::string k_GrainIdName("GrainId");
const std::string k_MaskName("Mask");
const std::string k_PhaseIdName("PhaseId");
const std::string k_RodriguesName("Rodrigues");

const std::string k_EulerZXZName("EulerZXZ");
const std::string k_EulerZYZName("EulerZYZ");
const std::string k_QuaternionName("Quaternion");
const std::string k_IPF001Name("IPF001");
const std::string k_IPF010Name("IPF010");
const std::string k_IPF100Name("IPF100");

// ****************************************************************************
// Phase Constants
const std::string k_PhaseInfoName("PhaseInfo");
const std::string k_Name("Name");
const std::string k_SpaceGroupName("SpaceGroup");
const std::string k_UnitCellName("UnitCell");
const std::string k_UniversalHermannMauguinName("UniversalHermannMauguin");

} // namespace Constants

int32_t GetLaueIndexFromSpaceGroup(int32_t spaceGroupId);

/**
 * @brief
 */
class ORIENTATIONANALYSIS_EXPORT GrainMapperReader
{
public:
  explicit GrainMapperReader(const std::string& filePath, bool readDctData, bool readAbsorptionData);
  ~GrainMapperReader();

  typedef struct
  {
    std::string Name;
    int32_t SpaceGroup;
    std::vector<double> UnitCell; //  ABC, Alpha, Beta, Gamma
    std::string UniversalHermannMauguin;
  } GrainMapperPhase;

  nx::core::Result<> readHeaderOnly();

  std::vector<size_t> getLabDCTDimensions() const;
  std::vector<float> getLabDCTSpacing() const;
  std::vector<float> getLabDCTOrigin() const;

  std::vector<size_t> getAbsorptionCTDimensions() const;
  std::vector<float> getAbsorptionCTSpacing() const;
  std::vector<float> getAbsorptionCTOrigin() const;

  nx::core::Result<> readLabDCTHeader(hid_t fileId);
  nx::core::Result<> readAbsorptionHeader(hid_t fileId);

  std::vector<std::string> getDctDatasetNames() const;
  std::map<std::string, nx::core::DataType> getNameToDataTypeMap() const;
  std::map<std::string, size_t> getNameToCompDimMap() const;
  std::vector<GrainMapperPhase> getPhaseInformation() const;
  herr_t readPhaseInfo(hid_t parentId);
  herr_t findAvailableDctDatasets(hid_t parentId);

private:
  bool m_ReadDctData = false;
  bool m_ReadAbsorptionData = false;

  std::string m_ErrorMessage = {};
  std::string m_FileName = {};
  std::string m_HDF5Path = {};
  std::string m_OINAVersion = {};

  std::vector<size_t> m_LabDctDimensions;
  std::vector<double> m_LabDctSpacing = {1.0, 1.0, 1.0};
  std::vector<double> m_LabDctOrigin = {0.0, 0.0, 0.0};

  std::vector<size_t> m_AbsorptionCTDimensions;
  std::vector<double> m_AbsorptionCTSpacing = {1.0, 1.0, 1.0};
  std::vector<double> m_AbsorptionCTOrigin = {0.0, 0.0, 0.0};

  std::vector<std::string> m_AvailableDCTDatasets;
  std::vector<GrainMapperPhase> m_PhaseInfos;
};

}; // namespace GrainMapper3DUtilities
