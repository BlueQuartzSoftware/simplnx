#include "ImportH5EspritData.hpp"

#include "complex/Common/Constants.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
ImportH5EspritData::ImportH5EspritData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const ImportH5DataInputValues* inputValues,
                                       ImportH5EspritDataInputValues* espritInputValues)
: ImportH5Data<H5EspritReader>(dataStructure, mesgHandler, shouldCancel, inputValues)
, m_EspritInputValues(espritInputValues)
{
}

// -----------------------------------------------------------------------------
ImportH5EspritData::~ImportH5EspritData() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ImportH5EspritData::operator()()
{
  return execute();
}

// -----------------------------------------------------------------------------
Result<> ImportH5EspritData::copyRawEbsdData(int index)
{
  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  const usize totalPoints = imageGeom.getNumXCells() * imageGeom.getNumYCells();
  const usize offset = index * totalPoints;
  const float32 degToRad = m_EspritInputValues->DegreesToRadians ? Constants::k_PiOver180F : 1.0f;

  // Condense the Euler Angles from 3 separate arrays into a single 1x3 array
  {
    const auto* f1 = reinterpret_cast<float32*>(m_Reader->getPointerByName(EbsdLib::H5Esprit::phi1));
    const auto* f2 = reinterpret_cast<float32*>(m_Reader->getPointerByName(EbsdLib::H5Esprit::PHI));
    const auto* f3 = reinterpret_cast<float32*>(m_Reader->getPointerByName(EbsdLib::H5Esprit::phi2));
    auto& eulerAngles = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(EbsdLib::Esprit::EulerAngles));
    for(size_t i = 0; i < totalPoints; i++)
    {
      eulerAngles[offset + 3 * i] = f1[i] * degToRad;
      eulerAngles[offset + 3 * i + 1] = f2[i] * degToRad;
      eulerAngles[offset + 3 * i + 2] = f3[i] * degToRad;
    }
  }
  {
    const auto* f1 = reinterpret_cast<float32*>(m_Reader->getPointerByName(EbsdLib::H5Esprit::MAD));
    auto& mad = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(EbsdLib::H5Esprit::MAD));
    for(size_t i = 0; i < totalPoints; i++)
    {
      mad[offset + i] = f1[i];
    }
  }

  {
    const auto* f1 = reinterpret_cast<int32*>(m_Reader->getPointerByName(EbsdLib::H5Esprit::NIndexedBands));
    auto& nIndexBands = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(EbsdLib::H5Esprit::NIndexedBands));
    for(size_t i = 0; i < totalPoints; i++)
    {
      nIndexBands[offset + i] = f1[i];
    }
  }

  {
    const auto* f1 = reinterpret_cast<int32*>(m_Reader->getPointerByName(EbsdLib::H5Esprit::Phase));
    auto& phase = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(EbsdLib::H5Esprit::Phase));
    for(size_t i = 0; i < totalPoints; i++)
    {
      phase[offset + i] = f1[i];
    }
  }

  {
    const auto* f1 = reinterpret_cast<int32*>(m_Reader->getPointerByName(EbsdLib::H5Esprit::RadonBandCount));
    auto& radonBandCount = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(EbsdLib::H5Esprit::RadonBandCount));
    for(size_t i = 0; i < totalPoints; i++)
    {
      radonBandCount[offset + i] = f1[i];
    }
  }

  {
    const auto* f1 = reinterpret_cast<float32*>(m_Reader->getPointerByName(EbsdLib::H5Esprit::RadonQuality));
    auto& radonQuality = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(EbsdLib::H5Esprit::RadonQuality));
    for(size_t i = 0; i < totalPoints; i++)
    {
      radonQuality[offset + i] = f1[i];
    }
  }

  {
    const auto* f1 = reinterpret_cast<int32*>(m_Reader->getPointerByName(EbsdLib::H5Esprit::XBEAM));
    auto& xBeam = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(EbsdLib::H5Esprit::XBEAM));
    for(size_t i = 0; i < totalPoints; i++)
    {
      xBeam[offset + i] = f1[i];
    }
  }

  {
    const auto* f1 = reinterpret_cast<int32*>(m_Reader->getPointerByName(EbsdLib::H5Esprit::YBEAM));
    auto& yBeam = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(EbsdLib::H5Esprit::YBEAM));
    for(size_t i = 0; i < totalPoints; i++)
    {
      yBeam[offset + i] = f1[i];
    }
  }

  if(m_InputValues->ReadPatternData)
  {
    const uint8* patternDataPtr = m_Reader->getPatternData();
    std::array<int32, 2> pDims = {{0, 0}};
    m_Reader->getPatternDims(pDims);
    if(pDims[0] != 0 && pDims[1] != 0)
    {
      std::vector<usize> pDimsV(2);
      pDimsV[0] = pDims[0];
      pDimsV[1] = pDims[1];
      auto& patternData = m_DataStructure.getDataRefAs<UInt8Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(EbsdLib::H5Esprit::RawPatterns));
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
