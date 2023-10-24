#include "ReadH5EspritData.hpp"

#include "complex/Common/Constants.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"

using namespace complex;

// -----------------------------------------------------------------------------
ReadH5EspritData::ReadH5EspritData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const ReadH5DataInputValues* inputValues,
                                       ReadH5EspritDataInputValues* espritInputValues)
: ReadH5Data<H5EspritReader>(dataStructure, mesgHandler, shouldCancel, inputValues)
, m_EspritInputValues(espritInputValues)
{
}

// -----------------------------------------------------------------------------
ReadH5EspritData::~ReadH5EspritData() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ReadH5EspritData::operator()()
{
  return execute();
}

// -----------------------------------------------------------------------------
Result<> ReadH5EspritData::copyRawEbsdData(int index)
{
  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  const usize totalPoints = imageGeom.getNumXCells() * imageGeom.getNumYCells();
  const usize offset = index * totalPoints;

  {
    const float32 degToRad = m_EspritInputValues->DegreesToRadians ? Constants::k_PiOver180F : 1.0f;
    const auto* phi1 = reinterpret_cast<float32*>(m_Reader->getPointerByName(EbsdLib::H5Esprit::phi1));
    const auto* phi = reinterpret_cast<float32*>(m_Reader->getPointerByName(EbsdLib::H5Esprit::PHI));
    const auto* phi2 = reinterpret_cast<float32*>(m_Reader->getPointerByName(EbsdLib::H5Esprit::phi2));
    auto& eulerAngles = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(EbsdLib::Esprit::EulerAngles));

    const auto* m1 = reinterpret_cast<float32*>(m_Reader->getPointerByName(EbsdLib::H5Esprit::MAD));
    auto& mad = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(EbsdLib::H5Esprit::MAD));

    const auto* nIndBands = reinterpret_cast<int32*>(m_Reader->getPointerByName(EbsdLib::H5Esprit::NIndexedBands));
    auto& nIndexBands = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(EbsdLib::H5Esprit::NIndexedBands));

    const auto* p1 = reinterpret_cast<int32*>(m_Reader->getPointerByName(EbsdLib::H5Esprit::Phase));
    auto& phase = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(EbsdLib::H5Esprit::Phase));

    const auto* radBandCnt = reinterpret_cast<int32*>(m_Reader->getPointerByName(EbsdLib::H5Esprit::RadonBandCount));
    auto& radonBandCount = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(EbsdLib::H5Esprit::RadonBandCount));

    const auto* radQual = reinterpret_cast<float32*>(m_Reader->getPointerByName(EbsdLib::H5Esprit::RadonQuality));
    auto& radonQuality = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(EbsdLib::H5Esprit::RadonQuality));

    const auto* xBm = reinterpret_cast<int32*>(m_Reader->getPointerByName(EbsdLib::H5Esprit::XBEAM));
    auto& xBeam = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(EbsdLib::H5Esprit::XBEAM));

    const auto* yBm = reinterpret_cast<int32*>(m_Reader->getPointerByName(EbsdLib::H5Esprit::YBEAM));
    auto& yBeam = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(EbsdLib::H5Esprit::YBEAM));

    for(size_t i = 0; i < totalPoints; i++)
    {
      // Condense the Euler Angles from 3 separate arrays into a single 1x3 array
      eulerAngles[offset + 3 * i] = phi1[i] * degToRad;
      eulerAngles[offset + 3 * i + 1] = phi[i] * degToRad;
      eulerAngles[offset + 3 * i + 2] = phi2[i] * degToRad;

      mad[offset + i] = m1[i];

      nIndexBands[offset + i] = nIndBands[i];

      phase[offset + i] = p1[i];

      radonBandCount[offset + i] = radBandCnt[i];

      radonQuality[offset + i] = radQual[i];

      xBeam[offset + i] = xBm[i];

      yBeam[offset + i] = yBm[i];
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
