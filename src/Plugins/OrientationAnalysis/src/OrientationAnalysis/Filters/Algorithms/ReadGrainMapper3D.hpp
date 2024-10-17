#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"
#include "OrientationAnalysis/utilities/GrainMapper3DUtilities.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <hdf5.h>

using namespace GrainMapper3DUtilities;

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT ReadGrainMapper3DInputValues
{
  std::filesystem::path InputFile;
  bool ReadDctData;
  DataPath DctImageGeometryPath;
  std::string DctCellAttributeMatrixName;
  std::string DctCellEnsembleAttributeMatrixName;
  bool ConvertPhaseData;
  bool ConvertOrientationData;

  bool ReadAbsorptionData;
  DataPath AbsorptionImageGeometryPath;
  std::string AbsorptionCellAttributeMatrixName;
};

namespace GM3DConstants
{
const std::string k_CrystalStructures("CrystalStructures");
const std::string k_LatticeConstants("LatticeConstants");
const std::string k_MaterialName("MaterialName");
} // namespace GM3DConstants
/**
 * @class ReadGrainMapper3D
 * @brief This filter determines the average C-axis location of each Feature.
 */

class ORIENTATIONANALYSIS_EXPORT ReadGrainMapper3D
{
public:
  ReadGrainMapper3D(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadGrainMapper3DInputValues* inputValues);
  ~ReadGrainMapper3D() noexcept = default;

  ReadGrainMapper3D(const ReadGrainMapper3D&) = delete;
  ReadGrainMapper3D(ReadGrainMapper3D&&) noexcept = delete;
  ReadGrainMapper3D& operator=(const ReadGrainMapper3D&) = delete;
  ReadGrainMapper3D& operator=(ReadGrainMapper3D&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

protected:
  Result<> copyPhaseInformation(GrainMapperReader& reader, hid_t fileId) const;
  Result<> copyDctData(GrainMapper3DUtilities::GrainMapperReader& reader, hid_t fileId) const;
  Result<> copyAbsorptionData(GrainMapperReader& reader, hid_t fileId) const;

private:
  DataStructure& m_DataStructure;
  const ReadGrainMapper3DInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
