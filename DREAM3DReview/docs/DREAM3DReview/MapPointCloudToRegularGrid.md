# MapPointCloudToRegularGrid

| Ready | Parameter Key | Human Name | Parameter Type | Parameter Class |
|-------|---------------|------------|-----------------|----------------|
| YES | SamplingGridType | Sampling Grid Type | ChoicesParameter::ValueType | ChoicesParameter |
| YES | GridDimensions | Grid Dimensions | VectorInt32Parameter::ValueType | VectorInt32Parameter |
| YES | ImageDataContainerPath | ImageDataContainerPath | DataPath | DataGroupSelectionParameter |
| YES | DataContainerName | Data Container to Map | DataPath | DataGroupSelectionParameter |
| YES | UseMask | Use Mask | bool | BoolParameter |
| YES | MaskArrayPath | Mask | DataPath | ArraySelectionParameter |
| YES | VoxelIndicesArrayPath | Voxel Indices | DataPath | ArrayCreationParameter |
| YES | CreatedImageDataContainerName | Created Image DataContainer | DataPath | DataGroupCreationParameter |
