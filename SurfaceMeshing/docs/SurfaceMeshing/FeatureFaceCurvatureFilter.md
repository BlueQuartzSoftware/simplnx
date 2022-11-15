# FeatureFaceCurvatureFilter

| Ready | Parameter Key | Human Name | Parameter Type | Parameter Class |
|-------|---------------|------------|-----------------|----------------|
| YES | NRing | Neighborhood Ring Count | int32 | Int32Parameter |
| YES | ComputePrincipalDirectionVectors | Compute Principal Direction Vectors | bool | BoolParameter |
| YES | ComputeGaussianCurvature | Compute Gaussian Curvature | bool | BoolParameter |
| YES | ComputeMeanCurvature | Compute Mean Curvature | bool | BoolParameter |
| YES | UseNormalsForCurveFitting | Use Face Normals for Curve Fitting | bool | BoolParameter |
| YES | FaceAttributeMatrixPath | Face Attribute Matrix | DataPath | DataGroupSelectionParameter |
| YES | SurfaceMeshFaceLabelsArrayPath | Face Labels | DataPath | ArraySelectionParameter |
| YES | SurfaceMeshFeatureFaceIdsArrayPath | Feature Face Ids | DataPath | ArraySelectionParameter |
| YES | SurfaceMeshFaceNormalsArrayPath | Face Normals | DataPath | ArraySelectionParameter |
| YES | SurfaceMeshTriangleCentroidsArrayPath | Face Centroids | DataPath | ArraySelectionParameter |
| YES | SurfaceMeshPrincipalCurvature1sArrayName | Principal Curvature 1 | DataPath | ArrayCreationParameter |
| YES | SurfaceMeshPrincipalCurvature2sArrayName | Principal Curvature 2 | DataPath | ArrayCreationParameter |
| YES | SurfaceMeshPrincipalDirection1sArrayName | Principal Direction 1 | DataPath | ArrayCreationParameter |
| YES | SurfaceMeshPrincipalDirection2sArrayName | Principal Direction 2 | DataPath | ArrayCreationParameter |
| YES | SurfaceMeshGaussianCurvaturesArrayName | Gaussian Curvature | DataPath | ArrayCreationParameter |
| YES | SurfaceMeshMeanCurvaturesArrayName | Mean Curvature | DataPath | ArrayCreationParameter |
