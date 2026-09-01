#include "ReadH5EspritData.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"

using namespace nx::core;

ReadH5EspritData::ReadH5EspritData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const ReadH5DataInputValues* inputValues,
                                   ReadH5EspritDataInputValues* espritInputValues)
: IEbsdOemReader<ebsdlib::H5EspritReader>(dataStructure, mesgHandler, shouldCancel, inputValues)
, m_EspritInputValues(espritInputValues)
{
}

ReadH5EspritData::~ReadH5EspritData() noexcept = default;

Result<> ReadH5EspritData::operator()()
{
  return execute();
}

Result<> ReadH5EspritData::copyRawEbsdData(int index)
{
  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  const usize totalPoints = imageGeom.getNumXCells() * imageGeom.getNumYCells();
  const usize offset = index * totalPoints;

  {
    const float32 degToRad = m_EspritInputValues->DegreesToRadians ? nx::core::Constants::k_PiOver180F : 1.0f;
    const auto* phi1 = reinterpret_cast<float32*>(m_Reader->getPointerByName(ebsdlib::H5Esprit::phi1));
    const auto* phi = reinterpret_cast<float32*>(m_Reader->getPointerByName(ebsdlib::H5Esprit::PHI));
    const auto* phi2 = reinterpret_cast<float32*>(m_Reader->getPointerByName(ebsdlib::H5Esprit::phi2));
    auto& eulerAngles = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(ebsdlib::Esprit::EulerAngles));

    const auto* m1 = reinterpret_cast<float32*>(m_Reader->getPointerByName(ebsdlib::H5Esprit::MAD));
    auto& mad = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(ebsdlib::H5Esprit::MAD));

    const auto* nIndBands = reinterpret_cast<int32*>(m_Reader->getPointerByName(ebsdlib::H5Esprit::NIndexedBands));
    auto& nIndexBands = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(ebsdlib::H5Esprit::NIndexedBands));

    const auto* p1 = reinterpret_cast<int32*>(m_Reader->getPointerByName(ebsdlib::H5Esprit::Phase));
    auto& phase = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(ebsdlib::H5Esprit::Phase));

    const auto* radBandCnt = reinterpret_cast<int32*>(m_Reader->getPointerByName(ebsdlib::H5Esprit::RadonBandCount));
    auto& radonBandCount = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(ebsdlib::H5Esprit::RadonBandCount));

    const auto* radQual = reinterpret_cast<float32*>(m_Reader->getPointerByName(ebsdlib::H5Esprit::RadonQuality));
    auto& radonQuality = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(ebsdlib::H5Esprit::RadonQuality));

    const auto* xBm = reinterpret_cast<int32*>(m_Reader->getPointerByName(ebsdlib::H5Esprit::XBEAM));
    auto& xBeam = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(ebsdlib::H5Esprit::XBEAM));

    const auto* yBm = reinterpret_cast<int32*>(m_Reader->getPointerByName(ebsdlib::H5Esprit::YBEAM));
    auto& yBeam = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(ebsdlib::H5Esprit::YBEAM));

    // Interleave Euler channels and apply the unit conversion in bounded pages.
    {
      constexpr usize k_ChunkTuples = 65536;
      std::vector<float32> eulerChunk(k_ChunkTuples * 3);
      auto& eulerStore = eulerAngles.getDataStoreRef();
      for(usize chunkStart = 0; chunkStart < totalPoints; chunkStart += k_ChunkTuples)
      {
        const usize chunkCount = std::min(k_ChunkTuples, totalPoints - chunkStart);
        for(usize i = 0; i < chunkCount; i++)
        {
          eulerChunk[i * 3] = phi1[chunkStart + i] * degToRad;
          eulerChunk[i * 3 + 1] = phi[chunkStart + i] * degToRad;
          eulerChunk[i * 3 + 2] = phi2[chunkStart + i] * degToRad;
        }
        eulerStore.copyFromBuffer((offset + chunkStart) * 3, nonstd::span<const float32>(eulerChunk.data(), chunkCount * 3));
      }
    }

    // Each scalar channel uses one scan-sized transfer. These Result values are
    // not inspected by the current API.
    mad.getDataStoreRef().copyFromBuffer(offset, nonstd::span<const float32>(m1, totalPoints));
    nIndexBands.getDataStoreRef().copyFromBuffer(offset, nonstd::span<const int32>(nIndBands, totalPoints));
    phase.getDataStoreRef().copyFromBuffer(offset, nonstd::span<const int32>(p1, totalPoints));
    radonBandCount.getDataStoreRef().copyFromBuffer(offset, nonstd::span<const int32>(radBandCnt, totalPoints));
    radonQuality.getDataStoreRef().copyFromBuffer(offset, nonstd::span<const float32>(radQual, totalPoints));
    xBeam.getDataStoreRef().copyFromBuffer(offset, nonstd::span<const int32>(xBm, totalPoints));
    yBeam.getDataStoreRef().copyFromBuffer(offset, nonstd::span<const int32>(yBm, totalPoints));
  }

  if(m_InputValues->ReadPatternData)
  {
    const uint8* patternDataPtr = m_Reader->getPatternData();
    if(patternDataPtr == nullptr)
    {
      return MakeErrorResult(-34980, "Pattern data was requested but no pattern data was found in the data file");
    }
    std::array<int32, 2> pDims = {{0, 0}};
    m_Reader->getPatternDims(pDims);
    if(pDims[0] != 0 && pDims[1] != 0)
    {
      std::vector<usize> pDimsV(2);
      pDimsV[0] = pDims[0];
      pDimsV[1] = pDims[1];
      auto& patternData = m_DataStructure.getDataRefAs<UInt8Array>(m_InputValues->CellAttributeMatrixPath.createChildPath(ebsdlib::H5Esprit::RawPatterns));
      const usize numComponents = patternData.getNumberOfComponents();
      patternData.getDataStoreRef().copyFromBuffer(offset * numComponents, nonstd::span<const uint8>(patternDataPtr, totalPoints * numComponents));
    }
  }

  return {};
}
