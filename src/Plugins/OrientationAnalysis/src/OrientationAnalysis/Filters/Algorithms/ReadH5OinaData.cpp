#include "ReadH5OinaData.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"

using namespace nx::core;

namespace
{

template <typename T>
void copyRawData(const ReadH5DataInputValues* m_InputValues, size_t totalPoints, DataStructure& m_DataStructure, H5OINAReader& m_Reader, const std::string& name, usize offset)
{
  using ArrayType = DataArray<T>;
  auto& dataRef = m_DataStructure.getDataRefAs<ArrayType>(m_InputValues->CellAttributeMatrixPath.createChildPath(name));
  auto* dataStorePtr = dataRef.getDataStore();

  usize numElements = m_Reader.getNumberOfElements();
  const nonstd::span<T> rawDataPtr(reinterpret_cast<T*>(m_Reader.getPointerByName(name)), numElements);
  std::copy(rawDataPtr.begin(), rawDataPtr.end(), dataStorePtr->begin() + offset);
}

template <typename T>
void convertHexEulerAngle(const ReadH5DataInputValues* m_InputValues, size_t totalPoints, DataStructure& m_DataStructure)
{
  using ArrayType = DataArray<T>;

  if(m_InputValues->EdaxHexagonalAlignment)
  {
    auto& crystalStructuresRef = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CellEnsembleAttributeMatrixPath.createChildPath(EbsdLib::AngFile::CrystalStructures));
    auto& crystalStructuresDSRef = crystalStructuresRef.getDataStoreRef();

    auto& cellPhasesRef = m_DataStructure.getDataRefAs<ArrayType>(m_InputValues->CellAttributeMatrixPath.createChildPath(EbsdLib::H5OINA::Phase));
    auto& cellPhasesDSRef = cellPhasesRef.getDataStoreRef();

    auto& eulerRef = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(EbsdLib::H5OINA::Euler));
    auto& eulerDataStoreRef = eulerRef.getDataStoreRef();

    for(size_t i = 0; i < totalPoints; i++)
    {
      if(crystalStructuresDSRef[cellPhasesDSRef[i]] == EbsdLib::CrystalStructure::Hexagonal_High)
      {
        eulerDataStoreRef[3 * i + 2] = eulerDataStoreRef[3 * i + 2] + 30.0F; // See the documentation for this correction factor
      }
    }
  }
}

} // namespace

// -----------------------------------------------------------------------------
ReadH5OinaData::ReadH5OinaData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadH5DataInputValues* inputValues)
: ReadH5Data<H5OINAReader>(dataStructure, mesgHandler, shouldCancel, inputValues)
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
  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  const usize totalPoints = imageGeom.getNumXCells() * imageGeom.getNumYCells();
  const usize offset = index * totalPoints;

  copyRawData<uint8>(m_InputValues, totalPoints, m_DataStructure, *m_Reader, EbsdLib::H5OINA::BandContrast, offset);
  copyRawData<uint8>(m_InputValues, totalPoints, m_DataStructure, *m_Reader, EbsdLib::H5OINA::BandSlope, offset);
  copyRawData<uint8>(m_InputValues, totalPoints, m_DataStructure, *m_Reader, EbsdLib::H5OINA::Bands, offset);
  copyRawData<uint8>(m_InputValues, totalPoints, m_DataStructure, *m_Reader, EbsdLib::H5OINA::Error, offset);
  copyRawData<float>(m_InputValues, totalPoints * 3, m_DataStructure, *m_Reader, EbsdLib::H5OINA::Euler, offset);
  copyRawData<float>(m_InputValues, totalPoints, m_DataStructure, *m_Reader, EbsdLib::H5OINA::MeanAngularDeviation, offset);
  if(m_InputValues->ConvertPhaseToInt32)
  {
    const nonstd::span<uint8> rawDataPtr(reinterpret_cast<uint8*>(m_Reader->getPointerByName(EbsdLib::H5OINA::Phase)), totalPoints);
    using ArrayType = DataArray<int32>;
    auto& dataRef = m_DataStructure.getDataRefAs<ArrayType>(m_InputValues->CellAttributeMatrixPath.createChildPath(EbsdLib::H5OINA::Phase));
    auto* dataStorePtr = dataRef.getDataStore();
    for(size_t i = 0; i < totalPoints; i++)
    {
      dataStorePtr->setValue(i + offset, static_cast<int32>(rawDataPtr[i]));
    }
  }
  else
  {
    copyRawData<uint8>(m_InputValues, totalPoints, m_DataStructure, *m_Reader, EbsdLib::H5OINA::Phase, offset);
  }
  copyRawData<float>(m_InputValues, totalPoints, m_DataStructure, *m_Reader, EbsdLib::H5OINA::X, offset);
  copyRawData<float>(m_InputValues, totalPoints, m_DataStructure, *m_Reader, EbsdLib::H5OINA::Y, offset);

  if(m_InputValues->EdaxHexagonalAlignment)
  {
    if(m_InputValues->ConvertPhaseToInt32)
    {
      convertHexEulerAngle<int32>(m_InputValues, totalPoints, m_DataStructure);
    }
    else
    {
      convertHexEulerAngle<uint8>(m_InputValues, totalPoints, m_DataStructure);
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
      auto& patternData = m_DataStructure.getDataRefAs<UInt8Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(EbsdLib::H5OINA::UnprocessedPatterns));
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
