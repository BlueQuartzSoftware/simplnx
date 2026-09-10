#include "WriteOnScaleTableFile.hpp"

#include "SimplnxCore/Filters/Algorithms/RotateSampleRefFrame.hpp"
#include "SimplnxCore/Filters/RotateSampleRefFrameFilter.hpp"

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/RectGridGeom.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <Eigen/Dense>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <numbers>
#include <numeric>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace fs = std::filesystem;
using namespace nx::core;

namespace
{
const DataPath k_ScratchGeometryPath({"Image Geometry"});
const DataPath k_ScratchCellDataPath = k_ScratchGeometryPath.createChildPath("Cell Data");

enum class Axis : uint8
{
  X,
  Y,
  Z
};

Eigen::Matrix3f CreateRotationMatrix(float32 angle, Axis axis)
{
  const float32 cosAngle = std::cos(angle);
  const float32 sinAngle = std::sin(angle);
  Eigen::Matrix3f matrix = Eigen::Matrix3f::Identity();

  switch(axis)
  {
  case Axis::X:
    matrix << Eigen::Vector3f{1.0F, 0.0F, 0.0F}, Eigen::Vector3f{0.0F, cosAngle, sinAngle}, Eigen::Vector3f{0.0F, -sinAngle, cosAngle};
    break;
  case Axis::Y:
    matrix << Eigen::Vector3f{cosAngle, 0.0F, -sinAngle}, Eigen::Vector3f{0.0F, 1.0F, 0.0F}, Eigen::Vector3f{sinAngle, 0.0F, cosAngle};
    break;
  case Axis::Z:
    matrix << Eigen::Vector3f{cosAngle, sinAngle, 0.0F}, Eigen::Vector3f{-sinAngle, cosAngle, 0.0F}, Eigen::Vector3f{0.0F, 0.0F, 1.0F};
    break;
  }

  return matrix;
}

Eigen::Matrix3f DetermineRotationMatrix(const SizeVec3& dimensions)
{
  // The fixed swap order and left multiplication reproduce the DREAM.3D 6.6 axis-reorder matrix.
  std::vector<Eigen::Matrix3f> rotationMatrices;
  SizeVec3 reorderedDimensions = dimensions;
  SizeVec3 sortedDimensions = dimensions;
  std::sort(sortedDimensions.begin(), sortedDimensions.end(), std::greater<usize>());

  if(reorderedDimensions[0] != sortedDimensions[0])
  {
    if(reorderedDimensions[1] == sortedDimensions[0])
    {
      std::swap(reorderedDimensions[0], reorderedDimensions[1]);
      rotationMatrices.push_back(CreateRotationMatrix(std::numbers::pi_v<float32> / 2.0F, Axis::Z));
    }
    else
    {
      std::swap(reorderedDimensions[0], reorderedDimensions[2]);
      rotationMatrices.push_back(CreateRotationMatrix(std::numbers::pi_v<float32> / 2.0F, Axis::Y));
    }
  }

  if(reorderedDimensions[1] != sortedDimensions[1])
  {
    std::swap(reorderedDimensions[1], reorderedDimensions[2]);
    rotationMatrices.push_back(CreateRotationMatrix(std::numbers::pi_v<float32> / 2.0F, Axis::X));
  }

  const Eigen::Matrix3f identity = Eigen::Matrix3f::Identity();
  return std::accumulate(rotationMatrices.cbegin(), rotationMatrices.cend(), identity, [](const Eigen::Matrix3f& accumulated, const Eigen::Matrix3f& next) { return next * accumulated; });
}

DynamicTableParameter::ValueType ConvertRotationMatrixToTable(const Eigen::Matrix3f& matrix)
{
  DynamicTableParameter::ValueType table(4, std::vector<float64>(4, 0.0));
  for(usize row = 0; row < 3; row++)
  {
    for(usize col = 0; col < 3; col++)
    {
      table[row][col] = matrix(row, col);
    }
  }
  table[3][3] = 1.0;
  return table;
}

template <class Generator>
void WriteEntries(std::ofstream& output, usize count, usize maxEntriesPerLine, Generator&& generator)
{
  // The legacy format writes each separator before its entry and leaves matrix data without a trailing newline.
  usize entriesPerLine = 0;
  for(usize index = 0; index < count; index++)
  {
    if(entriesPerLine != 0)
    {
      if(entriesPerLine % maxEntriesPerLine == 0)
      {
        output << '\n';
        entriesPerLine = 0;
      }
      else
      {
        output << ' ';
      }
    }
    output << generator(index);
    entriesPerLine++;
  }
}

void WriteImageCoordinates(std::ofstream& output, std::string_view label, usize nodeCount, float32 origin, float32 spacing)
{
  output << label << ' ' << nodeCount << '\n';
  WriteEntries(output, nodeCount, 6, [origin, spacing](usize index) { return fmt::format("{:.8E}", origin + (static_cast<float32>(index) * spacing)); });
  output << '\n';
}

void WriteRectGridCoordinates(std::ofstream& output, std::string_view label, const Float32Array& bounds)
{
  output << label << ' ' << bounds.getNumberOfTuples() << '\n';
  WriteEntries(output, bounds.getNumberOfTuples(), 6, [&bounds](usize index) { return fmt::format("{:.8E}", bounds[index]); });
  output << '\n';
}

struct FindMaxGrainId
{
  template <class T>
  usize operator()(const IDataArray& featureIds) const
  {
    const auto& typedFeatureIds = dynamic_cast<const DataArray<T>&>(featureIds);
    usize maxGrainId = 0;
    for(const T value : typedFeatureIds)
    {
      if constexpr(std::is_signed_v<T>)
      {
        if(value > 0)
        {
          maxGrainId = std::max(maxGrainId, static_cast<usize>(value));
        }
      }
      else
      {
        maxGrainId = std::max(maxGrainId, static_cast<usize>(value));
      }
    }
    return maxGrainId;
  }
};

struct CopyFeatureIds
{
  template <class T>
  bool operator()(const IDataArray& sourceArray, DataStructure& destinationDataStructure, const DataObject::IdType& destinationParentId) const
  {
    const auto& typedSource = dynamic_cast<const DataArray<T>&>(sourceArray);
    auto destinationStore = DataStoreUtilities::CreateDataStore<T>(typedSource.getTupleShape(), typedSource.getComponentShape(), IDataAction::Mode::Execute);
    auto* destinationArray = DataArray<T>::Create(destinationDataStructure, sourceArray.getName(), destinationStore, destinationParentId);
    if(destinationArray == nullptr)
    {
      return false;
    }
    std::copy(typedSource.cbegin(), typedSource.cend(), destinationArray->begin());
    return true;
  }
};

struct WriteFeatureIds
{
  template <class T>
  void operator()(std::ofstream& output, const IDataArray& featureIds, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler) const
  {
    const auto& typedFeatureIds = dynamic_cast<const DataArray<T>&>(featureIds);
    const usize tupleCount = typedFeatureIds.getNumberOfTuples();
    const usize progressInterval = std::max<usize>(1, tupleCount / 100);
    usize entriesPerLine = 0;
    for(usize index = 0; index < tupleCount; index++)
    {
      if(index % progressInterval == 0)
      {
        if(shouldCancel)
        {
          return;
        }
        messageHandler({IFilter::Message::Type::Info, fmt::format("Writing matrix values: {}%", (index * 100) / tupleCount)});
      }

      if(entriesPerLine != 0)
      {
        if(entriesPerLine % 40 == 0)
        {
          output << '\n';
          entriesPerLine = 0;
        }
        else
        {
          output << ' ';
        }
      }
      output << fmt::format("{}", typedFeatureIds[index]);
      entriesPerLine++;
    }
  }
};
} // namespace

WriteOnScaleTableFile::WriteOnScaleTableFile(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                             WriteOnScaleTableFileInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(messageHandler)
{
}

WriteOnScaleTableFile::~WriteOnScaleTableFile() noexcept = default;

Result<> WriteOnScaleTableFile::operator()()
{
  const auto& inputGeometry = m_DataStructure.getDataRefAs<IGridGeometry>(m_InputValues->InputGeometryPath);
  const auto& inputFeatureIds = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->FeatureIdsArrayPath);
  const auto& phaseNames = m_DataStructure.getDataRefAs<StringArray>(m_InputValues->PhaseNamesArrayPath);

  if(inputFeatureIds.getNumberOfTuples() == 0)
  {
    return MakeErrorResult(-12027, fmt::format("The feature IDs array '{}' has 0 tuples. Select an array with at least one tuple.", m_InputValues->FeatureIdsArrayPath.toString()));
  }

  const IGridGeometry* exportGeometry = &inputGeometry;
  const IDataArray* exportFeatureIds = &inputFeatureIds;
  DataStructure rotatedDataStructure;
  const SizeVec3 inputDimensions = inputGeometry.getDimensions();
  if(!(inputDimensions[0] >= inputDimensions[1] && inputDimensions[1] >= inputDimensions[2]))
  {
    // The sub-filter rotates a private copy so the export does not change the input data structure.
    auto* scratchGeometry = ImageGeom::Create(rotatedDataStructure, k_ScratchGeometryPath.getTargetName());
    if(scratchGeometry == nullptr)
    {
      return MakeErrorResult(-12028, "Failed to create the scratch Image Geometry for the Rotate Sample Reference Frame sub-filter.");
    }
    const auto& inputImageGeometry = dynamic_cast<const ImageGeom&>(inputGeometry);
    scratchGeometry->setDimensions(inputImageGeometry.getDimensions());
    scratchGeometry->setOrigin(inputImageGeometry.getOrigin());
    scratchGeometry->setSpacing(inputImageGeometry.getSpacing());

    auto* scratchCellData = AttributeMatrix::Create(rotatedDataStructure, k_ScratchCellDataPath.getTargetName(), inputFeatureIds.getTupleShape(), scratchGeometry->getId());
    if(scratchCellData == nullptr)
    {
      return MakeErrorResult(-12029, "Failed to create the scratch Cell Data Attribute Matrix for the Rotate Sample Reference Frame sub-filter.");
    }
    scratchGeometry->setCellData(*scratchCellData);
    const bool copied = ExecuteDataFunctionIntType(CopyFeatureIds{}, inputFeatureIds.getDataType(), inputFeatureIds, rotatedDataStructure, scratchCellData->getId());
    if(!copied)
    {
      return MakeErrorResult(-12030, fmt::format("Failed to copy the feature IDs array '{}' for the Rotate Sample Reference Frame sub-filter.", m_InputValues->FeatureIdsArrayPath.toString()));
    }

    RotateSampleRefFrameFilter rotateFilter;
    Arguments rotateArgs = rotateFilter.getDefaultArguments();
    rotateArgs.insertOrAssign(RotateSampleRefFrameFilter::k_RotationRepresentation_Key,
                              std::make_any<ChoicesParameter::ValueType>(to_underlying(RotateSampleRefFrame::RotationRepresentation::RotationMatrix)));
    rotateArgs.insertOrAssign(RotateSampleRefFrameFilter::k_RotationMatrix_Key,
                              std::make_any<DynamicTableParameter::ValueType>(ConvertRotationMatrixToTable(DetermineRotationMatrix(inputDimensions))));
    rotateArgs.insertOrAssign(RotateSampleRefFrameFilter::k_SelectedImageGeometryPath_Key, std::make_any<DataPath>(k_ScratchGeometryPath));
    rotateArgs.insertOrAssign(RotateSampleRefFrameFilter::k_RemoveOriginalGeometry_Key, std::make_any<bool>(true));
    rotateArgs.insertOrAssign(RotateSampleRefFrameFilter::k_RotateSliceBySlice_Key, std::make_any<bool>(false));
    rotateArgs.insertOrAssign(RotateSampleRefFrameFilter::k_KeepInputGeometryOrigin_Key, std::make_any<bool>(false));

    auto rotatePreflight = rotateFilter.preflight(rotatedDataStructure, rotateArgs);
    if(rotatePreflight.outputActions.invalid())
    {
      return MakeErrorResult(-12031, fmt::format("The Rotate Sample Reference Frame sub-filter preflight failed while reordering geometry '{}': {}", m_InputValues->InputGeometryPath.toString(),
                                                 rotatePreflight.outputActions.errors().front().message));
    }
    if(m_ShouldCancel)
    {
      return {};
    }

    auto rotateExecute = rotateFilter.execute(rotatedDataStructure, rotateArgs);
    if(rotateExecute.result.invalid())
    {
      return MakeErrorResult(-12032, fmt::format("The Rotate Sample Reference Frame sub-filter failed while reordering geometry '{}': {}", m_InputValues->InputGeometryPath.toString(),
                                                 rotateExecute.result.errors().front().message));
    }
    if(m_ShouldCancel)
    {
      return {};
    }

    exportGeometry = &rotatedDataStructure.getDataRefAs<ImageGeom>(k_ScratchGeometryPath);
    exportFeatureIds = &rotatedDataStructure.getDataRefAs<IDataArray>(k_ScratchCellDataPath.createChildPath(inputFeatureIds.getName()));
    const SizeVec3 reorderedDimensions = exportGeometry->getDimensions();
    m_MessageHandler({IFilter::Message::Type::Info, fmt::format("Applied an OnScale axis reorder with Rotate Sample Reference Frame. New dimensions: {} x {} x {}.", reorderedDimensions[0],
                                                                reorderedDimensions[1], reorderedDimensions[2])});
  }

  const usize maxGrainId = ExecuteDataFunctionIntType(FindMaxGrainId{}, exportFeatureIds->getDataType(), *exportFeatureIds);
  const fs::path outputPath = m_InputValues->OutputPath / fmt::format("{}.flxtbl", m_InputValues->FilePrefix);
  auto atomicFileResult = AtomicFile::Create(outputPath);
  if(atomicFileResult.invalid())
  {
    return MakeErrorResult(-12033, fmt::format("Failed to create a temporary output file for '{}': {}", outputPath.string(), atomicFileResult.errors().front().message));
  }
  AtomicFile atomicFile = std::move(atomicFileResult.value());
  std::ofstream output(atomicFile.tempFilePath(), std::ios::binary | std::ios::trunc);
  if(!output.is_open())
  {
    return MakeErrorResult(-12034, fmt::format("Failed to open the temporary OnScale table file '{}'. Check write permissions for output path '{}'.", atomicFile.tempFilePath().string(),
                                               m_InputValues->OutputPath.string()));
  }

  if(m_ShouldCancel)
  {
    return {};
  }
  m_MessageHandler({IFilter::Message::Type::Info, "Writing OnScale header..."});
  output << "hedr 0\ninfo 1\n";

  const SizeVec3 dimensions = exportGeometry->getDimensions();
  if(const auto* imageGeometry = dynamic_cast<const ImageGeom*>(exportGeometry); imageGeometry != nullptr)
  {
    const FloatVec3 origin = imageGeometry->getOrigin();
    const FloatVec3 spacing = imageGeometry->getSpacing();
    const std::array<std::string_view, 3> labels = {"xcrd", "ycrd", "zcrd"};
    for(usize axis = 0; axis < labels.size(); axis++)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      m_MessageHandler({IFilter::Message::Type::Info, fmt::format("Writing {} coordinates...", labels[axis])});
      WriteImageCoordinates(output, labels[axis], dimensions[axis] + 1, origin[axis], spacing[axis]);
    }
  }
  else
  {
    const auto& rectGridGeometry = dynamic_cast<const RectGridGeom&>(*exportGeometry);
    const std::array<std::pair<std::string_view, const Float32Array*>, 3> bounds = {
        {{"xcrd", rectGridGeometry.getXBounds()}, {"ycrd", rectGridGeometry.getYBounds()}, {"zcrd", rectGridGeometry.getZBounds()}}};
    for(const auto& [label, boundsArray] : bounds)
    {
      if(m_ShouldCancel)
      {
        return {};
      }
      m_MessageHandler({IFilter::Message::Type::Info, fmt::format("Writing {} coordinates...", label)});
      WriteRectGridCoordinates(output, label, *boundsArray);
    }
  }

  if(m_ShouldCancel)
  {
    return {};
  }
  m_MessageHandler({IFilter::Message::Type::Info, "Writing OnScale keypoints..."});
  output << "keypoints\n" << m_InputValues->NumKeypoints[0] << ' ' << m_InputValues->NumKeypoints[1] << ' ' << m_InputValues->NumKeypoints[2] << '\n';

  if(m_ShouldCancel)
  {
    return {};
  }
  m_MessageHandler({IFilter::Message::Type::Info, "Writing OnScale divisions..."});
  output << "divisions\n" << dimensions[0] << ' ' << dimensions[1] << ' ' << dimensions[2] << '\n';

  if(m_ShouldCancel)
  {
    return {};
  }
  m_MessageHandler({IFilter::Message::Type::Info, "Writing OnScale phase names..."});
  output << "name " << maxGrainId << '\n';
  const usize phaseNameCount = phaseNames.getNumberOfTuples();
  for(usize grainId = 1; grainId <= maxGrainId; grainId++)
  {
    output << (grainId < phaseNameCount ? phaseNames[grainId] : fmt::format("Phase_{}", grainId)) << ' ';
  }
  output << "\nmatr " << dimensions[0] * dimensions[1] * dimensions[2] << '\n';

  if(m_ShouldCancel)
  {
    return {};
  }
  m_MessageHandler({IFilter::Message::Type::Info, "Writing OnScale matrix values..."});
  ExecuteDataFunctionIntType(WriteFeatureIds{}, exportFeatureIds->getDataType(), output, *exportFeatureIds, m_ShouldCancel, m_MessageHandler);
  if(m_ShouldCancel)
  {
    return {};
  }

  output.close();
  if(output.fail())
  {
    return MakeErrorResult(-12035, fmt::format("Failed to write the OnScale table file '{}'. Check available storage and write permissions.", outputPath.string()));
  }

  Result<> commitResult = atomicFile.commit();
  if(commitResult.invalid())
  {
    return MakeErrorResult(-12036, fmt::format("Failed to commit the OnScale table file '{}': {}", outputPath.string(), commitResult.errors().front().message));
  }
  return {};
}
