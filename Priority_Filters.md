| Plugin::ClassName | Human Label | Ready to Port |
|---|---|---|
| ComplexCore::ConditionalSetValue | "Replace Value in Array (Conditional)" | YES  |
| ComplexCore::CreateDataArray | "Create Data Array" | YES |
| ComplexCore::MultiThresholdObjects2 | "Threshold Objects" | NO |
| ComplexCore::ReadDream3DFile | "Read DREAM.3D Data File" | YES |
| ComplexCore::RemoveArrays | "Delete Data" | NO |
| ComplexCore::WriteDream3DFile | "Write DREAM.3D Data File" | YES |
| DREAM3DReview::ApproximatePointCloudHull | "Approximate Point Cloud Hull" | YES |
| DREAM3DReview::ExtractInternalSurfacesFromTriangleGeometry | "Extract Internal Surfaces from Triangle" | YES |
| DREAM3DReview::FindNeighborListStatistics | "Find NeighborList Statistics" | YES |
| DREAM3DReview::InterpolatePointCloudToRegularGrid | "Interpolate Point Cloud to Regular Grid" | YES |
| DREAM3DReview::IterativeClosestPoint | "Iterative Closest Point" | YES |
| DREAM3DReview::MapPointCloudToRegularGrid | "Map Point Cloud to Regular Grid" | YES |
| DREAM3DReview::PointSampleTriangleGeometry | "Point Sample Triangle Geometry" | YES |
| DREAM3DReview::RemoveFlaggedVertices | "Remove Flagged Vertices" | YES |
| DREAM3DReview::RobustAutomaticThreshold | "Robust Automatic Threshold" | YES |
| DREAM3DReview::ApplyTransformationToGeometry | "Apply Transformation to Geometry" | NO |
| ImportExport::ReadStlFile | "Import STL File" | YES  |
| ITKImageProcessing::ITKBinaryThresholdImage | "ITK::Binary Threshold Image Filter" | YES |
| ITKImageProcessing::ITKGradientMagnitudeImage | "ITK::Gradient Magnitude Image Filter" | YES |
| ITKImageProcessing::ITKGrayscaleFillholeImage | "ITK::Grayscale Fillhole Image Filter" | YES |
| ITKImageProcessing::ITKMaskImage | "ITK::Mask Image Filter" | YES |
| ITKImageProcessing::ITKNormalizeImage | "ITK::Normalize Image Filter" | YES |
| ITKImageProcessing::ITKImportImageStack | | YES |
| ITKImageProcessing::ITKImageReader | | YES |
| ITKImageProcessing::ITKImageWriter | | YES |
| Processing::MinSize | "Minimum Size" | YES |
| Reconstruction::ScalarSegmentFeatures | "Segment Features (Scalar)" | YES |
| StatsToolbox::FindDifferenceMap | "Find Difference Map" | YES |
| StatsToolbox::FindSizes | "Find Feature Sizes" |YES  |
| SurfaceMeshing::LaplacianSmoothing | "Laplacian Smoothing" | YES |
| SurfaceMeshing::QuickSurfaceMesh | "Quick Surface Mesh" | YES |
| SurfaceMeshing::TriangleAreaFilter | "Generate Triangle Areas" | YES  |


## Pipeline 1 ##

    "DataContainerReader"
    "ITKGradientMagnitudeImage"
    "RobustAutomaticThreshold"
    "CreateDataArray"
    "ConditionalSetValue"
    "RemoveArrays"
    "ITKMaskImage"
    "ITKGrayscaleFillholeImage"
    "FindDifferenceMap"
    "ITKNormalizeImage"
    "ITKBinaryThresholdImage"
    "MultiThresholdObjects"
    "ScalarSegmentFeatures"
    "RemoveArrays"
    "FindSizes"
    "MinSize"
    "DataContainerWriter"


## Pipeline 2 ##

    "DataContainerReader"
    "ITKGradientMagnitudeImage"
    "RobustAutomaticThreshold"
    "CreateDataArray"
    "ConditionalSetValue"
    "RemoveArrays"
    "QuickSurfaceMesh"
    "ExtractInternalSurfacesFromTriangleGeometry"
    "RemoveArrays"
    "LaplacianSmoothing"
    "TriangleAreaFilter"
    "PointSampleTriangleGeometry"
    "DataContainerWriter"

## Pipeline 3 ##

    "ReadStlFile"
    "TriangleAreaFilter"
    "PointSampleTriangleGeometry"
    "DataContainerWriter"

## Pipeline 4 ##

    "DataContainerReader"
    "MultiThresholdObjects"
    "RemoveFlaggedVertices"
    "RemoveArrays"
    "ApproximatePointCloudHull"
    "DataContainerWriter"

## Pipeline 5 ##

    "DataContainerReader"
    "DataContainerReader"
    "DataContainerReader"
    "IterativeClosestPoint"
    "IterativeClosestPoint"
    "ApplyTransformationToGeometry"
    "ApplyTransformationToGeometry"
    "MapPointCloudToRegularGrid"
    "InterpolatePointCloudToRegularGrid"
    "FindNeighborListStatistics"
    "FindNeighborListStatistics"
    "DataContainerWriter"