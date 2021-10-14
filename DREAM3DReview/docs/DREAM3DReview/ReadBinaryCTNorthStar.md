# ReadBinaryCTNorthStar #

| Ready | Parameter Key | Human Name | Parameter Type | Parameter Class |
|-------|---------------|------------|-----------------|----------------|
| YES | InputHeaderFile | Input Header File | FileSystemPathParameter::ValueType | FileSystemPathParameter |
| YES | DataContainerName | Data Container | StringParameter::ValueType | StringParameter |
| YES | CellAttributeMatrixName | Cell Attribute Matrix | DataPath | ArrayCreationParameter |
| YES | DensityArrayName | Density | DataPath | ArrayCreationParameter |
| YES | LengthUnit | Length Unit | ChoicesParameter::ValueType | ChoicesParameter |
| NO | VolumeDescription | Original Volume | <<<NOT_IMPLEMENTED>>> | PreflightUpdatedValueFilterParameter |
| NO | DataFileInfo | Dat Files | <<<NOT_IMPLEMENTED>>> | PreflightUpdatedValueFilterParameter |
| YES | ImportSubvolume | Import Subvolume | bool | BoolParameter |
| YES | StartVoxelCoord | Starting XYZ Voxel | VectorInt32Parameter::ValueType | VectorInt32Parameter |
| YES | EndVoxelCoord | Ending XYZ Voxel | VectorInt32Parameter::ValueType | VectorInt32Parameter |
| NO | ImportedVolumeDescription | Imported Volume | <<<NOT_IMPLEMENTED>>> | PreflightUpdatedValueFilterParameter |
