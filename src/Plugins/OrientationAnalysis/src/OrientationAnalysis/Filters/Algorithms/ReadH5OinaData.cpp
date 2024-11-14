#include "ReadH5OinaData.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"

using namespace nx::core;

namespace
{

template <typename T>
void copyRawData(const DataPath& cellAMPath, size_t totalPoints, DataStructure& m_DataStructure, H5OINAReader& m_Reader, const std::string& name, usize offset = 0)
{
  using ArrayType = DataArray<T>;
  auto& dataRef = m_DataStructure.getDataRefAs<ArrayType>(cellAMPath.createChildPath(name));
  auto* dataStorePtr = dataRef.getDataStore();

  const nonstd::span<T> rawDataPtr(reinterpret_cast<T*>(m_Reader.getPointerByName(name)), totalPoints);
  std::copy(rawDataPtr.begin(), rawDataPtr.end(), dataStorePtr->begin() + offset);
}

template <typename T>
void convertHexEulerAngle(const DataPath& cellAMPath, const DataPath& cellEnsembleAMPath, size_t totalPoints, DataStructure& m_DataStructure)
{
  // If EDAX Hexagonal Alignment is checked outside call
  using ArrayType = DataArray<T>;

  auto& crystalStructuresRef = m_DataStructure.getDataRefAs<UInt32Array>(cellEnsembleAMPath.createChildPath(EbsdLib::AngFile::CrystalStructures));
  auto& crystalStructuresDSRef = crystalStructuresRef.getDataStoreRef();

  auto& cellPhasesRef = m_DataStructure.getDataRefAs<ArrayType>(cellAMPath.createChildPath(EbsdLib::H5OINA::Phase));
  auto& cellPhasesDSRef = cellPhasesRef.getDataStoreRef();

  auto& eulerRef = m_DataStructure.getDataRefAs<Float32Array>(cellAMPath.createChildPath(EbsdLib::H5OINA::Euler));
  auto& eulerDataStoreRef = eulerRef.getDataStoreRef();

  for(size_t i = 0; i < totalPoints; i++)
  {
    if(crystalStructuresDSRef[cellPhasesDSRef[i]] == EbsdLib::CrystalStructure::Hexagonal_High)
    {
      eulerDataStoreRef[3 * i + 2] = eulerDataStoreRef[3 * i + 2] + 30.0F; // See the documentation for this correction factor
    }
  }
}

} // namespace

// -----------------------------------------------------------------------------
ReadH5OinaData::ReadH5OinaData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadH5DataInputValues* inputValues)
: IEbsdOemReader<H5OINAReader>(dataStructure, mesgHandler, shouldCancel, inputValues)
{
}

// -----------------------------------------------------------------------------
ReadH5OinaData::~ReadH5OinaData() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ReadH5OinaData::operator()()
{
  return execute();
}

// -----------------------------------------------------------------------------
Result<> ReadH5OinaData::copyRawEbsdData(int index)
{
  const DataPath imagePath(m_InputValues->ImageGeometryPath);
  const DataPath cellAMPath = imagePath.createChildPath(m_InputValues->CellAttributeMatrixName);

  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(imagePath);
  const usize totalPoints = imageGeom.getNumXCells() * imageGeom.getNumYCells();
  const usize offset = index * totalPoints;

  copyRawData<uint8>(cellAMPath, totalPoints, m_DataStructure, *m_Reader, EbsdLib::H5OINA::BandContrast, offset);
  copyRawData<uint8>(cellAMPath, totalPoints, m_DataStructure, *m_Reader, EbsdLib::H5OINA::BandSlope, offset);
  copyRawData<uint8>(cellAMPath, totalPoints, m_DataStructure, *m_Reader, EbsdLib::H5OINA::Bands, offset);
  copyRawData<uint8>(cellAMPath, totalPoints, m_DataStructure, *m_Reader, EbsdLib::H5OINA::Error, offset);
  copyRawData<float>(cellAMPath, totalPoints * 3, m_DataStructure, *m_Reader, EbsdLib::H5OINA::Euler, offset);
  copyRawData<float>(cellAMPath, totalPoints, m_DataStructure, *m_Reader, EbsdLib::H5OINA::MeanAngularDeviation, offset);
  if(m_InputValues->ConvertPhaseToInt32)
  {
    const nonstd::span<uint8> rawDataPtr(reinterpret_cast<uint8*>(m_Reader->getPointerByName(EbsdLib::H5OINA::Phase)), totalPoints);
    using ArrayType = DataArray<int32>;
    auto& dataRef = m_DataStructure.getDataRefAs<ArrayType>(cellAMPath.createChildPath(EbsdLib::H5OINA::Phase));
    auto* dataStorePtr = dataRef.getDataStore();
    for(size_t i = 0; i < totalPoints; i++)
    {
      dataStorePtr->setValue(i + offset, static_cast<int32>(rawDataPtr[i]));
    }
  }
  else
  {
    copyRawData<uint8>(cellAMPath, totalPoints, m_DataStructure, *m_Reader, EbsdLib::H5OINA::Phase, offset);
  }
  copyRawData<float>(cellAMPath, totalPoints, m_DataStructure, *m_Reader, EbsdLib::H5OINA::X, offset);
  copyRawData<float>(cellAMPath, totalPoints, m_DataStructure, *m_Reader, EbsdLib::H5OINA::Y, offset);

  if(m_InputValues->EdaxHexagonalAlignment)
  {
    const DataPath& cellEnsembleAMPath = imagePath.createChildPath(m_InputValues->CellEnsembleAttributeMatrixName);
    if(m_InputValues->ConvertPhaseToInt32)
    {
      convertHexEulerAngle<int32>(cellAMPath, cellEnsembleAMPath, totalPoints, m_DataStructure);
    }
    else
    {
      convertHexEulerAngle<uint8>(cellAMPath, cellEnsembleAMPath, totalPoints, m_DataStructure);
    }
  }

  if(m_InputValues->ReadPatternData)
  {
    const uint16* patternDataPtr = m_Reader->getPatternData();
    std::array<int32, 2> pDims = {{0, 0}};
    m_Reader->getPatternDims(pDims);
    if(pDims[0] != 0 && pDims[1] != 0)
    {
      std::vector<usize> pDimsV(2);
      pDimsV[0] = pDims[0];
      pDimsV[1] = pDims[1];
      auto& patternData = m_DataStructure.getDataRefAs<UInt8Array>(cellAMPath.createChildPath(EbsdLib::H5OINA::UnprocessedPatterns));
      const usize numComponents = patternData.getNumberOfComponents();
      for(usize i = 0; i < totalPoints; i++)
      {
        for(usize j = 0; j < numComponents; ++j)
        {
          patternData[offset + numComponents * i + j] = patternDataPtr[numComponents * i + j];
        }
      }
    }
  }

  return {};
}

// -----------------------------------------------------------------------------
Result<> ReadH5OinaData::copyRawEbsdData(const std::string& scanName)
{
  const DataPath imagePath({scanName});
  const DataPath cellAMPath = imagePath.createChildPath(m_InputValues->CellAttributeMatrixName);

  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(imagePath);
  const usize totalPoints = imageGeom.getNumXCells() * imageGeom.getNumYCells();

  copyRawData<uint8>(cellAMPath, totalPoints, m_DataStructure, *m_Reader, EbsdLib::H5OINA::BandContrast);
  copyRawData<uint8>(cellAMPath, totalPoints, m_DataStructure, *m_Reader, EbsdLib::H5OINA::BandSlope);
  copyRawData<uint8>(cellAMPath, totalPoints, m_DataStructure, *m_Reader, EbsdLib::H5OINA::Bands);
  copyRawData<uint8>(cellAMPath, totalPoints, m_DataStructure, *m_Reader, EbsdLib::H5OINA::Error);
  copyRawData<float>(cellAMPath, totalPoints * 3, m_DataStructure, *m_Reader, EbsdLib::H5OINA::Euler);
  copyRawData<float>(cellAMPath, totalPoints, m_DataStructure, *m_Reader, EbsdLib::H5OINA::MeanAngularDeviation);
  if(m_InputValues->ConvertPhaseToInt32)
  {
    const nonstd::span<uint8> rawDataPtr(reinterpret_cast<uint8*>(m_Reader->getPointerByName(EbsdLib::H5OINA::Phase)), totalPoints);
    using ArrayType = DataArray<int32>;
    auto& dataRef = m_DataStructure.getDataRefAs<ArrayType>(cellAMPath.createChildPath(EbsdLib::H5OINA::Phase));
    auto* dataStorePtr = dataRef.getDataStore();
    for(size_t i = 0; i < totalPoints; i++)
    {
      dataStorePtr->setValue(i, static_cast<int32>(rawDataPtr[i]));
    }
  }
  else
  {
    copyRawData<uint8>(cellAMPath, totalPoints, m_DataStructure, *m_Reader, EbsdLib::H5OINA::Phase);
  }
  copyRawData<float>(cellAMPath, totalPoints, m_DataStructure, *m_Reader, EbsdLib::H5OINA::X);
  copyRawData<float>(cellAMPath, totalPoints, m_DataStructure, *m_Reader, EbsdLib::H5OINA::Y);

  if(m_InputValues->EdaxHexagonalAlignment)
  {
    const DataPath& cellEnsembleAMPath = imagePath.createChildPath(m_InputValues->CellEnsembleAttributeMatrixName);
    if(m_InputValues->ConvertPhaseToInt32)
    {
      convertHexEulerAngle<int32>(cellAMPath, cellEnsembleAMPath, totalPoints, m_DataStructure);
    }
    else
    {
      convertHexEulerAngle<uint8>(cellAMPath, cellEnsembleAMPath, totalPoints, m_DataStructure);
    }
  }

  if(m_InputValues->ReadPatternData)
  {
    const uint16* patternDataPtr = m_Reader->getPatternData();
    std::array<int32, 2> pDims = {{0, 0}};
    m_Reader->getPatternDims(pDims);
    if(pDims[0] != 0 && pDims[1] != 0)
    {
      std::vector<usize> pDimsV(2);
      pDimsV[0] = pDims[0];
      pDimsV[1] = pDims[1];
      auto& patternData = m_DataStructure.getDataRefAs<UInt8Array>(cellAMPath.createChildPath(EbsdLib::H5OINA::UnprocessedPatterns));
      const usize numComponents = patternData.getNumberOfComponents();
      for(usize i = 0; i < totalPoints; i++)
      {
        for(usize j = 0; j < numComponents; ++j)
        {
          patternData[numComponents * i + j] = patternDataPtr[numComponents * i + j];
        }
      }
    }
  }

  return {};
}
