#include "WritePoleFigure.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Utilities/ArrayCreationUtilities.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"

#include <EbsdLib/Core/EbsdLibConstants.h>
#include <EbsdLib/LaueOps/CubicLowOps.h>
#include <EbsdLib/LaueOps/CubicOps.h>
#include <EbsdLib/LaueOps/HexagonalLowOps.h>
#include <EbsdLib/LaueOps/HexagonalOps.h>
#include <EbsdLib/LaueOps/MonoclinicOps.h>
#include <EbsdLib/LaueOps/OrthoRhombicOps.h>
#include <EbsdLib/LaueOps/TetragonalLowOps.h>
#include <EbsdLib/LaueOps/TetragonalOps.h>
#include <EbsdLib/LaueOps/TriclinicOps.h>
#include <EbsdLib/LaueOps/TrigonalLowOps.h>
#include <EbsdLib/LaueOps/TrigonalOps.h>
#include <EbsdLib/Utilities/LambertUtilities.h>
#include <EbsdLib/Utilities/ModifiedLambertProjection.h>
#include <EbsdLib/Utilities/PngWriter.h>
#include <EbsdLib/Utilities/PoleFigureCompositor.h>

using namespace nx::core;

namespace
{
const bool k_UseDiscreteHeatMap = false;

/**
 * @class ComputeIntensityStereographicProjection
 * @brief Converts one sphere-coordinate family to an intensity image.
 *
 * Discrete mode counts projected samples. Lambert mode builds two hemisphere
 * squares and creates their stereographic projection. Parallel tasks use
 * separate coordinate and intensity arrays.
 */
class ComputeIntensityStereographicProjection
{
public:
  ComputeIntensityStereographicProjection(ebsdlib::FloatArrayType* xyzCoords, ebsdlib::PoleFigureConfiguration_t* config, ebsdlib::DoubleArrayType* intensity, bool normalizeToMRD)
  : m_XYZCoords(xyzCoords)
  , m_Config(config)
  , m_Intensity(intensity)
  , m_NormalizeToMRD(normalizeToMRD)
  {
  }

  void operator()() const
  {
    m_Intensity->resizeTuples(m_Config->imageDim * m_Config->imageDim);
    m_Intensity->initializeWithZeros();

    if(m_Config->discrete)
    {
      int halfDim = m_Config->imageDim / 2;
      double* intensity = m_Intensity->getPointer(0);
      usize numCoords = m_XYZCoords->getNumberOfTuples();
      float32* xyzPtr = m_XYZCoords->getPointer(0);
      for(usize i = 0; i < numCoords; i++)
      {
        // Reflect southern-hemisphere directions before stereographic projection.
        if(xyzPtr[i * 3 + 2] < 0.0f)
        {
          xyzPtr[i * 3 + 0] *= -1.0f;
          xyzPtr[i * 3 + 1] *= -1.0f;
          xyzPtr[i * 3 + 2] *= -1.0f;
        }
        float32 x = xyzPtr[i * 3] / (1 + xyzPtr[i * 3 + 2]);
        float32 y = xyzPtr[i * 3 + 1] / (1 + xyzPtr[i * 3 + 2]);

        int xCoord = static_cast<int>(x * static_cast<float32>(halfDim - 1)) + halfDim;
        int yCoord = static_cast<int>(y * static_cast<float32>(halfDim - 1)) + halfDim;

        usize index = (yCoord * m_Config->imageDim) + xCoord;

        intensity[index]++;
      }
    }
    else
    {
      ebsdlib::ModifiedLambertProjection::Pointer lambert = ebsdlib::ModifiedLambertProjection::LambertBallToSquare(m_XYZCoords, m_Config->lambertDim, m_Config->sphereRadius);
      if(m_NormalizeToMRD)
      {
        lambert->normalizeSquaresToMRD();
      }
      lambert->createStereographicProjection(m_Config->imageDim, *m_Intensity);
    }
  }

private:
  ebsdlib::FloatArrayType* m_XYZCoords = nullptr;
  ebsdlib::PoleFigureConfiguration_t* m_Config = nullptr;
  ebsdlib::DoubleArrayType* m_Intensity = nullptr;
  bool m_NormalizeToMRD = false;
};

// -----------------------------------------------------------------------------
template <typename Ops>
std::vector<ebsdlib::UInt8ArrayType::Pointer> makePoleFigures(ebsdlib::PoleFigureConfiguration_t& config)
{
  Ops ops;
  return ops.generatePoleFigure(config);
}

template <typename OpsType>
std::vector<ebsdlib::DoubleArrayType::Pointer> createIntensityPoleFigures(ebsdlib::PoleFigureConfiguration_t& config, bool normalizeToMRD)
{
  OpsType ops;
  std::string label0 = std::string("<001>");
  std::string label1 = std::string("<011>");
  std::string label2 = std::string("<111>");
  if(!config.labels.empty())
  {
    label0 = config.labels.at(0);
  }
  if(config.labels.size() > 1)
  {
    label1 = config.labels.at(1);
  }
  if(config.labels.size() > 2)
  {
    label2 = config.labels.at(2);
  }

  const usize numOrientations = config.eulers->getNumberOfTuples();

  // Allocate one sphere-coordinate array for each pole family.
  std::array<int32, 3> symSize = ops.getNumSymmetry();

  const ShapeType dims = {3};
  const ebsdlib::FloatArrayType::Pointer xyz001 = ebsdlib::FloatArrayType::CreateArray(numOrientations * symSize[0], dims, label0 + std::string("xyzCoords"), true);
  const ebsdlib::FloatArrayType::Pointer xyz011 = ebsdlib::FloatArrayType::CreateArray(numOrientations * symSize[1], dims, label1 + std::string("xyzCoords"), true);
  const ebsdlib::FloatArrayType::Pointer xyz111 = ebsdlib::FloatArrayType::CreateArray(numOrientations * symSize[2], dims, label2 + std::string("xyzCoords"), true);

  config.sphereRadius = 1.0f;

  // EbsdLib expands each Euler orientation through the selected symmetry operators.
  ops.generateSphereCoordsFromEulers(config.eulers, xyz001.get(), xyz011.get(), xyz111.get(), config.hexConvention);

  // Each intensity array receives a Lambert or discrete stereographic image.
  const ebsdlib::DoubleArrayType::Pointer intensity001 = ebsdlib::DoubleArrayType::CreateArray(config.imageDim * config.imageDim, label0 + "_Intensity_Image", true);
  const ebsdlib::DoubleArrayType::Pointer intensity011 = ebsdlib::DoubleArrayType::CreateArray(config.imageDim * config.imageDim, label1 + "_Intensity_Image", true);
  const ebsdlib::DoubleArrayType::Pointer intensity111 = ebsdlib::DoubleArrayType::CreateArray(config.imageDim * config.imageDim, label2 + "_Intensity_Image", true);

  // Pole families use independent arrays and can run in parallel.
  ParallelTaskAlgorithm taskRunner;
  taskRunner.setParallelizationEnabled(true);
  taskRunner.execute(ComputeIntensityStereographicProjection(xyz001.get(), &config, intensity001.get(), normalizeToMRD));
  taskRunner.execute(ComputeIntensityStereographicProjection(xyz011.get(), &config, intensity011.get(), normalizeToMRD));
  taskRunner.execute(ComputeIntensityStereographicProjection(xyz111.get(), &config, intensity111.get(), normalizeToMRD));
  taskRunner.wait();

  return {intensity001, intensity011, intensity111};
}

template <typename T>
typename EbsdDataArray<T>::Pointer flipAndMirrorPoleFigure(EbsdDataArray<T>* src, const ebsdlib::PoleFigureConfiguration_t& config)
{
  typename EbsdDataArray<T>::Pointer converted = EbsdDataArray<T>::CreateArray(config.imageDim * config.imageDim, src->getComponentDimensions(), src->getName(), true);
  // Reverse row order while preserving each row's X order.
  for(int y = 0; y < config.imageDim; y++)
  {
    const int destY = config.imageDim - 1 - y;
    for(int x = 0; x < config.imageDim; x++)
    {
      const usize indexSrc = y * config.imageDim + x;
      const usize indexDest = destY * config.imageDim + x;

      T* argbPtr = src->getTuplePointer(indexSrc);
      converted->setTuple(indexDest, argbPtr);
    }
  }
  return converted;
}

} // namespace

WritePoleFigure::WritePoleFigure(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WritePoleFigureInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

WritePoleFigure::~WritePoleFigure() noexcept = default;

Result<> WritePoleFigure::operator()()
{
  // Create the requested disk-output directory before phase processing.
  if(m_InputValues->WriteImageToDisk)
  {
    if(!fs::exists(m_InputValues->OutputPath))
    {
      if(!fs::create_directories(m_InputValues->OutputPath))
      {
        return MakeErrorResult(-67020, fmt::format("Unable to create output directory {}", m_InputValues->OutputPath.string()));
      }
    }
  }

  const std::vector<ebsdlib::LaueOps::Pointer> orientationOps = ebsdlib::LaueOps::GetAllOrientationOps();

  const nx::core::Float32Array& eulerAngles = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CellEulerAnglesArrayPath);
  auto& phases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellPhasesArrayPath);

  auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);
  auto& materialNames = m_DataStructure.getDataRefAs<StringArray>(m_InputValues->MaterialNameArrayPath);

  std::unique_ptr<MaskCompareUtilities::MaskCompare> maskCompare = nullptr;
  if(m_InputValues->UseMask)
  {
    try
    {
      maskCompare = MaskCompareUtilities::InstantiateMaskCompare(m_DataStructure, m_InputValues->MaskArrayPath);
    } catch(const std::out_of_range& exception)
    {
      // Direct callers can bypass preflight, so return an invalid mask as a Result.
      return MakeErrorResult(-53900, fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->MaskArrayPath.toString()));
    }
  }

  const usize numPoints = eulerAngles.getNumberOfTuples();
  const usize numPhases = crystalStructures.getNumberOfTuples();

  // Initialize output geometry to one figure. A composite can resize it later.
  ShapeType tupleShape = {1, static_cast<usize>(m_InputValues->ImageSize), static_cast<usize>(m_InputValues->ImageSize)};
  auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->OutputImageGeometryPath);
  auto cellAttrMatPath = imageGeom.getCellDataPath();
  imageGeom.setDimensions({static_cast<usize>(m_InputValues->ImageSize), static_cast<usize>(m_InputValues->ImageSize), 1});
  if(Result<> resizeResult = imageGeom.getCellData()->resizeTuples(tupleShape); resizeResult.invalid())
  {
    return ConvertResult(std::move(resizeResult));
  }

  // Phase and Euler page buffers total one MiB. MaskCompareUtilities does not
  // use this page and can still perform per-tuple store access.
  constexpr usize k_StreamChunkTuples = 65536;

  std::vector<int32> phaseChunk(k_StreamChunkTuples);
  std::vector<float32> eulerChunk(k_StreamChunkTuples * 3);

  // Scan cell inputs twice per phase. The first pass counts selected tuples.
  // The second pass fills an EbsdLib array whose size equals the phase count.
  for(usize phase = 1; phase < numPhases; ++phase)
  {
    usize count = 0;
    // Count first so the phase array needs one allocation.
    for(usize chunkStart = 0; chunkStart < numPoints; chunkStart += k_StreamChunkTuples)
    {
      const usize chunkLen = std::min(k_StreamChunkTuples, numPoints - chunkStart);
      if(Result<> ioResult = phases.getDataStoreRef().copyIntoBuffer(chunkStart, nonstd::span<int32>(phaseChunk.data(), chunkLen)); ioResult.invalid())
      {
        return ConvertResult(std::move(ioResult));
      }
      for(usize i = 0; i < chunkLen; ++i)
      {
        if(phaseChunk[i] == static_cast<int32>(phase))
        {
          const usize globalIdx = chunkStart + i;
          if(!m_InputValues->UseMask || maskCompare->isTrue(globalIdx))
          {
            count++;
          }
        }
      }
    }
    const ShapeType eulerCompDim = {3};
    const ebsdlib::FloatArrayType::Pointer subEulerAnglesPtr = ebsdlib::FloatArrayType::CreateArray(count, eulerCompDim, "Euler_Angles_Per_Phase", true);
    subEulerAnglesPtr->initializeWithValue(std::numeric_limits<float32>::signaling_NaN());
    ebsdlib::FloatArrayType& subEulerAngles = *subEulerAnglesPtr;

    // Fill the allocated phase array during the second input scan.
    count = 0;
    for(usize chunkStart = 0; chunkStart < numPoints; chunkStart += k_StreamChunkTuples)
    {
      const usize chunkLen = std::min(k_StreamChunkTuples, numPoints - chunkStart);
      if(Result<> ioResult = phases.getDataStoreRef().copyIntoBuffer(chunkStart, nonstd::span<int32>(phaseChunk.data(), chunkLen)); ioResult.invalid())
      {
        return ConvertResult(std::move(ioResult));
      }
      if(Result<> ioResult = eulerAngles.getDataStoreRef().copyIntoBuffer(chunkStart * 3, nonstd::span<float32>(eulerChunk.data(), chunkLen * 3)); ioResult.invalid())
      {
        return ConvertResult(std::move(ioResult));
      }
      for(usize i = 0; i < chunkLen; ++i)
      {
        if(phaseChunk[i] == static_cast<int32>(phase))
        {
          const usize globalIdx = chunkStart + i;
          if(!m_InputValues->UseMask || maskCompare->isTrue(globalIdx))
          {
            subEulerAngles[count * 3] = eulerChunk[i * 3];
            subEulerAngles[count * 3 + 1] = eulerChunk[i * 3 + 1];
            subEulerAngles[count * 3 + 2] = eulerChunk[i * 3 + 2];
            count++;
          }
        }
      }
    }
    if(subEulerAnglesPtr->getNumberOfTuples() == 0)
    {
      continue;
    }

    ebsdlib::PoleFigureConfiguration_t config;
    config.eulers = subEulerAnglesPtr.get();
    config.imageDim = m_InputValues->ImageSize;
    config.lambertDim = m_InputValues->LambertSize;
    config.numColors = m_InputValues->NumColors;
    config.discrete = (static_cast<WritePoleFigure::Algorithm>(m_InputValues->GenerationAlgorithm) == WritePoleFigure::Algorithm::Discrete);
    config.discreteHeatMap = k_UseDiscreteHeatMap;
    config.hexConvention = m_InputValues->HexConvention;
    config.flipFinalImage = m_InputValues->FlipFinalImage;
    config.axisNames = std::vector<std::string>{"A1", "A2", "A3"};

    m_MessageHandler({IFilter::Message::Type::Info, fmt::format("Generating Pole Figures for Phase {}", phase)});
    if(m_InputValues->SaveIntensityData)
    {
      std::vector<ebsdlib::DoubleArrayType::Pointer> intensityImages;

      switch(crystalStructures[phase])
      {
      case ebsdlib::CrystalStructure::Cubic_High:
        intensityImages = createIntensityPoleFigures<ebsdlib::CubicOps>(config, m_InputValues->NormalizeToMRD);
        break;
      case ebsdlib::CrystalStructure::Cubic_Low:
        intensityImages = createIntensityPoleFigures<ebsdlib::CubicLowOps>(config, m_InputValues->NormalizeToMRD);
        break;
      case ebsdlib::CrystalStructure::Hexagonal_High:
        intensityImages = createIntensityPoleFigures<ebsdlib::HexagonalOps>(config, m_InputValues->NormalizeToMRD);
        break;
      case ebsdlib::CrystalStructure::Hexagonal_Low:
        intensityImages = createIntensityPoleFigures<ebsdlib::HexagonalLowOps>(config, m_InputValues->NormalizeToMRD);
        break;
      case ebsdlib::CrystalStructure::Trigonal_High:
        intensityImages = createIntensityPoleFigures<ebsdlib::TrigonalOps>(config, m_InputValues->NormalizeToMRD);
        break;
      case ebsdlib::CrystalStructure::Trigonal_Low:
        intensityImages = createIntensityPoleFigures<ebsdlib::TrigonalLowOps>(config, m_InputValues->NormalizeToMRD);
        break;
      case ebsdlib::CrystalStructure::Tetragonal_High:
        intensityImages = createIntensityPoleFigures<ebsdlib::TetragonalOps>(config, m_InputValues->NormalizeToMRD);
        break;
      case ebsdlib::CrystalStructure::Tetragonal_Low:
        intensityImages = createIntensityPoleFigures<ebsdlib::TetragonalLowOps>(config, m_InputValues->NormalizeToMRD);
        break;
      case ebsdlib::CrystalStructure::OrthoRhombic:
        intensityImages = createIntensityPoleFigures<ebsdlib::OrthoRhombicOps>(config, m_InputValues->NormalizeToMRD);
        break;
      case ebsdlib::CrystalStructure::Monoclinic:
        intensityImages = createIntensityPoleFigures<ebsdlib::MonoclinicOps>(config, m_InputValues->NormalizeToMRD);
        break;
      case ebsdlib::CrystalStructure::Triclinic:
        intensityImages = createIntensityPoleFigures<ebsdlib::TriclinicOps>(config, m_InputValues->NormalizeToMRD);
        break;
      default:
        m_MessageHandler({IFilter::Message::Type::Warning,
                          fmt::format("Phase {} has unknown crystal structure value {}; no pole figures will be generated for this phase.", phase, static_cast<uint32>(crystalStructures[phase]))});
        break;
      }

      if(intensityImages.size() == 3)
      {
        DataPath amPath = m_InputValues->IntensityGeometryDataPath.createChildPath(write_pole_figure::k_ImageAttrMatName);
        // Preflight creates phase-one arrays. Later phases create their arrays here.
        if(phase > 1)
        {
          const std::vector<size_t> intensityImageDims = {static_cast<usize>(config.imageDim), static_cast<usize>(config.imageDim), 1ULL};
          DataPath arrayDataPath = amPath.createChildPath(fmt::format("Phase_{}_{}", phase, m_InputValues->IntensityPlot1Name));
          Result<> creationResult = ArrayCreationUtilities::CreateArray<float64>(m_DataStructure, intensityImageDims, {1ULL}, arrayDataPath, IDataAction::Mode::Execute);
          if(creationResult.invalid())
          {
            return ConvertResult(std::move(creationResult));
          }

          arrayDataPath = amPath.createChildPath(fmt::format("Phase_{}_{}", phase, m_InputValues->IntensityPlot2Name));
          creationResult = ArrayCreationUtilities::CreateArray<float64>(m_DataStructure, intensityImageDims, {1ULL}, arrayDataPath, IDataAction::Mode::Execute);
          if(creationResult.invalid())
          {
            return ConvertResult(std::move(creationResult));
          }

          arrayDataPath = amPath.createChildPath(fmt::format("Phase_{}_{}", phase, m_InputValues->IntensityPlot3Name));
          creationResult = ArrayCreationUtilities::CreateArray<float64>(m_DataStructure, intensityImageDims, {1ULL}, arrayDataPath, IDataAction::Mode::Execute);
          if(creationResult.invalid())
          {
            return ConvertResult(std::move(creationResult));
          }
        }

        auto intensityPlot1Array = m_DataStructure.getDataRefAs<Float64Array>(amPath.createChildPath(fmt::format("Phase_{}_{}", phase, m_InputValues->IntensityPlot1Name)));
        auto intensityPlot2Array = m_DataStructure.getDataRefAs<Float64Array>(amPath.createChildPath(fmt::format("Phase_{}_{}", phase, m_InputValues->IntensityPlot2Name)));
        auto intensityPlot3Array = m_DataStructure.getDataRefAs<Float64Array>(amPath.createChildPath(fmt::format("Phase_{}_{}", phase, m_InputValues->IntensityPlot3Name)));

        std::vector<size_t> compDims = {1ULL};
        for(int imageIndex = 0; imageIndex < intensityImages.size(); imageIndex++)
        {
          intensityImages[imageIndex] = flipAndMirrorPoleFigure<double>(intensityImages[imageIndex].get(), config);
        }

        // Each intensity image uses one destination transfer.
        {
          const usize plotElems = static_cast<usize>(intensityImages[0]->getNumberOfTuples()) * intensityImages[0]->getNumberOfComponents();
          if(Result<> ioResult = intensityPlot1Array.getDataStoreRef().copyFromBuffer(0, nonstd::span<const float64>(intensityImages[0]->getPointer(0), plotElems)); ioResult.invalid())
          {
            return ConvertResult(std::move(ioResult));
          }
        }
        {
          const usize plotElems = static_cast<usize>(intensityImages[1]->getNumberOfTuples()) * intensityImages[1]->getNumberOfComponents();
          if(Result<> ioResult = intensityPlot2Array.getDataStoreRef().copyFromBuffer(0, nonstd::span<const float64>(intensityImages[1]->getPointer(0), plotElems)); ioResult.invalid())
          {
            return ConvertResult(std::move(ioResult));
          }
        }
        {
          const usize plotElems = static_cast<usize>(intensityImages[2]->getNumberOfTuples()) * intensityImages[2]->getNumberOfComponents();
          if(Result<> ioResult = intensityPlot3Array.getDataStoreRef().copyFromBuffer(0, nonstd::span<const float64>(intensityImages[2]->getPointer(0), plotElems)); ioResult.invalid())
          {
            return ConvertResult(std::move(ioResult));
          }
        }

        DataPath metaDataPath = m_InputValues->IntensityGeometryDataPath.createChildPath(write_pole_figure::k_MetaDataName);
        auto metaDataArrayRef = m_DataStructure.getDataRefAs<StringArray>(metaDataPath);
        if(metaDataArrayRef.getNumberOfTuples() != numPhases)
        {
          if(Result<> resizeResult = metaDataArrayRef.resizeTuples(std::vector<usize>{numPhases}); resizeResult.invalid())
          {
            return ConvertResult(std::move(resizeResult));
          }
        }

        std::vector<std::string> laueNames = ebsdlib::LaueOps::GetLaueNames();
        const uint32_t laueIndex = crystalStructures[phase];
        const std::string materialName = materialNames[phase];

        metaDataArrayRef[phase] = fmt::format("Phase Num: {}\nMaterial Name: {}\nLaue Group: {}\nHemisphere: Northern\nSamples: {}\nLambert Square Dim: {}", phase, materialName, laueNames[laueIndex],
                                              config.eulers->getNumberOfTuples(), config.lambertDim);
      }
    }

    if(m_InputValues->SaveAsImageGeometry || m_InputValues->WriteImageToDisk)
    {
      ebsdlib::CompositePoleFigureConfiguration_t compositeConfig;
      compositeConfig.eulers = subEulerAnglesPtr.get();
      compositeConfig.imageDim = m_InputValues->ImageSize;
      compositeConfig.lambertDim = m_InputValues->LambertSize;
      compositeConfig.numColors = m_InputValues->NumColors;
      compositeConfig.minScale = config.minScale;
      compositeConfig.maxScale = config.maxScale;
      compositeConfig.sphereRadius = config.sphereRadius;
      compositeConfig.discrete = config.discrete;
      compositeConfig.discreteHeatMap = config.discreteHeatMap;
      compositeConfig.markerStyle.radiusFraction = m_InputValues->DiscreteMarkerRadius;
      compositeConfig.colorMap = config.colorMap;
      compositeConfig.poleFigureNames = config.labels;
      compositeConfig.order = config.order;
      compositeConfig.axisNames = config.axisNames;

      compositeConfig.flipFinalImage = config.flipFinalImage;
      compositeConfig.layoutType = static_cast<ebsdlib::PoleFigureLayoutType>(m_InputValues->ImageLayout);
      compositeConfig.laueOpsIndex = crystalStructures[phase];
      compositeConfig.phaseName = materialNames[phase];
      compositeConfig.phaseNumber = static_cast<int32>(phase);
      compositeConfig.title = m_InputValues->Title;
      compositeConfig.hexConvention = m_InputValues->HexConvention;

      // Discrete figures use the marker renderer. Other figures use the raster compositor.
      ebsdlib::CompositePoleFigureResult compositeResult = ebsdlib::GeneratePoleFigureComposite(compositeConfig);

      if(compositeResult.image == nullptr)
      {
        continue;
      }

      const int32 pageWidth = compositeResult.width;
      const int32 pageHeight = compositeResult.height;

      if(m_InputValues->SaveAsImageGeometry)
      {
        // Match the output geometry to the composite page.
        imageGeom.setDimensions({static_cast<usize>(pageWidth), static_cast<usize>(pageHeight), 1});
        if(Result<> resizeResult = imageGeom.getCellData()->resizeTuples({1, static_cast<usize>(pageHeight), static_cast<usize>(pageWidth)}); resizeResult.invalid())
        {
          return ConvertResult(std::move(resizeResult));
        }
        tupleShape[0] = 1;
        tupleShape[1] = pageHeight;
        tupleShape[2] = pageWidth;
        auto imageArrayPath = cellAttrMatPath.createChildPath(fmt::format("Phase_{}", phase));
        auto arrayCreationResult = ArrayCreationUtilities::CreateArray<uint8>(m_DataStructure, tupleShape, {3ULL}, imageArrayPath, IDataAction::Mode::Execute);
        if(arrayCreationResult.invalid())
        {
          return arrayCreationResult;
        }

        // Pack RGB components from the RGBA composite and use one destination transfer.
        auto& imageData = m_DataStructure.getDataRefAs<UInt8Array>(imageArrayPath);
        imageData.fill(0);
        const usize tupleCount = static_cast<usize>(pageHeight) * pageWidth;
        const uint8_t* rgbaPtr = compositeResult.image->getPointer(0);
        std::vector<uint8> rgbBuf(tupleCount * 3);
        for(usize t = 0; t < tupleCount; t++)
        {
          rgbBuf[t * 3 + 0] = rgbaPtr[t * 4 + 0];
          rgbBuf[t * 3 + 1] = rgbaPtr[t * 4 + 1];
          rgbBuf[t * 3 + 2] = rgbaPtr[t * 4 + 2];
        }
        if(Result<> ioResult = imageData.getDataStoreRef().copyFromBuffer(0, nonstd::span<const uint8>(rgbBuf.data(), rgbBuf.size())); ioResult.invalid())
        {
          return ConvertResult(std::move(ioResult));
        }
      }

      if(m_InputValues->WriteImageToDisk)
      {
        // Disk output is PNG regardless of the retained ImageFormat setting.
        const std::string filename = fmt::format("{}/{}{}.png", m_InputValues->OutputPath.string(), m_InputValues->ImagePrefix, phase);
        auto result = PngWriter::WriteColorImage(filename, pageWidth, pageHeight, 4, compositeResult.image->getPointer(0));
        if(result.first < 0)
        {
          return MakeErrorResult(-53900, fmt::format("Error writing pole figure image '{}' to disk.\n    Error Code from PNG Writer: {}\n    Message: {}", filename, result.first, result.second));
        }
      }
    }
  }
  return {};
}
