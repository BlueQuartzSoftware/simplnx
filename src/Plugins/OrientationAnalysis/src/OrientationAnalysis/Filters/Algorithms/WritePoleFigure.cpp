#include "WritePoleFigure.hpp"

#include "OrientationAnalysis/utilities/delaunator.h"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Pipeline/Pipeline.hpp"
#include "simplnx/Utilities/ArrayCreationUtilities.hpp"
#include "simplnx/Utilities/IntersectionUtilities.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"
#include "simplnx/Utilities/Parsing/DREAM3D/Dream3dIO.hpp"
#include "simplnx/Utilities/RTree.hpp"

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

#include "H5Support/H5Lite.h"
#include "H5Support/H5ScopedSentinel.h"
#include "H5Support/H5Utilities.h"

using namespace nx::core;

namespace
{
const bool k_UseDiscreteHeatMap = false;

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
      size_t numCoords = m_XYZCoords->getNumberOfTuples();
      float* xyzPtr = m_XYZCoords->getPointer(0);
      for(size_t i = 0; i < numCoords; i++)
      {
        if(xyzPtr[i * 3 + 2] < 0.0f) // If the unit sphere data is in the southern hemisphere
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
      ebsdlib::ModifiedLambertProjection::Pointer lambert = ebsdlib::ModifiedLambertProjection::LambertBallToSquare(m_XYZCoords, m_Config->lambertDim, m_Config->sphereRadius);
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
    // int percent = 0;
    int counter = xPositionsPtr.size() / 100;
    RTreeType m_RTree;
    // Populate the RTree

    size_t numTris = delaunayGeom->getNumberOfFaces();
    for(size_t tIndex = 0; tIndex < numTris; tIndex++)
    {
      std::array<float, 6> boundBox = nx::core::IntersectionUtilities::GetBoundingBoxAtTri(*delaunayGeom, tIndex);
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
          // percent += 1;
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
        bool inTriangle = nx::core::IntersectionUtilities::RayTriangleIntersect2(rayOrigin, rayDirection, v0, v1, v2, barycentricCoord);
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

  int writeLambertData(ebsdlib::ModifiedLambertProjection::Pointer lambert) const
  {
    int err = -1;

    int m_Dimension = lambert->getDimension();
    ebsdlib::DoubleArrayType::Pointer m_NorthSquare = lambert->getNorthSquare();
    ebsdlib::DoubleArrayType::Pointer m_SouthSquare = lambert->getSouthSquare();

    // We want half the sphere area for each square because each square represents a hemisphere.
    const float32 sphereRadius = 1.0f;
    float halfSphereArea = 4.0f * ebsdlib::constants::k_PiF * sphereRadius * sphereRadius / 2.0f;
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

        ebsdlib::LambertUtilities::LambertSquareVertToSphereVert(vert.data(), ebsdlib::LambertUtilities::Hemisphere::North);

        northSphereCoords[index * 3] = vert[0];
        northSphereCoords[index * 3 + 1] = vert[1];
        northSphereCoords[index * 3 + 2] = vert[2];

        northStereoCoords[index * 3] = vert[0] / (1.0f + vert[2]);
        northStereoCoords[index * 3 + 1] = vert[1] / (1.0f + vert[2]);
        northStereoCoords[index * 3 + 2] = 0.0f;

        // Reset the Lambert Square Coord
        vert[0] = origin + (static_cast<float>(x) * m_StepSize);
        vert[1] = origin + (static_cast<float>(y) * m_StepSize);
        ebsdlib::LambertUtilities::LambertSquareVertToSphereVert(vert.data(), ebsdlib::LambertUtilities::Hemisphere::South);

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
    Result result = ArrayCreationUtilities::CreateArray<IGeometry::MeshIndexType>(dataStructure, faceTupleShape, {3ULL}, sharedFaceListPath, IDataAction::Mode::Execute);
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
    result = ArrayCreationUtilities::CreateArray<float>(dataStructure, vertexTupleShape, {3}, vertexPath, IDataAction::Mode::Execute);
    if(result.invalid())
    {
      return -2;
      // return MergeResults(result, MakeErrorResult(-5510, fmt::format("{}CreateGeometry2DAction: Could not allocate SharedVertList '{}'", prefix, vertexPath.toString())));
    }
    auto* vertexArray = dataStructure.getDataAs<Float32Array>(vertexPath);
    if(vertexArray == nullptr)
    {
      throw std::runtime_error(fmt::format("DataPath does not point to a DataArray. DataPath: '{}'", vertexPath.toString()));
    }
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

  const size_t numOrientations = config.eulers->getNumberOfTuples();

  // Create an Array to hold the XYZ Coordinates which are the coords on the sphere.
  // this is size for CUBIC ONLY, <001> Family
  std::array<int32_t, 3> symSize = ops.getNumSymmetry();

  const std::vector<size_t> dims = {3};
  const ebsdlib::FloatArrayType::Pointer xyz001 = ebsdlib::FloatArrayType::CreateArray(numOrientations * symSize[0], dims, label0 + std::string("xyzCoords"), true);
  // this is size for CUBIC ONLY, <011> Family
  const ebsdlib::FloatArrayType::Pointer xyz011 = ebsdlib::FloatArrayType::CreateArray(numOrientations * symSize[1], dims, label1 + std::string("xyzCoords"), true);
  // this is size for CUBIC ONLY, <111> Family
  const ebsdlib::FloatArrayType::Pointer xyz111 = ebsdlib::FloatArrayType::CreateArray(numOrientations * symSize[2], dims, label2 + std::string("xyzCoords"), true);

  config.sphereRadius = 1.0f;

  // Generate the coords on the sphere **** Parallelized
  ops.generateSphereCoordsFromEulers(config.eulers, xyz001.get(), xyz011.get(), xyz111.get(), config.hexConvention);

  // These arrays hold the "intensity" images which eventually get converted to an actual Color RGB image
  // Generate the modified Lambert projection images (Squares, 2 of them, 1 for northern hemisphere, 1 for southern hemisphere
  const ebsdlib::DoubleArrayType::Pointer intensity001 = ebsdlib::DoubleArrayType::CreateArray(config.imageDim * config.imageDim, label0 + "_Intensity_Image", true);
  const ebsdlib::DoubleArrayType::Pointer intensity011 = ebsdlib::DoubleArrayType::CreateArray(config.imageDim * config.imageDim, label1 + "_Intensity_Image", true);
  const ebsdlib::DoubleArrayType::Pointer intensity111 = ebsdlib::DoubleArrayType::CreateArray(config.imageDim * config.imageDim, label2 + "_Intensity_Image", true);

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
typename EbsdDataArray<T>::Pointer flipAndMirrorPoleFigure(EbsdDataArray<T>* src, const ebsdlib::PoleFigureConfiguration_t& config)
{
  typename EbsdDataArray<T>::Pointer converted = EbsdDataArray<T>::CreateArray(config.imageDim * config.imageDim, src->getComponentDimensions(), src->getName(), true);
  // We need to flip the image "vertically", which means the bottom row becomes
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

} // namespace

// -----------------------------------------------------------------------------
WritePoleFigure::WritePoleFigure(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WritePoleFigureInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
WritePoleFigure::~WritePoleFigure() noexcept = default;

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
  ShapeType tupleShape = {1, static_cast<usize>(m_InputValues->ImageSize), static_cast<usize>(m_InputValues->ImageSize)};
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
    const ebsdlib::FloatArrayType::Pointer subEulerAnglesPtr = ebsdlib::FloatArrayType::CreateArray(count, eulerCompDim, "Euler_Angles_Per_Phase", true);
    subEulerAnglesPtr->initializeWithValue(std::numeric_limits<float>::signaling_NaN());
    ebsdlib::FloatArrayType& subEulerAngles = *subEulerAnglesPtr;

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

    // std::vector<ebsdlib::UInt8ArrayType::Pointer> figures;

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

    m_MessageHandler.sendInfoMessage(fmt::format("Generating Pole Figures for Phase {}", phase));
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
        m_MessageHandler.sendWarningMessage(
            fmt::format("Phase {} has unknown crystal structure value {}; no pole figures will be generated for this phase.", phase, static_cast<uint32>(crystalStructures[phase])));
        break;
      }

      if(intensityImages.size() == 3)
      {
        DataPath amPath = m_InputValues->IntensityGeometryDataPath.createChildPath(write_pole_figure::k_ImageAttrMatName);
        // If there is more than a single phase we will need to add more arrays to the DataStructure
        if(phase > 1)
        {
          const std::vector<size_t> intensityImageDims = {static_cast<usize>(config.imageDim), static_cast<usize>(config.imageDim), 1ULL};
          DataPath arrayDataPath = amPath.createChildPath(fmt::format("Phase_{}_{}", phase, m_InputValues->IntensityPlot1Name));
          Result<> result = ArrayCreationUtilities::CreateArray<float64>(m_DataStructure, intensityImageDims, {1ULL}, arrayDataPath, IDataAction::Mode::Execute);

          arrayDataPath = amPath.createChildPath(fmt::format("Phase_{}_{}", phase, m_InputValues->IntensityPlot2Name));
          result = ArrayCreationUtilities::CreateArray<float64>(m_DataStructure, intensityImageDims, {1ULL}, arrayDataPath, IDataAction::Mode::Execute);

          arrayDataPath = amPath.createChildPath(fmt::format("Phase_{}_{}", phase, m_InputValues->IntensityPlot3Name));
          result = ArrayCreationUtilities::CreateArray<float64>(m_DataStructure, intensityImageDims, {1ULL}, arrayDataPath, IDataAction::Mode::Execute);
        }

        auto intensityPlot1Array = m_DataStructure.getDataRefAs<Float64Array>(amPath.createChildPath(fmt::format("Phase_{}_{}", phase, m_InputValues->IntensityPlot1Name)));
        auto intensityPlot2Array = m_DataStructure.getDataRefAs<Float64Array>(amPath.createChildPath(fmt::format("Phase_{}_{}", phase, m_InputValues->IntensityPlot2Name)));
        auto intensityPlot3Array = m_DataStructure.getDataRefAs<Float64Array>(amPath.createChildPath(fmt::format("Phase_{}_{}", phase, m_InputValues->IntensityPlot3Name)));

        std::vector<size_t> compDims = {1ULL};
        for(int imageIndex = 0; imageIndex < intensityImages.size(); imageIndex++)
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

        std::vector<std::string> laueNames = ebsdlib::LaueOps::GetLaueNames();
        const uint32_t laueIndex = crystalStructures[phase];
        const std::string materialName = materialNames[phase];

        metaDataArrayRef[phase] = fmt::format("Phase Num: {}\nMaterial Name: {}\nLaue Group: {}\nHemisphere: Northern\nSamples: {}\nLambert Square Dim: {}", phase, materialName, laueNames[laueIndex],
                                              config.eulers->getNumberOfTuples(), config.lambertDim);
      }
    }

    if(m_InputValues->SaveAsImageGeometry || m_InputValues->WriteImageToDisk)
    {
      // Build the composite configuration
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
      compositeConfig.phaseNumber = static_cast<int32_t>(phase);
      compositeConfig.title = m_InputValues->Title;
      compositeConfig.hexConvention = m_InputValues->HexConvention;

      // Generate the composite pole figure image. GeneratePoleFigureComposite routes
      // discrete (non-heatmap) figures to the vector marker renderer and everything
      // else to the raster PoleFigureCompositor.
      ebsdlib::CompositePoleFigureResult compositeResult = ebsdlib::GeneratePoleFigureComposite(compositeConfig);

      if(compositeResult.image == nullptr)
      {
        continue;
      }

      const int32 pageWidth = compositeResult.width;
      const int32 pageHeight = compositeResult.height;

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
        auto arrayCreationResult = ArrayCreationUtilities::CreateArray<uint8>(m_DataStructure, tupleShape, {3ULL}, imageArrayPath, IDataAction::Mode::Execute);
        if(arrayCreationResult.invalid())
        {
          return arrayCreationResult;
        }

        // Get a reference to the RGB final array and then copy ONLY the RGB pixels from the RGBA data.
        auto& imageData = m_DataStructure.getDataRefAs<UInt8Array>(imageArrayPath);
        imageData.fill(0);
        const size_t tupleCount = static_cast<size_t>(pageHeight) * pageWidth;
        const uint8_t* rgbaPtr = compositeResult.image->getPointer(0);
        for(size_t t = 0; t < tupleCount; t++)
        {
          imageData[t * 3 + 0] = rgbaPtr[t * 4 + 0];
          imageData[t * 3 + 1] = rgbaPtr[t * 4 + 1];
          imageData[t * 3 + 2] = rgbaPtr[t * 4 + 2];
        }
      }

      // Write out the full RGBA data
      if(m_InputValues->WriteImageToDisk)
      {
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
