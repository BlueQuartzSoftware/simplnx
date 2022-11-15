# DetectEllipsoids

| Ready | Parameter Key | Human Name | Parameter Type | Parameter Class |
|-------|---------------|------------|-----------------|----------------|
| YES | MinFiberAxisLength | Min Fiber Axis Length | int32 | Int32Parameter |
| YES | MaxFiberAxisLength | Max Fiber Axis Length | int32 | Int32Parameter |
| YES | HoughTransformThreshold | Threshold for Hough Transform | float32 | Float32Parameter |
| YES | MinAspectRatio | Minimum Aspect Ratio | float32 | Float32Parameter |
| YES | ImageScaleBarLength | Length of Image Scale Bar | int32 | Int32Parameter |
| YES | FeatureIdsArrayPath | Feature Ids | DataPath | ArraySelectionParameter |
| YES | FeatureAttributeMatrixPath | Feature Attribute Matrix | DataPath | DataGroupSelectionParameter |
| YES | EllipseFeatureAttributeMatrixPath | Ellipsoid Feature Attribute Matrix | DataPath | DataGroupCreationParameter |
| YES | CenterCoordinatesArrayName | Ellipsoid Center Coordinates | DataPath | ArrayCreationParameter |
| YES | MajorAxisLengthArrayName | Ellipsoid Major Axis Lengths | DataPath | ArrayCreationParameter |
| YES | MinorAxisLengthArrayName | Ellipsoid Minor Axis Lengths | DataPath | ArrayCreationParameter |
| YES | RotationalAnglesArrayName | Ellipsoid Rotational Angles | DataPath | ArrayCreationParameter |
| YES | DetectedEllipsoidsFeatureIdsArrayPath | Detected Ellipsoids Feature Ids | DataPath | ArrayCreationParameter |
