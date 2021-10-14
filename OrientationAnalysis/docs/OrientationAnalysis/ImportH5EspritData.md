# ImportH5EspritData #

| Ready | Parameter Key | Human Name | Parameter Type | Parameter Class |
|-------|---------------|------------|-----------------|----------------|
| YES | InputFile | Input File | FileSystemPathParameter::ValueType | FileSystemPathParameter |
| NO | SelectedScanNames | Scan Names | <<<NOT_IMPLEMENTED>>> | OEMEbsdScanSelectionFilterParameter |
| YES | ZSpacing | Z Spacing (Microns) | float64 | Float64Parameter |
| YES | Origin | Origin (XYZ) | VectorFloat32Parameter::ValueType | VectorFloat32Parameter |
| YES | CombineEulerAngles | Combine phi1, PHI, phi2 into Single Euler Angles Attribute Array | bool | BoolParameter |
| YES | DegreesToRadians | Convert Euler Angles to Radians | bool | BoolParameter |
| YES | ReadPatternData | Import Pattern Data | bool | BoolParameter |
| YES | DataContainerName | Data Container | DataPath | DataGroupCreationParameter |
| YES | CellAttributeMatrixName | Cell Attribute Matrix | DataPath | ArrayCreationParameter |
| YES | CellEnsembleAttributeMatrixName | Cell Ensemble Attribute Matrix | DataPath | ArrayCreationParameter |
