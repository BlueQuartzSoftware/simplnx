#include "ImportH5Data.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/StringArray.hpp"

#include "EbsdLib/IO/TSL/AngConstants.h"
#include "EbsdLib/IO/TSL/AngPhase.h"

using namespace complex;

// -----------------------------------------------------------------------------
template <class T>
ImportH5Data<T>::ImportH5Data(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const ImportH5DataInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
, m_InputValues(inputValues)
{
  m_Reader = T::New();
  m_Reader->setFileName(inputValues->SelectedScanNames.inputFilePath.string());
}

// -----------------------------------------------------------------------------
template <class T>
ImportH5Data<T>::~ImportH5Data() noexcept = default;

// -----------------------------------------------------------------------------
template <class T>
const std::atomic_bool& ImportH5Data<T>::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
template <class T>
Result<> ImportH5Data<T>::execute()
{
  auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  imageGeom.setUnits(IGeometry::LengthUnit::Micrometer);

  int index = 0;
  for(const auto& currentScanName : m_InputValues->SelectedScanNames.scanNames)
  {
    Result<> readResults = readData(currentScanName);
    if(readResults.invalid())
    {
      return readResults;
    }

    Result<> copyDataResults = copyRawEbsdData(index);
    if(copyDataResults.invalid())
    {
      return copyDataResults;
    }

    ++index;
  }
  return {};
}

// -----------------------------------------------------------------------------
template <class T>
Result<> ImportH5Data<T>::readData(const std::string& scanName)
{
  m_Reader->setReadPatternData(m_InputValues->ReadPatternData);
  // If the user has already set a Scan Name to read then we are good to go.
  m_Reader->setHDF5Path(scanName);

  if(const int32 err = m_Reader->readFile(); err < 0)
  {
    return MakeErrorResult(-8970, "EbsdReader could not read the .h5 file.");
  }

  const auto phases = m_Reader->getPhaseVector();
  if(phases.empty())
  {
    return MakeErrorResult(-8971, "EbsdReader could not read the phase vector from the .h5 file.");
  }

  // These arrays are purposely created using the AngFile constant names for BOTH the Oim and the Esprit readers!
  auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CellEnsembleAttributeMatrixPath.createChildPath(EbsdLib::AngFile::CrystalStructures));
  auto& materialNames = m_DataStructure.getDataRefAs<StringArray>(m_InputValues->CellEnsembleAttributeMatrixPath.createChildPath(EbsdLib::AngFile::MaterialName));
  auto& latticeConstantsArray = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CellEnsembleAttributeMatrixPath.createChildPath(EbsdLib::AngFile::LatticeConstants));
  Float32Array::store_type* latticeConstants = latticeConstantsArray.getDataStore();

  crystalStructures[0] = EbsdLib::CrystalStructure::UnknownCrystalStructure;
  materialNames[0] = "Invalid Phase";
  latticeConstants->setComponent(0, 0, 0.0f);
  latticeConstants->setComponent(0, 1, 0.0f);
  latticeConstants->setComponent(0, 2, 0.0f);
  latticeConstants->setComponent(0, 3, 0.0f);
  latticeConstants->setComponent(0, 4, 0.0f);
  latticeConstants->setComponent(0, 5, 0.0f);
  for(const auto& phase : phases)
  {
    const int32 phaseId = phase->getPhaseIndex();
    crystalStructures[phaseId] = phase->determineLaueGroup();
    materialNames[phaseId] = phase->getMaterialName();
    std::vector<float32> lc = phase->getLatticeConstants();

    latticeConstants->setComponent(phaseId, 0, lc[0]);
    latticeConstants->setComponent(phaseId, 1, lc[1]);
    latticeConstants->setComponent(phaseId, 2, lc[2]);
    latticeConstants->setComponent(phaseId, 3, lc[3]);
    latticeConstants->setComponent(phaseId, 4, lc[4]);
    latticeConstants->setComponent(phaseId, 5, lc[5]);
  }

  return {};
}
