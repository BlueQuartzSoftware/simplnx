#include "ReadGrainMapper3D.hpp"

#include "OrientationAnalysis/utilities/GrainMapper3DUtilities.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"
#include "simplnx/Utilities/Parsing/HDF5/Readers/DatasetReader.hpp"

#include "H5Support/H5Lite.h"
#include "H5Support/H5ScopedSentinel.h"
#include "H5Support/H5Utilities.h"

#include <fmt/format.h>

#include <algorithm>
#include <vector>

using namespace nx::core;
using namespace H5Support;
using namespace GrainMapper3DUtilities;
namespace GM3DConst = GrainMapper3DUtilities::Constants;

namespace
{

} // namespace

namespace EbsdLib::CrystalStructure
{
inline constexpr uint32_t UnknownCrystalStructure = 999; //!< UnknownCrystalStructure
}

// -----------------------------------------------------------------------------
ReadGrainMapper3D::ReadGrainMapper3D(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadGrainMapper3DInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
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

  auto phases = reader.getPhaseInformation();
  DataPath cellEnsembleAMPath = m_InputValues->DctImageGeometryPath.createChildPath(m_InputValues->DctCellEnsembleAttributeMatrixName);

  // These arrays are purposely created using the AngFile constant names for BOTH the Oim and the Esprit readers!
  auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(cellEnsembleAMPath.createChildPath(GM3DConstants::k_CrystalStructures));
  auto& materialNames = m_DataStructure.getDataRefAs<StringArray>(cellEnsembleAMPath.createChildPath(GM3DConstants::k_MaterialName));
  auto& latticeConstantsArray = m_DataStructure.getDataRefAs<Float32Array>(cellEnsembleAMPath.createChildPath(GM3DConstants::k_LatticeConstants));
  Float32Array::store_type* latticeConstants = latticeConstantsArray.getDataStore();

  crystalStructures[0] = EbsdLib::CrystalStructure::UnknownCrystalStructure;
  materialNames[0] = "Invalid Phase";
  latticeConstants->setComponent(0, 0, 0.0f);
  latticeConstants->setComponent(0, 1, 0.0f);
  latticeConstants->setComponent(0, 2, 0.0f);
  latticeConstants->setComponent(0, 3, 0.0f);
  latticeConstants->setComponent(0, 4, 0.0f);
  latticeConstants->setComponent(0, 5, 0.0f);
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

  // Now check that each of the known data sets exist
  // Get the Image Geometry Dimensions
  hid_t dataGid = H5Gopen(labDctGid, GM3DConst::k_DataGroupName.c_str(), H5P_DEFAULT);
  if(dataGid < 0)
  {
    return MakeErrorResult(-89301, fmt::format("ReadGrainMapper3D: Error opening '/LabDCT/{}' group.", GM3DConst::k_DataGroupName));
  }
  groupSentinel.addGroupId(dataGid);

  reader.findAvailableDctDatasets(labDctGid);
  auto dctDataSets = reader.getDctDatasetNames();

  std::vector<std::string> floatDataSets = {GM3DConst::k_CompletenessName, GM3DConst::k_EulerZXZName, GM3DConst::k_EulerZYZName, GM3DConst::k_QuaternionName, GM3DConst::k_RodriguesName};
  std::vector<std::string> in32DataSets = {GM3DConst::k_GrainIdName};
  std::vector<std::string> uint8DataSets = {GM3DConst::k_MaskName, GM3DConst::k_IPF001Name, GM3DConst::k_IPF010Name, GM3DConst::k_IPF100Name, GM3DConst::k_PhaseIdName};
  Result<> result;

  // We need to special case this because we are converting from a uint8 value to an int32 value.
  if(m_InputValues->ConvertPhaseData && (std::count(dctDataSets.begin(), dctDataSets.end(), GM3DConst::k_PhaseIdName) > 0))
  {
    uint8DataSets.pop_back(); // Pop off the PhaseIdName data set since we are specifically reading it here.
    std::vector<uint8> phaseU8;
    herr_t error = H5Lite::readVectorDataset(dataGid, GM3DConst::k_PhaseIdName, phaseU8);
    if(error < 0)
    {
      return MakeErrorResult(-89302, fmt::format("ReadGrainMapper3D: Error reading '/LabDCT/Data/{}' dataset.", GM3DConst::k_PhaseIdName));
    }
    DataPath dataArrayPath = m_InputValues->DctImageGeometryPath.createChildPath(m_InputValues->DctCellAttributeMatrixName).createChildPath(GM3DConst::k_PhaseIdName);

    auto& phaseI32 = m_DataStructure.getDataAs<Int32Array>(dataArrayPath)->getDataStoreRef();
    // Copy the data from the temp buffer into the final spot.
    std::copy(phaseU8.begin(), phaseU8.end(), phaseI32.begin());
  }

  // We need to special case this because we are converting from a 3 component to a 4 component
  if(m_InputValues->ConvertOrientationData && (std::count(dctDataSets.begin(), dctDataSets.end(), GM3DConst::k_RodriguesName) > 0))
  {
    floatDataSets.pop_back(); // Pop off the Rodrigues data set since we are specifically reading it here.
    std::vector<float32> gm3dRoData;
    herr_t error = H5Lite::readVectorDataset(dataGid, GM3DConst::k_RodriguesName, gm3dRoData);
    if(error < 0)
    {
      return MakeErrorResult(-89303, fmt::format("ReadGrainMapper3D: Error reading '/LabDCT/Data/{}' dataset.", GM3DConst::k_RodriguesName));
    }
    DataPath dataArrayPath = m_InputValues->DctImageGeometryPath.createChildPath(m_InputValues->DctCellAttributeMatrixName).createChildPath(GM3DConst::k_RodriguesName);

    auto& rodData = m_DataStructure.getDataAs<Float32Array>(dataArrayPath)->getDataStoreRef();
    // Copy the data from the temp buffer into the final spot doing the conversion on the fly
    // See the section on reference frames to understand what is going on in here.
    for(size_t t = 0; t < rodData.getNumberOfTuples(); t++)
    {
      const float32 r0 = gm3dRoData[t * 3] * -1.0f;
      const float32 r1 = gm3dRoData[t * 3 + 1] * -1.0f;
      const float32 r2 = gm3dRoData[t * 3 + 2] * -1.0f;
      const float length = sqrtf(r0 * r0 + r1 * r1 + r2 * r2);

      rodData[t * 4] = r0 / length;
      rodData[t * 4 + 1] = r1 / length;
      rodData[t * 4 + 2] = r2 / length;
      rodData[t * 4 + 3] = length;
    }
  }

  // Read all remaining data sets from the HDF5 file.
  for(const auto& dataSetName : dctDataSets)
  {
    DataPath dataArrayPath = m_InputValues->DctImageGeometryPath.createChildPath(m_InputValues->DctCellAttributeMatrixName).createChildPath(dataSetName);

    nx::core::HDF5::DatasetReader datasetReader(dataGid, dataSetName);

    if(std::count(floatDataSets.begin(), floatDataSets.end(), dataSetName) > 0)
    {
      result = nx::core::HDF5::Support::FillDataArray<float32>(m_DataStructure, dataArrayPath, datasetReader);
    }
    else if(std::count(in32DataSets.begin(), in32DataSets.end(), dataSetName) > 0)
    {
      result = nx::core::HDF5::Support::FillDataArray<int32>(m_DataStructure, dataArrayPath, datasetReader);
    }
    else if(std::count(uint8DataSets.begin(), uint8DataSets.end(), dataSetName) > 0)
    {
      result = nx::core::HDF5::Support::FillDataArray<uint8>(m_DataStructure, dataArrayPath, datasetReader);
    }
    if(result.invalid())
    {
      return result;
    }
  }

  // Convert the Quaternions Reference Frame and ordering if asked by the user and if the data set exists
  if((std::count(dctDataSets.begin(), dctDataSets.end(), GM3DConst::k_QuaternionName) > 0) && m_InputValues->ConvertOrientationData)
  {
    DataPath dataArrayPath = m_InputValues->DctImageGeometryPath.createChildPath(m_InputValues->DctCellAttributeMatrixName).createChildPath(GM3DConst::k_QuaternionName);
    auto& quatData = m_DataStructure.getDataAs<Float32Array>(dataArrayPath)->getDataStoreRef();
    // Copy the data from the temp buffer into the final spot doing the conversion on the fly
    // We are reordering from wxyz (Scalar-Vector) to xyzw (Vetor-Scalar) and at the same time
    // we are taking the conjugate of the quaternion
    for(size_t t = 0; t < quatData.getNumberOfTuples(); t++)
    {
      const float32 w = quatData[t * 4];
      const float32 x = quatData[t * 4 + 1] * -1.0f;
      const float32 y = quatData[t * 4 + 2] * -1.0f;
      const float32 z = quatData[t * 4 + 3] * -1.0f;

      quatData[t * 4] = x;
      quatData[t * 4 + 1] = y;
      quatData[t * 4 + 2] = z;
      quatData[t * 4 + 3] = w;
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

  nx::core::HDF5::DatasetReader datasetReader(gid, GM3DConst::k_DataGroupName);

  return nx::core::HDF5::Support::FillDataArray<uint16>(m_DataStructure, dataArrayPath, datasetReader);
}

// -----------------------------------------------------------------------------
Result<> ReadGrainMapper3D::operator()()
{
  GrainMapperReader reader(m_InputValues->InputFile.string(), m_InputValues->ReadDctData, m_InputValues->ReadAbsorptionData);

  hid_t fileId = H5Support::H5Utilities::openFile(m_InputValues->InputFile.string(), true);
  if(fileId < 0)
  {
    return MakeErrorResult(-89350, fmt::format("Grain Mapper 3D File '{}' could not be opened.", m_InputValues->InputFile.string()));
  }
  auto sentinel = H5Support::H5ScopedFileSentinel(fileId, false);

  // ***********************************************************************
  // Read the Phase Information
  Result<> result = copyPhaseInformation(reader, fileId);
  if(result.invalid())
  {
    return result;
  }

  // ***********************************************************************
  // Read the LabDCT Information
  result = copyDctData(reader, fileId);
  if(result.invalid())
  {
    return result;
  }

  // ***********************************************************************
  // Read the LabDCT Information
  result = copyAbsorptionData(reader, fileId);
  if(result.invalid())
  {
    return result;
  }

  return {};
}
