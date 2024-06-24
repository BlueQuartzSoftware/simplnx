#include "WritePoleFigure.hpp"

#include "OrientationAnalysis/utilities/FiraSansRegular.hpp"
#include "OrientationAnalysis/utilities/Fonts.hpp"
#include "OrientationAnalysis/utilities/IntersectionUtilities.hpp"
#include "OrientationAnalysis/utilities/LatoBold.hpp"
#include "OrientationAnalysis/utilities/LatoRegular.hpp"
#include "OrientationAnalysis/utilities/RTree.hpp"
#include "OrientationAnalysis/utilities/TiffWriter.hpp"
#include "OrientationAnalysis/utilities/delaunator.h"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/Common/RgbColor.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
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
#include "EbsdLib/Utilities/LambertUtilities.h"
#include "EbsdLib/Utilities/ModifiedLambertProjection.h"

#include "H5Support/H5Lite.h"
#include "H5Support/H5ScopedSentinel.h"
#include "H5Support/H5Utilities.h"

#define CANVAS_ITY_IMPLEMENTATION
#include <canvas_ity.hpp>

using namespace nx::core;

namespace
{
const bool k_UseDiscreteHeatMap = false;

class ComputeIntensityStereographicProjection
{
public:
  ComputeIntensityStereographicProjection(EbsdLib::FloatArrayType* xyzCoords, PoleFigureConfiguration_t* config, EbsdLib::DoubleArrayType* intensity, bool normalizeToMRD)
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
      size_t numCoords = m_XYZCoords->getNumberOfTuples();
      float* xyzPtr = m_XYZCoords->getPointer(0);
      for(size_t i = 0; i < numCoords; i++)
      {
        if(xyzPtr[i * 3 + 2] < 0.0f)
        {
          xyzPtr[i * 3 + 0] *= -1.0f;
          xyzPtr[i * 3 + 1] *= -1.0f;
          xyzPtr[i * 3 + 2] *= -1.0f;
        }
        float x = xyzPtr[i * 3] / (1 + xyzPtr[i * 3 + 2]);
        float y = xyzPtr[i * 3 + 1] / (1 + xyzPtr[i * 3 + 2]);

        int xCoord = static_cast<int>(x * static_cast<float32>(halfDim - 1)) + halfDim;
        int yCoord = static_cast<int>(y * static_cast<float32>(halfDim - 1)) + halfDim;

        size_t index = (yCoord * m_Config->imageDim) + xCoord;

        intensity[index]++;
      }
    }
    else
    {
      ModifiedLambertProjection::Pointer lambert = ModifiedLambertProjection::LambertBallToSquare(m_XYZCoords, m_Config->lambertDim, m_Config->sphereRadius);
      if(m_NormalizeToMRD)
      {
        lambert->normalizeSquaresToMRD();
      }
      lambert->createStereographicProjection(m_Config->imageDim, *m_Intensity);

      // This next function (writeLambertData) is experimental but should be left in to
      // make sure it will still compile. At some point I will get back to the development
      // of this alternate pole figure generation.
      // writeLambertData(lambert);
    }
  }

  template <typename V, typename T, typename W>
  void unstructuredGridInterpolator(nx::core::IFilter* filter, nx::core::TriangleGeom* delaunayGeom, std::vector<V>& xPositionsPtr, std::vector<V>& yPositionsPtr, T* xyValues,
                                    typename std::vector<W>& outputValues) const
  {
    using Vec3f = nx::core::Vec3<float>;
    using RTreeType = RTree<size_t, float, 2, float>;

    // filter->notifyStatusMessage(QString("Starting Interpolation...."));
    nx::core::IGeometry::SharedFaceList& delTriangles = delaunayGeom->getFacesRef();
    size_t numTriangles = delaunayGeom->getNumberOfFaces();
    int percent = 0;
    int counter = xPositionsPtr.size() / 100;
    RTreeType m_RTree;
    // Populate the RTree

    size_t numTris = delaunayGeom->getNumberOfFaces();
    for(size_t tIndex = 0; tIndex < numTris; tIndex++)
    {
      std::array<float, 6> boundBox = nx::IntersectionUtilities::GetBoundingBoxAtTri(*delaunayGeom, tIndex);
      m_RTree.Insert(boundBox.data(), boundBox.data() + 3, tIndex); // Note, all values including zero are fine in this version
    }

    for(size_t vertIndex = 0; vertIndex < xPositionsPtr.size(); vertIndex++)
    {
      Vec3f rayOrigin(xPositionsPtr[vertIndex], yPositionsPtr[vertIndex], 1.0F);
      Vec3f rayDirection(0.0F, 0.0F, -1.0F);
      Vec3f barycentricCoord(0.0F, 0.0F, 0.0F);
      //      int xPos = xPositionsPtr[vertIndex];
      //      int yPos = yPositionsPtr[vertIndex];
      if(counter != 0)
      {

        if(vertIndex % counter == 0)
        {
          //          QString ss = QObject::tr("Interpolating || %1% Complete").arg(percent);
          //          filter->notifyStatusMessage(ss);
          percent += 1;
        }
      }

      // Create these reusable variables to save the reallocation each time through the loop

      std::vector<size_t> hitTriangleIds;
      std::function<bool(size_t)> func = [&](size_t id) {
        hitTriangleIds.push_back(id);
        return true; // keep going
      };

      int nhits = m_RTree.Search(rayOrigin.data(), rayOrigin.data(), func);
      for(auto triIndex : hitTriangleIds)
      {
        barycentricCoord = {0.0F, 0.0F, 0.0F};
        std::array<size_t, 3> triVertIndices;
        // Get the Vertex Coordinates for each of the 3 vertices
        std::array<nx::core::Point3Df, 3> verts;
        delaunayGeom->getFaceCoordinates(triIndex, verts);
        Vec3f v0 = verts[0];
        Vec3f v1 = verts[1];
        Vec3f v2 = verts[2];

        // Get the vertex Indices from the triangle
        delaunayGeom->getFacePointIds(triIndex, triVertIndices);
        bool inTriangle = nx::IntersectionUtilities::RayTriangleIntersect2(rayOrigin, rayDirection, v0, v1, v2, barycentricCoord);
        if(inTriangle)
        {
          // Linear Interpolate dx and dy values using the barycentric coordinates
          delaunayGeom->getFaceCoordinates(triIndex, verts);
          float f0 = xyValues[triVertIndices[0]];
          float f1 = xyValues[triVertIndices[1]];
          float f2 = xyValues[triVertIndices[2]];

          float interpolatedVal = (barycentricCoord[0] * f0) + (barycentricCoord[1] * f1) + (barycentricCoord[2] * f2);

          outputValues[vertIndex] = interpolatedVal;

          break;
        }
      }
    }
  }

  int writeLambertData(ModifiedLambertProjection::Pointer lambert) const
  {
    int err = -1;

    int m_Dimension = lambert->getDimension();
    EbsdLib::DoubleArrayType::Pointer m_NorthSquare = lambert->getNorthSquare();
    EbsdLib::DoubleArrayType::Pointer m_SouthSquare = lambert->getSouthSquare();

    // We want half the sphere area for each square because each square represents a hemisphere.
    const float32 sphereRadius = 1.0f;
    float halfSphereArea = 4.0f * EbsdLib::Constants::k_PiF * sphereRadius * sphereRadius / 2.0f;
    // The length of a side of the square is the square root of the area
    float squareEdge = std::sqrt(halfSphereArea);
    float32 m_StepSize = squareEdge / static_cast<float>(m_Dimension);

    float32 m_MaxCoord = squareEdge / 2.0f;
    float32 m_MinCoord = -squareEdge / 2.0f;
    std::array<float, 3> vert = {0.0f, 0.0f, 0.0f};

    std::vector<float> squareCoords(m_Dimension * m_Dimension * 3);

    // Northern Hemisphere Coordinates
    std::vector<float> northSphereCoords(m_Dimension * m_Dimension * 3);
    std::vector<float> northStereoCoords(m_Dimension * m_Dimension * 3);

    // Southern Hemisphere Coordinates
    std::vector<float> southSphereCoords(m_Dimension * m_Dimension * 3);
    std::vector<float> southStereoCoords(m_Dimension * m_Dimension * 3);

    size_t index = 0;

    const float origin = m_MinCoord + (m_StepSize / 2.0f);
    for(int32_t y = 0; y < m_Dimension; ++y)
    {
      for(int x = 0; x < m_Dimension; ++x)
      {
        vert[0] = origin + (static_cast<float>(x) * m_StepSize);
        vert[1] = origin + (static_cast<float>(y) * m_StepSize);

        squareCoords[index * 3] = vert[0];
        squareCoords[index * 3 + 1] = vert[1];
        squareCoords[index * 3 + 2] = 0.0f;

        LambertUtilities::LambertSquareVertToSphereVert(vert.data(), LambertUtilities::Hemisphere::North);

        northSphereCoords[index * 3] = vert[0];
        northSphereCoords[index * 3 + 1] = vert[1];
        northSphereCoords[index * 3 + 2] = vert[2];

        northStereoCoords[index * 3] = vert[0] / (1.0f + vert[2]);
        northStereoCoords[index * 3 + 1] = vert[1] / (1.0f + vert[2]);
        northStereoCoords[index * 3 + 2] = 0.0f;

        // Reset the Lambert Square Coord
        vert[0] = origin + (static_cast<float>(x) * m_StepSize);
        vert[1] = origin + (static_cast<float>(y) * m_StepSize);
        LambertUtilities::LambertSquareVertToSphereVert(vert.data(), LambertUtilities::Hemisphere::South);

        southSphereCoords[index * 3] = vert[0];
        southSphereCoords[index * 3 + 1] = vert[1];
        southSphereCoords[index * 3 + 2] = vert[2];

        southStereoCoords[index * 3] = vert[0] / (1.0f + vert[2]);
        southStereoCoords[index * 3 + 1] = vert[1] / (1.0f + vert[2]);
        southStereoCoords[index * 3 + 2] = 0.0f;

        index++;
      }
    }

    //**********************************************************************************************************************
    // Triangulate the stereo coordinates

    DataStructure dataStructure;

    auto* triangleGeomPtr = TriangleGeom::Create(dataStructure, "Delaunay");
    auto& triangleGeom = *triangleGeomPtr;

    DataPath sharedFaceListPath({"Delaunay", "SharedFaceList"});

    usize numPts = northStereoCoords.size() / 3;
    // Create the default DataArray that will hold the FaceList and Vertices. We
    // size these to 1 because the Csv parser will resize them to the appropriate number of tuples
    using DimensionType = std::vector<size_t>;

    DimensionType faceTupleShape = {0};
    Result result = CreateArray<IGeometry::MeshIndexType>(dataStructure, faceTupleShape, {3ULL}, sharedFaceListPath, IDataAction::Mode::Execute);
    if(result.invalid())
    {
      return -1;
      // return MergeResults(result, MakeErrorResult(-5509, fmt::format("{}CreateGeometry2DAction: Could not allocate SharedTriList '{}'", prefix, trianglesPath.toString())));
    }
    auto& sharedFaceListRef = dataStructure.getDataRefAs<IGeometry::MeshIndexArrayType>(sharedFaceListPath);
    triangleGeom.setFaceList(sharedFaceListRef);

    // Create the Vertex Array with a component size of 3
    DataPath vertexPath({"Delaunay", "SharedVertexList"});

    DimensionType vertexTupleShape = {0};
    result = CreateArray<float>(dataStructure, vertexTupleShape, {3}, vertexPath, IDataAction::Mode::Execute);
    if(result.invalid())
    {
      return -2;
      // return MergeResults(result, MakeErrorResult(-5510, fmt::format("{}CreateGeometry2DAction: Could not allocate SharedVertList '{}'", prefix, vertexPath.toString())));
    }
    Float32Array* vertexArray = ArrayFromPath<float>(dataStructure, vertexPath);
    triangleGeom.setVertices(*vertexArray);
    triangleGeom.resizeVertexList(numPts);

    auto* vertexAttributeMatrix = AttributeMatrix::Create(dataStructure, "VertexData", {numPts}, triangleGeom.getId());
    if(vertexAttributeMatrix == nullptr)
    {
      return -3;
      // return MakeErrorResult(-5512, fmt::format("CreateGeometry2DAction: Failed to create attribute matrix: '{}'", prefix, vertexDataPath.toString()));
    }
    triangleGeom.setVertexAttributeMatrix(*vertexAttributeMatrix);

    if(nullptr != triangleGeom.getVertexAttributeMatrix())
    {
      triangleGeom.getVertexAttributeMatrix()->resizeTuples({numPts});
    }
    auto vertexCoordsPtr = triangleGeom.getVerticesRef();

    // Create Coords for the Delaunator Algorithm
    std::vector<float64> coords(2 * numPts, 0.0);
    for(usize i = 0; i < numPts; i++)
    {
      coords[i * 2] = northStereoCoords[i * 3];
      coords[i * 2 + 1] = northStereoCoords[i * 3 + 1];
      vertexCoordsPtr[i * 3] = northStereoCoords[i * 3];
      vertexCoordsPtr[i * 3 + 1] = northStereoCoords[i * 3 + 1];
      vertexCoordsPtr[i * 3 + 2] = northStereoCoords[i * 3 + 2];
    }

    // Perform the triangulation
    nx::delaunator::Delaunator d(coords);

    usize numTriangles = d.triangles.size();
    triangleGeom.resizeFaceList(numTriangles / 3);
    auto sharedTriListPtr = triangleGeom.getFacesRef();
    // usize triangleIndex = 0;
    for(usize i = 0; i < numTriangles; i += 3)
    {
      std::array<usize, 3> triIDs = {d.triangles[i], d.triangles[i + 1], d.triangles[i + 2]};
      sharedTriListPtr[i] = d.triangles[i];
      sharedTriListPtr[i + 1] = d.triangles[i + 1];
      sharedTriListPtr[i + 2] = d.triangles[i + 2];
    }

    // Create the vertex and face AttributeMatrix
    auto* faceAttributeMatrix = AttributeMatrix::Create(dataStructure, "Face Data", {numTriangles / 3}, triangleGeom.getId());
    if(faceAttributeMatrix == nullptr)
    {
      return -4;
      // return MakeErrorResult(-5511, fmt::format("{}CreateGeometry2DAction: Failed to create attribute matrix: '{}'", prefix, faceDataPath.toString()));
    }
    triangleGeom.setFaceAttributeMatrix(*faceAttributeMatrix);

    Pipeline pipeline;
    const Result<> result2 = DREAM3D::WriteFile(fmt::format("/tmp/delaunay_triangulation.dream3d"), dataStructure, pipeline, true);
    //******************************************************************************************************************************

    //******************************************************************************************************************************
    // Perform a Bi-linear Interpolation
    // Generate a regular grid of XY points
    size_t numSteps = 1024;
    float32 stepInc = 2.0f / static_cast<float>(numSteps);
    std::vector<float32> xcoords(numSteps * numSteps);
    std::vector<float32> ycoords(numSteps * numSteps);
    for(size_t y = 0; y < numSteps; ++y)
    {
      for(size_t x = 0; x < numSteps; x++)
      {
        size_t idx = y * numSteps + x;
        xcoords[idx] = -1.0f + static_cast<float32>(x) * stepInc;
        ycoords[idx] = -1.0f + static_cast<float32>(y) * stepInc;
      }
    }

    std::vector<double> outputValues(numSteps * numSteps);
    unstructuredGridInterpolator<float32, double, double>(nullptr, &triangleGeom, xcoords, ycoords, m_NorthSquare->data(), outputValues);

    //******************************************************************************************************************************

    //******************************************************************************************************************************
    // Write out all the data
    {
      hid_t groupId = H5Support::H5Utilities::createFile("/tmp/lambert_data.h5");
      H5Support::H5ScopedFileSentinel fileSentinel(groupId, false);

      std::vector<hsize_t> dims = {static_cast<hsize_t>(m_Dimension), static_cast<hsize_t>(m_Dimension)};
      err = H5Support::H5Lite::writePointerDataset(groupId, m_NorthSquare->getName(), 2, dims.data(), m_NorthSquare->data());
      err = H5Support::H5Lite::writePointerDataset(groupId, m_SouthSquare->getName(), 2, dims.data(), m_SouthSquare->data());
      dims[0] = m_Dimension * m_Dimension;
      dims[1] = 3ULL;

      err = H5Support::H5Lite::writePointerDataset(groupId, "Lambert Square Coords", 2, dims.data(), squareCoords.data());
      err = H5Support::H5Lite::writePointerDataset(groupId, "North Sphere Coords", 2, dims.data(), northSphereCoords.data());
      err = H5Support::H5Lite::writePointerDataset(groupId, "North Stereo Coords", 2, dims.data(), northStereoCoords.data());

      err = H5Support::H5Lite::writePointerDataset(groupId, "South Sphere Coords", 2, dims.data(), southSphereCoords.data());
      err = H5Support::H5Lite::writePointerDataset(groupId, "South Stereo Coords", 2, dims.data(), southStereoCoords.data());

      dims[0] = numSteps * numSteps;
      err = H5Support::H5Lite::writePointerDataset(groupId, "X Coords", 1, dims.data(), xcoords.data());
      err = H5Support::H5Lite::writePointerDataset(groupId, "Y Coords", 1, dims.data(), ycoords.data());
      err = H5Support::H5Lite::writePointerDataset(groupId, "Interpolated Values", 1, dims.data(), outputValues.data());
    }

    return err;
  }

private:
  EbsdLib::FloatArrayType* m_XYZCoords = nullptr;
  PoleFigureConfiguration_t* m_Config = nullptr;
  EbsdLib::DoubleArrayType* m_Intensity = nullptr;
  bool m_NormalizeToMRD = false;
};

// -----------------------------------------------------------------------------
template <typename Ops>
std::vector<EbsdLib::UInt8ArrayType::Pointer> makePoleFigures(PoleFigureConfiguration_t& config)
{
  Ops ops;
  return ops.generatePoleFigure(config);
}

template <typename OpsType>
std::vector<EbsdLib::DoubleArrayType::Pointer> createIntensityPoleFigures(PoleFigureConfiguration_t& config, bool normalizeToMRD)
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
  taskRunner.execute(ComputeIntensityStereographicProjection(xyz001.get(), &config, intensity001.get(), normalizeToMRD));
  taskRunner.execute(ComputeIntensityStereographicProjection(xyz011.get(), &config, intensity011.get(), normalizeToMRD));
  taskRunner.execute(ComputeIntensityStereographicProjection(xyz111.get(), &config, intensity111.get(), normalizeToMRD));
  taskRunner.wait(); // This will spill over if the number of DataArrays to process does not divide evenly by the number of threads.

  return {intensity001, intensity011, intensity111};
}

// -----------------------------------------------------------------------------
template <typename T>
typename EbsdDataArray<T>::Pointer flipAndMirrorPoleFigure(EbsdDataArray<T>* src, const PoleFigureConfiguration_t& config)
{
  typename EbsdDataArray<T>::Pointer converted = EbsdDataArray<T>::CreateArray(config.imageDim * config.imageDim, src->getComponentDimensions(), src->getName(), true);
  // We need to flip the image "vertically", which means the bottom row becomes
  // the top row and convert from BGRA to RGBA ordering (This is a Little Endian code)
  // If this is ever compiled on a BIG ENDIAN machine the colors will be off.
  for(int y = 0; y < config.imageDim; y++)
  {
    const int destY = config.imageDim - 1 - y;
    for(int x = 0; x < config.imageDim; x++)
    {
      const size_t indexSrc = y * config.imageDim + x;
      const size_t indexDest = destY * config.imageDim + x;

      T* argbPtr = src->getTuplePointer(indexSrc);
      converted->setTuple(indexDest, argbPtr);
    }
  }
  return converted;
}

template <typename T>
typename EbsdDataArray<T>::Pointer convertColorOrder(EbsdDataArray<T>* src, const PoleFigureConfiguration_t& config)
{
  typename EbsdDataArray<T>::Pointer converted = EbsdDataArray<T>::CreateArray(config.imageDim * config.imageDim, src->getComponentDimensions(), src->getName(), true);
  // BGRA to RGBA ordering (This is a Little Endian code)
  // If this is ever compiled on a BIG ENDIAN machine the colors will be off.
  size_t numTuples = src->getNumberOfTuples();
  for(size_t tIdx = 0; tIdx < numTuples; tIdx++)
  {
    T* argbPtr = src->getTuplePointer(tIdx);
    T* destPtr = converted->getTuplePointer(tIdx);
    destPtr[0] = argbPtr[2];
    destPtr[1] = argbPtr[1];
    destPtr[2] = argbPtr[0];
    destPtr[3] = argbPtr[3];
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
  const RectFType rect = std::make_pair(static_cast<float>(imageWidth) * scaleBarRelativeWidth, colorHeight * 1.00000f);
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
    context.fill_text(label.c_str(), position.first + margins + rect.first + margins, position.second + margins + (static_cast<float>(imageHeight) / 3.0f) + (heightInc * fontPtSize));
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
    colorTable[i] = RgbColor::dRgb(static_cast<uint8>(r * 255.0f), static_cast<uint8>(g * 255.0f), static_cast<uint8>(b * 255.0f), 255);
  }

  // Now start from the bottom and draw colored lines up the scale bar
  // A Slight Indentation for the scalar bar
  const float scaleBarRelativeWidth = 0.10f;

  const int imageHeight = config.imageDim;
  const int imageWidth = config.imageDim;
  const float colorHeight = (static_cast<float>(imageHeight)) / static_cast<float>(numColors);

  using RectFType = std::pair<float, float>;

  const RectFType rect = std::make_pair(static_cast<float32>(imageWidth) * scaleBarRelativeWidth, colorHeight * 1.00000f);

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
  context.fill_text(minStr.c_str(), position.first + 2.0F * margins + rect.first, position.second + (2 * margins) + (2 * fontPtSize) + (static_cast<float32>(numColors) * colorHeight));
  context.close_path();

  // Draw the color bar
  for(int i = 0; i < numColors; i++)
  {
    const nx::core::Rgb c = colorTable[numColors - i - 1];
    std::tie(r, g, b) = RgbColor::fRgb(c);

    const float32 x = position.first + margins;
    const float32 y = position.second + (2 * margins) + (2 * fontPtSize) + (static_cast<float32>(i) * colorHeight);

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

  const nx::core::Float32Array& eulerAngles = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->CellEulerAnglesArrayPath);
  auto& phases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellPhasesArrayPath);

  auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);
  auto& materialNames = m_DataStructure.getDataRefAs<StringArray>(m_InputValues->MaterialNameArrayPath);

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
  const size_t numPoints = eulerAngles.getNumberOfTuples();
  // Find how many phases we have by getting the number of Crystal Structures
  const size_t numPhases = crystalStructures.getNumberOfTuples();

  // Create the Image Geometry that will serve as the final storage location for each
  // pole figure. We are just giving it a default size for now, it will be resized
  // further down the algorithm.
  std::vector<usize> tupleShape = {1, static_cast<usize>(m_InputValues->ImageSize), static_cast<usize>(m_InputValues->ImageSize)};
  auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->OutputImageGeometryPath);
  auto cellAttrMatPath = imageGeom.getCellDataPath();
  imageGeom.setDimensions({static_cast<usize>(m_InputValues->ImageSize), static_cast<usize>(m_InputValues->ImageSize), 1});
  imageGeom.getCellData()->resizeTuples(tupleShape);

  // Loop over all the voxels gathering the Euler angles for a specific phase into an array
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
    const EbsdLib::FloatArrayType::Pointer subEulerAnglesPtr = EbsdLib::FloatArrayType::CreateArray(count, eulerCompDim, "Euler_Angles_Per_Phase", true);
    subEulerAnglesPtr->initializeWithValue(std::numeric_limits<float>::signaling_NaN());
    EbsdLib::FloatArrayType& subEulerAngles = *subEulerAnglesPtr;

    // Now loop through the Euler angles again and this time add them to the sub-Euler angle Array
    count = 0;
    for(size_t i = 0; i < numPoints; ++i)
    {
      if(phases[i] == phase)
      {
        if(!m_InputValues->UseMask || maskCompare->isTrue(i))
        {
          subEulerAngles[count * 3] = eulerAngles[i * 3];
          subEulerAngles[count * 3 + 1] = eulerAngles[i * 3 + 1];
          subEulerAngles[count * 3 + 2] = eulerAngles[i * 3 + 2];
          count++;
        }
      }
    }
    if(subEulerAnglesPtr->getNumberOfTuples() == 0)
    {
      continue;
    } // Skip because we have no Pole Figure data

    std::vector<EbsdLib::UInt8ArrayType::Pointer> figures;
    std::vector<EbsdLib::DoubleArrayType::Pointer> intensityImages;

    PoleFigureConfiguration_t config;
    config.eulers = subEulerAnglesPtr.get();
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
      intensityImages = createIntensityPoleFigures<CubicOps>(config, m_InputValues->NormalizeToMRD);
      break;
    case EbsdLib::CrystalStructure::Cubic_Low:
      figures = makePoleFigures<CubicLowOps>(config);
      intensityImages = createIntensityPoleFigures<CubicLowOps>(config, m_InputValues->NormalizeToMRD);
      break;
    case EbsdLib::CrystalStructure::Hexagonal_High:
      figures = makePoleFigures<HexagonalOps>(config);
      intensityImages = createIntensityPoleFigures<HexagonalOps>(config, m_InputValues->NormalizeToMRD);
      break;
    case EbsdLib::CrystalStructure::Hexagonal_Low:
      figures = makePoleFigures<HexagonalLowOps>(config);
      intensityImages = createIntensityPoleFigures<HexagonalLowOps>(config, m_InputValues->NormalizeToMRD);
      break;
    case EbsdLib::CrystalStructure::Trigonal_High:
      figures = makePoleFigures<TrigonalOps>(config);
      intensityImages = createIntensityPoleFigures<TrigonalOps>(config, m_InputValues->NormalizeToMRD);
      //   setWarningCondition(-1010, "Trigonal High Symmetry is not supported for Pole figures. This phase will be omitted from results");
      break;
    case EbsdLib::CrystalStructure::Trigonal_Low:
      figures = makePoleFigures<TrigonalLowOps>(config);
      intensityImages = createIntensityPoleFigures<TrigonalLowOps>(config, m_InputValues->NormalizeToMRD);
      //  setWarningCondition(-1010, "Trigonal Low Symmetry is not supported for Pole figures. This phase will be omitted from results");
      break;
    case EbsdLib::CrystalStructure::Tetragonal_High:
      figures = makePoleFigures<TetragonalOps>(config);
      intensityImages = createIntensityPoleFigures<TetragonalOps>(config, m_InputValues->NormalizeToMRD);
      //  setWarningCondition(-1010, "Tetragonal High Symmetry is not supported for Pole figures. This phase will be omitted from results");
      break;
    case EbsdLib::CrystalStructure::Tetragonal_Low:
      figures = makePoleFigures<TetragonalLowOps>(config);
      intensityImages = createIntensityPoleFigures<TetragonalLowOps>(config, m_InputValues->NormalizeToMRD);
      // setWarningCondition(-1010, "Tetragonal Low Symmetry is not supported for Pole figures. This phase will be omitted from results");
      break;
    case EbsdLib::CrystalStructure::OrthoRhombic:
      figures = makePoleFigures<OrthoRhombicOps>(config);
      intensityImages = createIntensityPoleFigures<OrthoRhombicOps>(config, m_InputValues->NormalizeToMRD);
      break;
    case EbsdLib::CrystalStructure::Monoclinic:
      figures = makePoleFigures<MonoclinicOps>(config);
      intensityImages = createIntensityPoleFigures<MonoclinicOps>(config, m_InputValues->NormalizeToMRD);
      break;
    case EbsdLib::CrystalStructure::Triclinic:
      figures = makePoleFigures<TriclinicOps>(config);
      intensityImages = createIntensityPoleFigures<TriclinicOps>(config, m_InputValues->NormalizeToMRD);
      break;
    default:
      break;
    }

    if(m_InputValues->SaveIntensityData && intensityImages.size() == 3)
    {
      DataPath amPath = m_InputValues->IntensityGeometryDataPath.createChildPath(write_pole_figure::k_ImageAttrMatName);
      // If there is more than a single phase we will need to add more arrays to the DataStructure
      if(phase > 1)
      {
        const std::vector<size_t> intensityImageDims = {static_cast<usize>(config.imageDim), static_cast<usize>(config.imageDim), 1ULL};
        DataPath arrayDataPath = amPath.createChildPath(fmt::format("Phase_{}_{}", phase, m_InputValues->IntensityPlot1Name));
        Result<> result = CreateArray<float64>(m_DataStructure, intensityImageDims, {1ULL}, arrayDataPath, IDataAction::Mode::Execute);

        arrayDataPath = amPath.createChildPath(fmt::format("Phase_{}_{}", phase, m_InputValues->IntensityPlot2Name));
        result = CreateArray<float64>(m_DataStructure, intensityImageDims, {1ULL}, arrayDataPath, IDataAction::Mode::Execute);

        arrayDataPath = amPath.createChildPath(fmt::format("Phase_{}_{}", phase, m_InputValues->IntensityPlot3Name));
        result = CreateArray<float64>(m_DataStructure, intensityImageDims, {1ULL}, arrayDataPath, IDataAction::Mode::Execute);
      }

      auto intensityPlot1Array = m_DataStructure.getDataRefAs<Float64Array>(amPath.createChildPath(fmt::format("Phase_{}_{}", phase, m_InputValues->IntensityPlot1Name)));
      auto intensityPlot2Array = m_DataStructure.getDataRefAs<Float64Array>(amPath.createChildPath(fmt::format("Phase_{}_{}", phase, m_InputValues->IntensityPlot2Name)));
      auto intensityPlot3Array = m_DataStructure.getDataRefAs<Float64Array>(amPath.createChildPath(fmt::format("Phase_{}_{}", phase, m_InputValues->IntensityPlot3Name)));

      std::vector<size_t> compDims = {1ULL};
      for(int imageIndex = 0; imageIndex < figures.size(); imageIndex++)
      {
        intensityImages[imageIndex] = flipAndMirrorPoleFigure<double>(intensityImages[imageIndex].get(), config);
      }

      std::copy(intensityImages[0]->begin(), intensityImages[0]->end(), intensityPlot1Array.begin());
      std::copy(intensityImages[1]->begin(), intensityImages[1]->end(), intensityPlot2Array.begin());
      std::copy(intensityImages[2]->begin(), intensityImages[2]->end(), intensityPlot3Array.begin());

      DataPath metaDataPath = m_InputValues->IntensityGeometryDataPath.createChildPath(write_pole_figure::k_MetaDataName);
      auto metaDataArrayRef = m_DataStructure.getDataRefAs<StringArray>(metaDataPath);
      if(metaDataArrayRef.getNumberOfTuples() != numPhases)
      {
        metaDataArrayRef.resizeTuples(std::vector<usize>{numPhases});
      }

      std::vector<std::string> laueNames = LaueOps::GetLaueNames();
      const uint32_t laueIndex = crystalStructures[phase];
      const std::string materialName = materialNames[phase];

      metaDataArrayRef[phase] = fmt::format("Phase Num: {}\nMaterial Name: {}\nLaue Group: {}\nHemisphere: Northern\nSamples: {}\nLambert Square Dim: {}", phase, materialName, laueNames[laueIndex],
                                            config.eulers->getNumberOfTuples(), config.lambertDim);
    }

    if(figures.size() == 3)
    {
      const auto imageWidth = static_cast<int32>(config.imageDim);
      const auto imageHeight = static_cast<int32>(config.imageDim);
      const float32 fontPtSize = static_cast<float>(imageHeight) / 16.0f;
      const float32 margins = static_cast<float>(imageHeight) / 32.0f;

      int32 pageWidth = 0;
      auto pageHeight = static_cast<int32>(margins + fontPtSize);

      float32 xCharWidth = 0.0f;
      {
        canvas_ity::canvas tempContext(m_InputValues->ImageSize, m_InputValues->ImageSize);
        const std::array<char, 2> buf = {'X', 0};
        tempContext.set_font(m_LatoBold.data(), static_cast<int>(m_LatoBold.size()), fontPtSize);
        xCharWidth = tempContext.measure_text(buf.data());
      }
      // Each Pole Figure gets its own Square mini canvas to draw into.
      const float32 subCanvasWidth = margins + static_cast<float32>(imageWidth) + xCharWidth + margins;
      const float32 subCanvasHeight = margins + fontPtSize + static_cast<float32>(imageHeight) + fontPtSize * 2 + margins * 2;

      std::vector<std::pair<float32, float32>> globalImageOrigins(4);
      if(static_cast<WritePoleFigure::LayoutType>(m_InputValues->ImageLayout) == WritePoleFigure::LayoutType::Horizontal)
      {
        pageWidth = static_cast<int32>(subCanvasWidth) * 4;
        pageHeight = pageHeight + static_cast<int32>(subCanvasHeight);
        globalImageOrigins[0] = std::make_pair(0.0f, static_cast<float>(pageHeight) - subCanvasHeight);
        globalImageOrigins[1] = std::make_pair(subCanvasWidth, static_cast<float>(pageHeight) - subCanvasHeight);
        globalImageOrigins[2] = std::make_pair(subCanvasWidth * 2.0f, static_cast<float>(pageHeight) - subCanvasHeight);
        globalImageOrigins[3] = std::make_pair(subCanvasWidth * 3.0f, static_cast<float>(pageHeight) - subCanvasHeight);
      }
      else if(static_cast<WritePoleFigure::LayoutType>(m_InputValues->ImageLayout) == WritePoleFigure::LayoutType::Vertical)
      {
        pageWidth = static_cast<int32>(subCanvasWidth);
        pageHeight = pageHeight + static_cast<int32>(subCanvasHeight) * 4;
        globalImageOrigins[0] = std::make_pair(0.0f, margins + fontPtSize);
        globalImageOrigins[1] = std::make_pair(0.0f, margins + fontPtSize + subCanvasHeight * 1.0f);
        globalImageOrigins[2] = std::make_pair(0.0f, margins + fontPtSize + subCanvasHeight * 2.0f);
        globalImageOrigins[3] = std::make_pair(0.0f, margins + fontPtSize + subCanvasHeight * 3.0f);
      }
      else if(static_cast<WritePoleFigure::LayoutType>(m_InputValues->ImageLayout) == nx::core::WritePoleFigure::LayoutType::Square)
      {
        pageWidth = static_cast<int32>(subCanvasWidth) * 2;
        pageHeight = pageHeight + static_cast<int32>(subCanvasHeight) * 2;
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

      std::vector<size_t> compDims = {4ULL};
      for(int imageIndex = 0; imageIndex < figures.size(); imageIndex++)
      {
        figures[imageIndex] = flipAndMirrorPoleFigure(figures[imageIndex].get(), config);
        figures[imageIndex] = convertColorOrder(figures[imageIndex].get(), config);
      }

      for(int i = 0; i < 3; i++)
      {
        std::array<float, 2> figureOrigin = {0.0f, 0.0f};
        std::tie(figureOrigin[0], figureOrigin[1]) = globalImageOrigins[i];
        context.draw_image(figures[i]->getPointer(0), imageWidth, imageHeight, imageWidth * figures[i]->getNumberOfComponents(), figureOrigin[0] + margins,
                           figureOrigin[1] + fontPtSize * 2.0f + margins * 2.0f, static_cast<float32>(imageWidth), static_cast<float32>(imageHeight));

        // Draw an outline on the figure
        context.begin_path();
        context.line_cap = canvas_ity::circle;
        context.set_line_width(3.0f);
        context.set_color(canvas_ity::stroke_style, 0.0f, 0.0f, 0.0f, 1.0f);
        context.arc(figureOrigin[0] + margins + static_cast<float32>(m_InputValues->ImageSize) / 2.0f,
                    figureOrigin[1] + fontPtSize * 2.0f + margins * 2.0f + static_cast<float32>(m_InputValues->ImageSize) / 2.0f, static_cast<float32>(m_InputValues->ImageSize) / 2.0f, 0,
                    nx::core::Constants::k_2Pi<float>);
        context.stroke();
        context.close_path();

        // Draw the X Axis lines
        context.begin_path();
        context.line_cap = canvas_ity::square;
        context.set_line_width(2.0f);
        context.set_color(canvas_ity::stroke_style, 0.0f, 0.0f, 0.0f, 1.0f);
        context.move_to(figureOrigin[0] + margins, figureOrigin[1] + fontPtSize * 2.0f + margins * 2.0f + static_cast<float32>(m_InputValues->ImageSize) / 2.0f);
        context.line_to(figureOrigin[0] + margins + static_cast<float32>(m_InputValues->ImageSize),
                        figureOrigin[1] + fontPtSize * 2.0f + margins * 2.0f + static_cast<float32>(m_InputValues->ImageSize) / 2.0f);
        context.stroke();
        context.close_path();

        // Draw the Y Axis lines
        context.begin_path();
        context.line_cap = canvas_ity::square;
        context.set_line_width(2.0f);
        context.set_color(canvas_ity::stroke_style, 0.0f, 0.0f, 0.0f, 1.0f);
        context.move_to(figureOrigin[0] + margins + static_cast<float32>(m_InputValues->ImageSize) / 2.0f, figureOrigin[1] + fontPtSize * 2.0f + margins * 2.0f);
        context.line_to(figureOrigin[0] + margins + static_cast<float32>(m_InputValues->ImageSize) / 2.0f,
                        figureOrigin[1] + fontPtSize * 2.0f + margins * 2.0f + static_cast<float32>(m_InputValues->ImageSize));
        context.stroke();
        context.close_path();

        // Draw X Axis Label
        context.begin_path();
        context.set_font(m_LatoBold.data(), static_cast<int>(m_LatoBold.size()), fontPtSize);
        context.set_color(canvas_ity::fill_style, 0.0f, 0.0f, 0.0f, 1.0f);
        context.text_baseline = baselines[0];
        context.fill_text("X", figureOrigin[0] + margins * 2.0f + static_cast<float32>(m_InputValues->ImageSize),
                          figureOrigin[1] + fontPtSize * 2.25f + margins * 2.0f + static_cast<float32>(m_InputValues->ImageSize) / 2.0f);
        context.close_path();

        // Draw Y Axis Label
        context.begin_path();
        context.set_font(m_LatoBold.data(), static_cast<int>(m_LatoBold.size()), fontPtSize);
        context.set_color(canvas_ity::fill_style, 0.0f, 0.0f, 0.0f, 1.0f);
        context.text_baseline = baselines[0];
        const float yFontWidth = context.measure_text("Y");
        context.fill_text("Y", figureOrigin[0] + margins - (0.5f * yFontWidth) + static_cast<float32>(m_InputValues->ImageSize) / 2.0f, figureOrigin[1] + fontPtSize * 2.0f + margins);
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
        drawInformationBlock(context, config, globalImageOrigins[3], margins, static_cast<float32>(imageHeight) / 20.0f, static_cast<int32_t>(phase), m_LatoRegular, laueNames[laueIndex],
                             materialName);
      }
      else
      {
        drawScalarBar(context, config, globalImageOrigins[3], margins, static_cast<float32>(imageHeight) / 20.0f, static_cast<int32_t>(phase), m_LatoRegular, laueNames[laueIndex], materialName);
      }

      // Fetch the rendered RGBA pixels from the entire canvas.
      std::vector<unsigned char> rgbaCanvasImage(static_cast<size_t>(pageHeight * pageWidth * 4));
      context.get_image_data(rgbaCanvasImage.data(), pageWidth, pageHeight, pageWidth * 4, 0, 0);
      if(m_InputValues->SaveAsImageGeometry)
      {
        // Ensure the final Image Geometry is sized correctly.
        imageGeom.setDimensions({static_cast<usize>(pageWidth), static_cast<usize>(pageHeight), 1});
        imageGeom.getCellData()->resizeTuples({1, static_cast<usize>(pageHeight), static_cast<usize>(pageWidth)});
        tupleShape[0] = 1;
        tupleShape[1] = pageHeight;
        tupleShape[2] = pageWidth;
        // Create an output array to hold the RGB formatted color image
        auto imageArrayPath = cellAttrMatPath.createChildPath(fmt::format("Phase_{}", phase));
        auto arrayCreationResult = nx::core::CreateArray<uint8>(m_DataStructure, tupleShape, {3ULL}, imageArrayPath, IDataAction::Mode::Execute);
        if(arrayCreationResult.invalid())
        {
          return arrayCreationResult;
        }

        // Get a reference to the RGB final array and then copy ONLY the RGB pixels from the
        // canvas RGBA data.
        auto& imageData = m_DataStructure.getDataRefAs<UInt8Array>(imageArrayPath);

        imageData.fill(0);
        size_t tupleCount = pageHeight * pageWidth;
        for(size_t t = 0; t < tupleCount; t++)
        {
          imageData[t * 3 + 0] = rgbaCanvasImage[t * 4 + 0];
          imageData[t * 3 + 1] = rgbaCanvasImage[t * 4 + 1];
          imageData[t * 3 + 2] = rgbaCanvasImage[t * 4 + 2];
        }
      }

      // Write out the full RGBA data
      if(m_InputValues->WriteImageToDisk)
      {
        const std::string filename = fmt::format("{}/{}{}.tiff", m_InputValues->OutputPath.string(), m_InputValues->ImagePrefix, phase);
        auto result = TiffWriter::WriteImage(filename, pageWidth, pageHeight, 4, rgbaCanvasImage.data());
        if(result.first < 0)
        {
          return MakeErrorResult(-53900, fmt::format("Error writing pole figure image '{}' to disk.\n    Error Code from Tiff Writer: {}\n    Message: {}", filename, result.first, result.second));
        }
      }
    }
  }
  return {};
}
