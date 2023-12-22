#include "WritePoleFigure.hpp"

#include "OrientationAnalysis/utilities/FiraSansRegular.hpp"
#include "OrientationAnalysis/utilities/Fonts.hpp"
#include "OrientationAnalysis/utilities/LatoBold.hpp"
#include "OrientationAnalysis/utilities/LatoRegular.hpp"
#include "OrientationAnalysis/utilities/TiffWriter.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/Common/RgbColor.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include "EbsdLib/Core/EbsdLibConstants.h"
#include "EbsdLib/LaueOps/CubicLowOps.h"
#include "EbsdLib/LaueOps/CubicOps.h"
#include "EbsdLib/LaueOps/HexagonalLowOps.h"
#include "EbsdLib/LaueOps/HexagonalOps.h"
#include "EbsdLib/LaueOps/MonoclinicOps.h"
#include "EbsdLib/LaueOps/OrthoRhombicOps.h"
#include "EbsdLib/LaueOps/TetragonalLowOps.h"
#include "EbsdLib/LaueOps/TetragonalOps.h"
#include "EbsdLib/LaueOps/TriclinicOps.h"
#include "EbsdLib/LaueOps/TrigonalLowOps.h"
#include "EbsdLib/LaueOps/TrigonalOps.h"
#include "EbsdLib/Utilities/ComputeStereographicProjection.h"

#define CANVAS_ITY_IMPLEMENTATION
#include <canvas_ity.hpp>

using namespace nx::core;

namespace
{
const bool k_UseDiscreteHeatMap = false;

// -----------------------------------------------------------------------------
template <typename Ops>
std::vector<EbsdLib::UInt8ArrayType::Pointer> makePoleFigures(PoleFigureConfiguration_t& config)
{
  Ops ops;
  return ops.generatePoleFigure(config);
}

template <typename OpsType>
std::vector<EbsdLib::DoubleArrayType ::Pointer> createIntensityPoleFigures(PoleFigureConfiguration_t& config)
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

  const size_t numOrientations = config.eulers->getNumberOfTuples();

  // Create an Array to hold the XYZ Coordinates which are the coords on the sphere.
  // this is size for CUBIC ONLY, <001> Family
  std::array<int32_t, 3> symSize = ops.getNumSymmetry();

  const std::vector<size_t> dims = {3};
  const EbsdLib::FloatArrayType::Pointer xyz001 = EbsdLib::FloatArrayType::CreateArray(numOrientations * symSize[0], dims, label0 + std::string("xyzCoords"), true);
  // this is size for CUBIC ONLY, <011> Family
  const EbsdLib::FloatArrayType::Pointer xyz011 = EbsdLib::FloatArrayType::CreateArray(numOrientations * symSize[1], dims, label1 + std::string("xyzCoords"), true);
  // this is size for CUBIC ONLY, <111> Family
  const EbsdLib::FloatArrayType::Pointer xyz111 = EbsdLib::FloatArrayType::CreateArray(numOrientations * symSize[2], dims, label2 + std::string("xyzCoords"), true);

  config.sphereRadius = 1.0f;

  // Generate the coords on the sphere **** Parallelized
  ops.generateSphereCoordsFromEulers(config.eulers, xyz001.get(), xyz011.get(), xyz111.get());

  // These arrays hold the "intensity" images which eventually get converted to an actual Color RGB image
  // Generate the modified Lambert projection images (Squares, 2 of them, 1 for northern hemisphere, 1 for southern hemisphere
  const EbsdLib::DoubleArrayType::Pointer intensity001 = EbsdLib::DoubleArrayType::CreateArray(config.imageDim * config.imageDim, label0 + "_Intensity_Image", true);
  const EbsdLib::DoubleArrayType::Pointer intensity011 = EbsdLib::DoubleArrayType::CreateArray(config.imageDim * config.imageDim, label1 + "_Intensity_Image", true);
  const EbsdLib::DoubleArrayType::Pointer intensity111 = EbsdLib::DoubleArrayType::CreateArray(config.imageDim * config.imageDim, label2 + "_Intensity_Image", true);

  // Compute the Stereographic Data in parallel
  ParallelTaskAlgorithm taskRunner;
  taskRunner.setParallelizationEnabled(true);
  taskRunner.execute(ops.ComputeStereographicProjection(xyz001.get(), &config, intensity001.get()));
  taskRunner.execute(ComputeStereographicProjection(xyz011.get(), &config, intensity011.get()));
  taskRunner.execute(ComputeStereographicProjection(xyz111.get(), &config, intensity111.get()));
  taskRunner.wait(); // This will spill over if the number of DataArrays to process does not divide evenly by the number of threads.

  // Find the Max and Min values based on ALL 3 arrays. We can color scale them all the same
  double max = std::numeric_limits<double>::min();
  double min = std::numeric_limits<double>::max();

  double* dPtr = intensity001->getPointer(0);
  size_t count = intensity001->getNumberOfTuples();
  for(size_t i = 0; i < count; ++i)
  {
    if(dPtr[i] > max)
    {
      max = dPtr[i];
    }
    if(dPtr[i] < min)
    {
      min = dPtr[i];
    }
  }

  dPtr = intensity011->getPointer(0);
  count = intensity011->getNumberOfTuples();
  for(size_t i = 0; i < count; ++i)
  {
    if(dPtr[i] > max)
    {
      max = dPtr[i];
    }
    if(dPtr[i] < min)
    {
      min = dPtr[i];
    }
  }

  dPtr = intensity111->getPointer(0);
  count = intensity111->getNumberOfTuples();
  for(size_t i = 0; i < count; ++i)
  {
    if(dPtr[i] > max)
    {
      max = dPtr[i];
    }
    if(dPtr[i] < min)
    {
      min = dPtr[i];
    }
  }

  config.minScale = min;
  config.maxScale = max;

  return {intensity001, intensity011, intensity111};
}

// -----------------------------------------------------------------------------
EbsdLib::UInt8ArrayType::Pointer flipAndMirrorPoleFigure(EbsdLib::UInt8ArrayType* src, const PoleFigureConfiguration_t& config)
{
  EbsdLib::UInt8ArrayType::Pointer converted = EbsdLib::UInt8ArrayType::CreateArray(static_cast<size_t>(config.imageDim * config.imageDim), {4}, src->getName(), true);
  // We need to flip the image "vertically", which means the bottom row becomes
  // the top row and convert from BGRA to RGB ordering (This is a Little Endian code)
  // If this is ever compiled on a BIG ENDIAN machine the colors will be off.
  for(int y = 0; y < config.imageDim; y++)
  {
    const int destY = config.imageDim - 1 - y;
    for(int x = 0; x < config.imageDim; x++)
    {
      const size_t indexSrc = y * config.imageDim + x;
      const size_t indexDest = destY * config.imageDim + x;

      uint8_t* argbPtr = src->getTuplePointer(indexSrc);
      uint8_t* destPtr = converted->getTuplePointer(indexDest);

      destPtr[0] = argbPtr[2];
      destPtr[1] = argbPtr[1];
      destPtr[2] = argbPtr[0];
      destPtr[3] = argbPtr[3];
    }
  }
  return converted;
}

// -----------------------------------------------------------------------------
void drawInformationBlock(canvas_ity::canvas& context, const PoleFigureConfiguration_t& config, const std::pair<float32, float32>& position, float margins, float fontPtSize, int32_t phaseNum,
                          std::vector<unsigned char>& fontData, const std::string& laueGroupName, const std::string& materialName)
{
  const float scaleBarRelativeWidth = 0.10f;
  //
  const int imageHeight = config.imageDim;
  const int imageWidth = config.imageDim;
  const float colorHeight = (static_cast<float>(imageHeight)) / static_cast<float>(config.numColors);
  //
  using RectFType = std::pair<float, float>;
  const RectFType rect = std::make_pair(imageWidth * scaleBarRelativeWidth, colorHeight * 1.00000f);
  //
  const std::array<canvas_ity::baseline_style, 6> baselines = {canvas_ity::alphabetic, canvas_ity::top, canvas_ity::middle, canvas_ity::bottom, canvas_ity::hanging, canvas_ity::ideographic};

  // Draw the information about the pole figure
  // clang-format off
  const std::vector<std::string> labels = {
        fmt::format("Phase Num: {}", phaseNum),
        fmt::format("Material Name: {}", materialName),
        fmt::format("Laue Group: {}", laueGroupName),
        fmt::format("Upper & Lower:"),
        fmt::format("Samples: {}", config.eulers->getNumberOfTuples()),
        fmt::format("Lambert Sq. Dim: {}", config.lambertDim)
  };

  // clang-format on
  float heightInc = 1.0f;
  for(const auto& label : labels)
  {
    // Draw the Number of Samples
    context.begin_path();
    context.set_font(fontData.data(), static_cast<int>(fontData.size()), fontPtSize);
    context.set_color(canvas_ity::fill_style, 0.0f, 0.0f, 0.0f, 1.0f);
    context.text_baseline = baselines[0];
    context.fill_text(label.c_str(), position.first + margins + rect.first + margins, position.second + margins + (imageHeight / 3.0) + (heightInc * fontPtSize));
    context.close_path();
    heightInc++;
  }
}

// -----------------------------------------------------------------------------
void drawScalarBar(canvas_ity::canvas& context, const PoleFigureConfiguration_t& config, const std::pair<float32, float32>& position, float margins, float fontPtSize, int32_t phaseNum,
                   std::vector<unsigned char>& fontData, const std::string& laueGroupName, const std::string& materialName)
{

  int numColors = config.numColors;

  // Get all the colors that we will need
  std::vector<nx::core::Rgba> colorTable(numColors);
  std::vector<float> colors(3 * numColors, 0.0);
  nx::core::RgbColor::GetColorTable(numColors, colors); // Generate the color table values
  float r = 0.0;
  float g = 0.0;
  float b = 0.0;
  for(int i = 0; i < numColors; i++) // Convert them to QRgbColor values
  {
    r = colors[3 * i];
    g = colors[3 * i + 1];
    b = colors[3 * i + 2];
    colorTable[i] = RgbColor::dRgb(r * 255, g * 255, b * 255, 255);
  }

  // Now start from the bottom and draw colored lines up the scale bar
  // A Slight Indentation for the scalar bar
  const float scaleBarRelativeWidth = 0.10f;

  const int imageHeight = config.imageDim;
  const int imageWidth = config.imageDim;
  const float colorHeight = (static_cast<float>(imageHeight)) / static_cast<float>(numColors);

  using RectFType = std::pair<float, float>;

  const RectFType rect = std::make_pair(imageWidth * scaleBarRelativeWidth, colorHeight * 1.00000f);

  const std::array<canvas_ity::baseline_style, 6> baselines = {canvas_ity::alphabetic, canvas_ity::top, canvas_ity::middle, canvas_ity::bottom, canvas_ity::hanging, canvas_ity::ideographic};

  // Draw the Max Value
  context.begin_path();
  const std::string maxStr = fmt::format("{:#.6}", config.maxScale);
  context.set_font(fontData.data(), static_cast<int>(fontData.size()), fontPtSize);
  context.set_color(canvas_ity::fill_style, 0.0f, 0.0f, 0.0f, 1.0f);
  context.text_baseline = baselines[0];
  context.fill_text(maxStr.c_str(), position.first + 2.0F * margins + rect.first, position.second + (2 * margins) + (2 * fontPtSize) + colorHeight);
  context.close_path();

  // Draw the Min value
  context.begin_path();
  const std::string minStr = fmt::format("{:#.6}", config.minScale);
  context.set_font(fontData.data(), static_cast<int>(fontData.size()), fontPtSize);
  context.set_color(canvas_ity::fill_style, 0.0f, 0.0f, 0.0f, 1.0f);
  context.text_baseline = baselines[0];
  context.fill_text(minStr.c_str(), position.first + 2.0F * margins + rect.first, position.second + (2 * margins) + (2 * fontPtSize) + ((numColors)*colorHeight));
  context.close_path();

  // Draw the color bar
  for(int i = 0; i < numColors; i++)
  {
    const nx::core::Rgb c = colorTable[numColors - i - 1];
    std::tie(r, g, b) = RgbColor::fRgb(c);

    const float32 x = position.first + margins;
    const float32 y = position.second + (2 * margins) + (2 * fontPtSize) + (i * colorHeight);

    context.begin_path();
    context.set_color(canvas_ity::fill_style, r, g, b, 1.0f);
    context.fill_rectangle(x, y, rect.first, rect.second);

    context.set_color(canvas_ity::stroke_style, r, g, b, 1.0f);
    context.set_line_width(1.0f);
    context.stroke_rectangle(x, y, rect.first, rect.second);
  }

  // Draw the information about the pole figure
  drawInformationBlock(context, config, position, margins, fontPtSize, phaseNum, fontData, laueGroupName, materialName);
} // namespace

} // namespace

// -----------------------------------------------------------------------------
WritePoleFigure::WritePoleFigure(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WritePoleFigureInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
  // Initialize our fonts
  fonts::Base64Decode(fonts::k_FiraSansRegularBase64, m_FiraSansRegular);
  fonts::Base64Decode(fonts::k_LatoRegularBase64, m_LatoRegular);
  fonts::Base64Decode(fonts::k_LatoBoldBase64, m_LatoBold);
}

// -----------------------------------------------------------------------------
WritePoleFigure::~WritePoleFigure() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& WritePoleFigure::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> WritePoleFigure::operator()()
{
  // Make sure any directory path is also available as the user may have just typed
  // in a path without actually creating the full path
  // Ensure the complete path to the output file exists or can be created
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

  const std::vector<LaueOps::Pointer> orientationOps = LaueOps::GetAllOrientationOps();

  const nx::core::Float32Array& eulers = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CellEulerAnglesArrayPath);
  nx::core::Int32Array& phases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellPhasesArrayPath);

  nx::core::UInt32Array& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);
  nx::core::StringArray& materialNames = m_DataStructure.getDataRefAs<StringArray>(m_InputValues->MaterialNameArrayPath);

  std::unique_ptr<MaskCompare> maskCompare = nullptr;
  if(m_InputValues->UseMask)
  {
    try
    {
      maskCompare = InstantiateMaskCompare(m_DataStructure, m_InputValues->MaskArrayPath);
    } catch(const std::out_of_range& exception)
    {
      // This really should NOT be happening as the path was verified during preflight BUT we may be calling this from
      // some other context that is NOT going through the normal nx::core::IFilter API of Preflight and Execute
      return MakeErrorResult(-53900, fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->MaskArrayPath.toString()));
    }
  }

  // Find the total number of angles we have based on the number of Tuples of the
  // Euler Angles array
  const size_t numPoints = eulers.getNumberOfTuples();
  // Find how many phases we have by getting the number of Crystal Structures
  const size_t numPhases = crystalStructures.getNumberOfTuples();

  // Loop over all the voxels gathering the Eulers for a specific phase into an array
  std::vector<usize> tupleShape = {1, static_cast<usize>(m_InputValues->ImageSize), static_cast<usize>(m_InputValues->ImageSize)};
  auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->OutputImageGeometryPath);
  auto cellAttrMatPath = imageGeom.getCellDataPath();
  imageGeom.setDimensions({static_cast<usize>(m_InputValues->ImageSize), static_cast<usize>(m_InputValues->ImageSize), 1});
  imageGeom.getCellData()->resizeTuples(tupleShape);
  for(size_t phase = 1; phase < numPhases; ++phase)
  {
    auto imageArrayPath = cellAttrMatPath.createChildPath(fmt::format("{}Phase_{}", m_InputValues->ImagePrefix, phase));
    auto arrayCreationResult = nx::core::CreateArray<uint8>(m_DataStructure, tupleShape, {4ULL}, imageArrayPath, IDataAction::Mode::Execute);
    if(arrayCreationResult.invalid())
    {
      return arrayCreationResult;
    }
  }

  // Loop over all the voxels gathering the Eulers for a specific phase into an array
  for(size_t phase = 1; phase < numPhases; ++phase)
  {
    size_t count = 0;
    // First find out how many voxels we are going to have. This is probably faster to loop twice than to
    // keep allocating memory everytime we find one.
    for(size_t i = 0; i < numPoints; ++i)
    {
      if(phases[i] == phase)
      {
        if(!m_InputValues->UseMask || maskCompare->isTrue(i))
        {
          count++;
        }
      }
    }
    const std::vector<size_t> eulerCompDim = {3};
    const EbsdLib::FloatArrayType::Pointer subEulersPtr = EbsdLib::FloatArrayType::CreateArray(count, eulerCompDim, "Eulers_Per_Phase", true);
    subEulersPtr->initializeWithValue(std::numeric_limits<float>::signaling_NaN());
    EbsdLib::FloatArrayType& subEulers = *subEulersPtr;

    // Now loop through the eulers again and this time add them to the sub-Eulers Array
    count = 0;
    for(size_t i = 0; i < numPoints; ++i)
    {
      if(phases[i] == phase)
      {
        if(!m_InputValues->UseMask || maskCompare->isTrue(i))
        {
          subEulers[count * 3] = eulers[i * 3];
          subEulers[count * 3 + 1] = eulers[i * 3 + 1];
          subEulers[count * 3 + 2] = eulers[i * 3 + 2];
          count++;
        }
      }
    }
    if(subEulersPtr->getNumberOfTuples() == 0)
    {
      continue;
    } // Skip because we have no Pole Figure data

    std::vector<EbsdLib::UInt8ArrayType::Pointer> figures;

    PoleFigureConfiguration_t config;
    config.eulers = subEulersPtr.get();
    config.imageDim = m_InputValues->ImageSize;
    config.lambertDim = m_InputValues->LambertSize;
    config.numColors = m_InputValues->NumColors;
    config.discrete = (static_cast<WritePoleFigure::Algorithm>(m_InputValues->GenerationAlgorithm) == WritePoleFigure::Algorithm::Discrete);
    config.discreteHeatMap = k_UseDiscreteHeatMap;

    m_MessageHandler({IFilter::Message::Type::Info, fmt::format("Generating Pole Figures for Phase {}", phase)});

    switch(crystalStructures[phase])
    {
    case EbsdLib::CrystalStructure::Cubic_High:
      figures = makePoleFigures<CubicOps>(config);
      break;
    case EbsdLib::CrystalStructure::Cubic_Low:
      figures = makePoleFigures<CubicLowOps>(config);
      break;
    case EbsdLib::CrystalStructure::Hexagonal_High:
      figures = makePoleFigures<HexagonalOps>(config);
      break;
    case EbsdLib::CrystalStructure::Hexagonal_Low:
      figures = makePoleFigures<HexagonalLowOps>(config);
      break;
    case EbsdLib::CrystalStructure::Trigonal_High:
      figures = makePoleFigures<TrigonalOps>(config);
      //   setWarningCondition(-1010, "Trigonal High Symmetry is not supported for Pole figures. This phase will be omitted from results");
      break;
    case EbsdLib::CrystalStructure::Trigonal_Low:
      figures = makePoleFigures<TrigonalLowOps>(config);
      //  setWarningCondition(-1010, "Trigonal Low Symmetry is not supported for Pole figures. This phase will be omitted from results");
      break;
    case EbsdLib::CrystalStructure::Tetragonal_High:
      figures = makePoleFigures<TetragonalOps>(config);
      //  setWarningCondition(-1010, "Tetragonal High Symmetry is not supported for Pole figures. This phase will be omitted from results");
      break;
    case EbsdLib::CrystalStructure::Tetragonal_Low:
      figures = makePoleFigures<TetragonalLowOps>(config);
      // setWarningCondition(-1010, "Tetragonal Low Symmetry is not supported for Pole figures. This phase will be omitted from results");
      break;
    case EbsdLib::CrystalStructure::OrthoRhombic:
      figures = makePoleFigures<OrthoRhombicOps>(config);
      break;
    case EbsdLib::CrystalStructure::Monoclinic:
      figures = makePoleFigures<MonoclinicOps>(config);
      break;
    case EbsdLib::CrystalStructure::Triclinic:
      figures = makePoleFigures<TriclinicOps>(config);
      break;
    default:
      break;
    }

    if(figures.size() == 3)
    {
      const uint32 imageWidth = static_cast<uint32>(config.imageDim);
      const uint32 imageHeight = static_cast<uint32>(config.imageDim);
      const float32 fontPtSize = imageHeight / 16.0f;
      const float32 margins = imageHeight / 32.0f;

      int32 pageWidth = 0;
      int32 pageHeight = margins + fontPtSize;

      float32 xCharWidth = 0.0f;
      {
        canvas_ity::canvas tempContext(m_InputValues->ImageSize, m_InputValues->ImageSize);
        const std::array<char, 2> buf = {'X', 0};
        tempContext.set_font(m_LatoBold.data(), static_cast<int>(m_LatoBold.size()), fontPtSize);
        xCharWidth = tempContext.measure_text(buf.data());
      }
      // Each Pole Figure gets its own Square mini canvas to draw into.
      const float32 subCanvasWidth = margins + imageWidth + xCharWidth + margins;
      const float32 subCanvasHeight = margins + fontPtSize + imageHeight + fontPtSize * 2 + margins * 2;

      std::vector<std::pair<float32, float32>> globalImageOrigins(4);
      if(static_cast<WritePoleFigure::LayoutType>(m_InputValues->ImageLayout) == WritePoleFigure::LayoutType::Horizontal)
      {
        pageWidth = subCanvasWidth * 4;
        pageHeight = pageHeight + subCanvasHeight;
        globalImageOrigins[0] = std::make_pair(0.0f, static_cast<float>(pageHeight) - subCanvasHeight);
        globalImageOrigins[1] = std::make_pair(subCanvasWidth, static_cast<float>(pageHeight) - subCanvasHeight);
        globalImageOrigins[2] = std::make_pair(subCanvasWidth * 2.0f, static_cast<float>(pageHeight) - subCanvasHeight);
        globalImageOrigins[3] = std::make_pair(subCanvasWidth * 3.0f, static_cast<float>(pageHeight) - subCanvasHeight);
      }
      else if(static_cast<WritePoleFigure::LayoutType>(m_InputValues->ImageLayout) == WritePoleFigure::LayoutType::Vertical)
      {
        pageWidth = subCanvasWidth;
        pageHeight = pageHeight + subCanvasHeight * 4.0f;
        globalImageOrigins[0] = std::make_pair(0.0f, margins + fontPtSize);
        globalImageOrigins[1] = std::make_pair(0.0f, margins + fontPtSize + subCanvasHeight * 1.0f);
        globalImageOrigins[2] = std::make_pair(0.0f, margins + fontPtSize + subCanvasHeight * 2.0f);
        globalImageOrigins[3] = std::make_pair(0.0f, margins + fontPtSize + subCanvasHeight * 3.0f);
      }
      else if(static_cast<WritePoleFigure::LayoutType>(m_InputValues->ImageLayout) == nx::core::WritePoleFigure::LayoutType::Square)
      {
        pageWidth = subCanvasWidth * 2.0f;
        pageHeight = pageHeight + subCanvasHeight * 2.0f;
        globalImageOrigins[0] = std::make_pair(0.0f, (static_cast<float>(pageHeight) - 2.0f * subCanvasHeight));           // Upper Left
        globalImageOrigins[1] = std::make_pair(subCanvasWidth, (static_cast<float>(pageHeight) - 2.0f * subCanvasHeight)); // Upper Right
        globalImageOrigins[2] = std::make_pair(0.0f, (static_cast<float>(pageHeight) - subCanvasHeight));                  // Lower Left
        globalImageOrigins[3] = std::make_pair(subCanvasWidth, (static_cast<float>(pageHeight) - subCanvasHeight));        // Lower Right
      }

      // Create a Canvas to draw into
      canvas_ity::canvas context(pageWidth, pageHeight);

      context.set_font(m_LatoBold.data(), static_cast<int>(m_LatoBold.size()), fontPtSize);
      context.set_color(canvas_ity::fill_style, 0.0f, 0.0f, 0.0f, 1.0f);
      canvas_ity::baseline_style const baselines[] = {canvas_ity::alphabetic, canvas_ity::top, canvas_ity::middle, canvas_ity::bottom, canvas_ity::hanging, canvas_ity::ideographic};
      context.text_baseline = baselines[0];

      // Fill the whole background with white
      context.move_to(0.0f, 0.0f);
      context.line_to(static_cast<float>(pageWidth), 0.0f);
      context.line_to(static_cast<float>(pageWidth), static_cast<float>(pageHeight));
      context.line_to(0.0f, static_cast<float>(pageHeight));
      context.line_to(0.0f, 0.0f);
      context.close_path();
      context.set_color(canvas_ity::fill_style, 1.0f, 1.0f, 1.0f, 1.0f);
      context.fill();

      for(int imageIndex = 0; imageIndex < figures.size(); imageIndex++)
      {
        figures[imageIndex] = flipAndMirrorPoleFigure(figures[imageIndex].get(), config);
      }

      for(int i = 0; i < 3; i++)
      {
        std::array<float, 2> figureOrigin = {0.0f, 0.0f};
        std::tie(figureOrigin[0], figureOrigin[1]) = globalImageOrigins[i];
        context.draw_image(figures[i]->getPointer(0), imageWidth, imageHeight, imageWidth * 4, figureOrigin[0] + margins, figureOrigin[1] + fontPtSize * 2 + margins * 2, imageWidth, imageHeight);

        // Draw an outline on the figure
        context.begin_path();
        context.line_cap = canvas_ity::circle;
        context.set_line_width(3.0f);
        context.set_color(canvas_ity::stroke_style, 0.0f, 0.0f, 0.0f, 1.0f);
        context.arc(figureOrigin[0] + margins + m_InputValues->ImageSize / 2.0f, figureOrigin[1] + fontPtSize * 2 + margins * 2 + m_InputValues->ImageSize / 2.0f, m_InputValues->ImageSize / 2.0f, 0,
                    nx::core::Constants::k_2Pi<float>);
        context.stroke();
        context.close_path();

        // Draw the X Axis lines
        context.begin_path();
        context.line_cap = canvas_ity::square;
        context.set_line_width(2.0f);
        context.set_color(canvas_ity::stroke_style, 0.0f, 0.0f, 0.0f, 1.0f);
        context.move_to(figureOrigin[0] + margins, figureOrigin[1] + fontPtSize * 2 + margins * 2 + m_InputValues->ImageSize / 2.0f);
        context.line_to(figureOrigin[0] + margins + m_InputValues->ImageSize, figureOrigin[1] + fontPtSize * 2 + margins * 2 + m_InputValues->ImageSize / 2.0f);
        context.stroke();
        context.close_path();

        // Draw the Y Axis lines
        context.begin_path();
        context.line_cap = canvas_ity::square;
        context.set_line_width(2.0f);
        context.set_color(canvas_ity::stroke_style, 0.0f, 0.0f, 0.0f, 1.0f);
        context.move_to(figureOrigin[0] + margins + m_InputValues->ImageSize / 2.0f, figureOrigin[1] + fontPtSize * 2 + margins * 2);
        context.line_to(figureOrigin[0] + margins + m_InputValues->ImageSize / 2.0f, figureOrigin[1] + fontPtSize * 2 + margins * 2 + m_InputValues->ImageSize);
        context.stroke();
        context.close_path();

        // Draw X Axis Label
        context.begin_path();
        context.set_font(m_LatoBold.data(), static_cast<int>(m_LatoBold.size()), fontPtSize);
        context.set_color(canvas_ity::fill_style, 0.0f, 0.0f, 0.0f, 1.0f);
        context.text_baseline = baselines[0];
        context.fill_text("X", figureOrigin[0] + margins * 2.0f + m_InputValues->ImageSize, figureOrigin[1] + fontPtSize * 2.25 + margins * 2 + m_InputValues->ImageSize / 2.0f);
        context.close_path();

        // Draw Y Axis Label
        context.begin_path();
        context.set_font(m_LatoBold.data(), static_cast<int>(m_LatoBold.size()), fontPtSize);
        context.set_color(canvas_ity::fill_style, 0.0f, 0.0f, 0.0f, 1.0f);
        context.text_baseline = baselines[0];
        const float yFontWidth = context.measure_text("Y");
        context.fill_text("Y", figureOrigin[0] + margins - (0.5f * yFontWidth) + m_InputValues->ImageSize / 2.0f, figureOrigin[1] + fontPtSize * 2 + margins);
        context.close_path();

        // Draw the figure subtitle. This is usually the direction or plane family
        std::string figureSubtitle = figures[i]->getName();
        figureSubtitle = nx::core::StringUtilities::replace(figureSubtitle, "<", "(");
        figureSubtitle = nx::core::StringUtilities::replace(figureSubtitle, ">", ")");
        std::string bottomPart;
        std::array<float, 2> textOrigin = {figureOrigin[0] + margins, figureOrigin[1] + fontPtSize + 2 * margins};
        for(size_t idx = 0; idx < figureSubtitle.size(); idx++)
        {
          if(figureSubtitle.at(idx) == '-')
          {
            const char charBuf[] = {figureSubtitle[idx + 1], 0};
            context.set_font(m_FiraSansRegular.data(), static_cast<int>(m_FiraSansRegular.size()), fontPtSize);
            float tw = 0.0f;
            if(!bottomPart.empty())
            {
              tw = context.measure_text(bottomPart.c_str());
            }
            const float charWidth = context.measure_text(charBuf);
            const float dashWidth = charWidth * 0.5f;
            const float dashOffset = charWidth * 0.25f;

            context.begin_path();
            context.line_cap = canvas_ity::square;
            context.set_line_width(2.0f);
            context.set_color(canvas_ity::stroke_style, 0.0f, 0.0f, 0.0f, 1.0f);
            context.move_to(textOrigin[0] + tw + dashOffset, textOrigin[1] - (0.8f * fontPtSize));
            context.line_to(textOrigin[0] + tw + dashOffset + dashWidth, textOrigin[1] - (0.8f * fontPtSize));
            context.stroke();
            context.close_path();
          }
          else
          {
            bottomPart.push_back(figureSubtitle.at(idx));
          }
        }

        // Draw the Direction subtitle text
        context.begin_path();
        context.set_font(m_FiraSansRegular.data(), static_cast<int>(m_FiraSansRegular.size()), fontPtSize);
        context.set_color(canvas_ity::fill_style, 0.0f, 0.0f, 0.0f, 1.0f);
        context.text_baseline = baselines[0];
        context.fill_text(bottomPart.c_str(), textOrigin[0], textOrigin[1]);
        context.close_path();
      }

      // Draw the title onto the canvas
      context.set_font(m_LatoBold.data(), static_cast<int>(m_LatoBold.size()), fontPtSize);
      context.set_color(canvas_ity::fill_style, 0.0f, 0.0f, 0.0f, 1.0f);
      context.text_baseline = baselines[0];
      context.fill_text(m_InputValues->Title.c_str(), margins, margins + fontPtSize);

      std::vector<std::string> laueNames = LaueOps::GetLaueNames();
      const uint32_t laueIndex = crystalStructures[phase];
      const std::string materialName = materialNames[phase];

      // Now draw the Color Scalar Bar if needed.
      if(config.discrete)
      {
        drawInformationBlock(context, config, globalImageOrigins[3], margins, imageHeight / 20.0f, static_cast<int32_t>(phase), m_LatoRegular, laueNames[laueIndex], materialName);
      }
      else
      {
        drawScalarBar(context, config, globalImageOrigins[3], margins, imageHeight / 20.0f, static_cast<int32_t>(phase), m_LatoRegular, laueNames[laueIndex], materialName);
      }

      // Fetch the rendered RGBA pixels from the entire canvas.
      std::vector<unsigned char> image(static_cast<size_t>(pageHeight * pageWidth * 4));
      context.get_image_data(image.data(), pageWidth, pageHeight, pageWidth * 4, 0, 0);
      if(m_InputValues->SaveAsImageGeometry)
      {
        imageGeom.setDimensions({static_cast<usize>(pageWidth), static_cast<usize>(pageHeight), 1});
        imageGeom.getCellData()->resizeTuples({1, static_cast<usize>(pageHeight), static_cast<usize>(pageWidth)});

        auto imageArrayPath = cellAttrMatPath.createChildPath(fmt::format("{}Phase_{}", m_InputValues->ImagePrefix, phase));
        auto& imageData = m_DataStructure.getDataRefAs<UInt8Array>(imageArrayPath);
        std::copy(image.begin(), image.end(), imageData.begin());
      }

      if(m_InputValues->WriteImageToDisk)
      {
        const std::string filename = fmt::format("{}/{}Phase_{}.tiff", m_InputValues->OutputPath.string(), m_InputValues->ImagePrefix, phase);
        auto result = TiffWriter::WriteImage(filename, pageWidth, pageHeight, 4, image.data());
        if(result.first < 0)
        {
          return MakeErrorResult(-53900, fmt::format("Error writing pole figure image '{}' to disk.\n    Error Code from Tiff Writer: {}\n    Message: {}", filename, result.first, result.second));
        }
      }
    }
  }
  return {};
}
