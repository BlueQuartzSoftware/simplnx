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
  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->image_geometry_path);
  const usize totalPoints = imageGeom.getNumXCells() * imageGeom.getNumYCells();
  const usize offset = index * totalPoints;

  {
    const float32 degToRad = m_EspritInputValues->degrees_to_radians ? nx::core::constants::k_PiOver180F : 1.0f;
    const auto* phi1Ptr = reinterpret_cast<float32*>(m_Reader->getPointerByName(ebsdlib::H5Esprit::phi1));
    const auto* phiPtr = reinterpret_cast<float32*>(m_Reader->getPointerByName(ebsdlib::H5Esprit::PHI));
    const auto* phi2Ptr = reinterpret_cast<float32*>(m_Reader->getPointerByName(ebsdlib::H5Esprit::phi2));
    auto& eulerAngles = m_DataStructure.getDataRefAs<Float32ArrayType>(m_InputValues->cell_attribute_matrix_path.createChildPath(ebsdlib::Esprit::EulerAngles));

    const auto* m1Ptr = reinterpret_cast<float32*>(m_Reader->getPointerByName(ebsdlib::H5Esprit::MAD));
    auto& mad = m_DataStructure.getDataRefAs<Float32ArrayType>(m_InputValues->cell_attribute_matrix_path.createChildPath(ebsdlib::H5Esprit::MAD));

    const auto* nIndBandsPtr = reinterpret_cast<int32*>(m_Reader->getPointerByName(ebsdlib::H5Esprit::NIndexedBands));
    auto& nIndexBands = m_DataStructure.getDataRefAs<Int32ArrayType>(m_InputValues->cell_attribute_matrix_path.createChildPath(ebsdlib::H5Esprit::NIndexedBands));

    const auto* p1Ptr = reinterpret_cast<int32*>(m_Reader->getPointerByName(ebsdlib::H5Esprit::Phase));
    auto& phase = m_DataStructure.getDataRefAs<Int32ArrayType>(m_InputValues->cell_attribute_matrix_path.createChildPath(ebsdlib::H5Esprit::Phase));

    const auto* radBandCntPtr = reinterpret_cast<int32*>(m_Reader->getPointerByName(ebsdlib::H5Esprit::RadonBandCount));
    auto& radonBandCount = m_DataStructure.getDataRefAs<Int32ArrayType>(m_InputValues->cell_attribute_matrix_path.createChildPath(ebsdlib::H5Esprit::RadonBandCount));

    const auto* radQualPtr = reinterpret_cast<float32*>(m_Reader->getPointerByName(ebsdlib::H5Esprit::RadonQuality));
    auto& radonQuality = m_DataStructure.getDataRefAs<Float32ArrayType>(m_InputValues->cell_attribute_matrix_path.createChildPath(ebsdlib::H5Esprit::RadonQuality));

    const auto* xBmPtr = reinterpret_cast<int32*>(m_Reader->getPointerByName(ebsdlib::H5Esprit::XBEAM));
    auto& xBeam = m_DataStructure.getDataRefAs<Int32ArrayType>(m_InputValues->cell_attribute_matrix_path.createChildPath(ebsdlib::H5Esprit::XBEAM));

    const auto* yBmPtr = reinterpret_cast<int32*>(m_Reader->getPointerByName(ebsdlib::H5Esprit::YBEAM));
    auto& yBeam = m_DataStructure.getDataRefAs<Int32ArrayType>(m_InputValues->cell_attribute_matrix_path.createChildPath(ebsdlib::H5Esprit::YBEAM));

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
          eulerChunk[i * 3] = phi1Ptr[chunkStart + i] * degToRad;
          eulerChunk[i * 3 + 1] = phiPtr[chunkStart + i] * degToRad;
          eulerChunk[i * 3 + 2] = phi2Ptr[chunkStart + i] * degToRad;
        }
        if(Result<> ioResult = eulerStore.copyFromBuffer((offset + chunkStart) * 3, nonstd::span<const float32>(eulerChunk.data(), chunkCount * 3)); ioResult.invalid())
        {
          return ConvertResult(std::move(ioResult));
        }
      }
    }

    // Each scalar channel uses one scan-sized transfer.
    if(Result<> ioResult = mad.getDataStoreRef().copyFromBuffer(offset, nonstd::span<const float32>(m1Ptr, totalPoints)); ioResult.invalid())
    {
      return ConvertResult(std::move(ioResult));
    }
    if(Result<> ioResult = nIndexBands.getDataStoreRef().copyFromBuffer(offset, nonstd::span<const int32>(nIndBandsPtr, totalPoints)); ioResult.invalid())
    {
      return ConvertResult(std::move(ioResult));
    }
    if(Result<> ioResult = phase.getDataStoreRef().copyFromBuffer(offset, nonstd::span<const int32>(p1Ptr, totalPoints)); ioResult.invalid())
    {
      return ConvertResult(std::move(ioResult));
    }
    if(Result<> ioResult = radonBandCount.getDataStoreRef().copyFromBuffer(offset, nonstd::span<const int32>(radBandCntPtr, totalPoints)); ioResult.invalid())
    {
      return ConvertResult(std::move(ioResult));
    }
    if(Result<> ioResult = radonQuality.getDataStoreRef().copyFromBuffer(offset, nonstd::span<const float32>(radQualPtr, totalPoints)); ioResult.invalid())
    {
      return ConvertResult(std::move(ioResult));
    }
    if(Result<> ioResult = xBeam.getDataStoreRef().copyFromBuffer(offset, nonstd::span<const int32>(xBmPtr, totalPoints)); ioResult.invalid())
    {
      return ConvertResult(std::move(ioResult));
    }
    if(Result<> ioResult = yBeam.getDataStoreRef().copyFromBuffer(offset, nonstd::span<const int32>(yBmPtr, totalPoints)); ioResult.invalid())
    {
      return ConvertResult(std::move(ioResult));
    }
  }

  if(m_InputValues->read_pattern_data)
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
      auto& patternData = m_DataStructure.getDataRefAs<UInt8ArrayType>(m_InputValues->cell_attribute_matrix_path.createChildPath(ebsdlib::H5Esprit::RawPatterns));
      const usize numComponents = patternData.getNumberOfComponents();
      if(Result<> ioResult = patternData.getDataStoreRef().copyFromBuffer(offset * numComponents, nonstd::span<const uint8>(patternDataPtr, totalPoints * numComponents)); ioResult.invalid())
      {
        return ConvertResult(std::move(ioResult));
      }
    }
  }

  return {};
}
