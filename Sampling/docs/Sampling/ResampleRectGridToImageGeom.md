# ResampleRectGridToImageGeom #

| Ready | Parameter Key | Human Name | Parameter Type | Parameter Class |
|-------|---------------|------------|-----------------|----------------|
| YES | RectilinearGridPath | Input Rectilinear Grid | DataPath | DataGroupSelectionParameter |
| YES | SelectedDataArrayPaths | Attribute Arrays to Copy | MultiArraySelectionParameter::ValueType | MultiArraySelectionParameter |
| NO | RectGridGeometryDesc | Rect Grid Geom Info. | <<<NOT_IMPLEMENTED>>> | PreflightUpdatedValueFilterParameter |
| YES | Dimensions | Dimensions (Voxels) | VectorInt32Parameter::ValueType | VectorInt32Parameter |
| NO | CreatedGeometryDescription | Created Volume Info. | <<<NOT_IMPLEMENTED>>> | PreflightUpdatedValueFilterParameter |
| YES | ImageGeometryPath | Data Container | DataPath | DataGroupCreationParameter |
| YES | ImageGeomCellAttributeMatrix | Cell Attribute Matrix | DataPath | ArrayCreationParameter |
