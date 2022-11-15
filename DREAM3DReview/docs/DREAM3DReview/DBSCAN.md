# DBSCAN

| Ready | Parameter Key | Human Name | Parameter Type | Parameter Class |
|-------|---------------|------------|-----------------|----------------|
| YES | Epsilon | Epsilon | float32 | Float32Parameter |
| YES | MinPnts | Minimum Number of Points | int32 | Int32Parameter |
| YES | DistanceMetric | Distance Metric | ChoicesParameter::ValueType | ChoicesParameter |
| YES | UseMask | Use Mask | bool | BoolParameter |
| YES | SelectedArrayPath | Attribute Array to Cluster | DataPath | ArraySelectionParameter |
| YES | MaskArrayPath | Mask | DataPath | ArraySelectionParameter |
| YES | FeatureIdsArrayName | Cluster Ids | DataPath | ArrayCreationParameter |
| YES | FeatureAttributeMatrixName | Cluster Attribute Matrix | DataPath | ArrayCreationParameter |
