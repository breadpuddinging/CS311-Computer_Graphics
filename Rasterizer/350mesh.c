// Nathaniel Li


/*** Creating and destroying ***/

/* Feel free to read the struct's members, but don't write them, except through 
the accessors below such as meshSetTriangle, meshSetVertex. */
typedef struct meshMesh meshMesh;
struct meshMesh {
	int triNum, vertNum, attrDim;
	int *tri;						/* triNum * 3 ints */
	double *vert;					/* vertNum * attrDim doubles */
};

/* Initializes a mesh with enough memory to hold its triangles and vertices. 
Does not actually fill in those triangles or vertices with useful data. When 
you are finished with the mesh, you must call meshFinalize to deallocate its 
backing resources. */
int meshInitialize(meshMesh *mesh, int triNum, int vertNum, int attrDim) {
	mesh->tri = (int *)malloc(triNum * 3 * sizeof(int) +
		vertNum * attrDim * sizeof(double));
	if (mesh->tri != NULL) {
		mesh->vert = (double *)&(mesh->tri[triNum * 3]);
		mesh->triNum = triNum;
		mesh->vertNum = vertNum;
		mesh->attrDim = attrDim;
	}
	return (mesh->tri == NULL);
}

/* Sets the trith triangle to have vertex indices i, j, k. */
void meshSetTriangle(meshMesh *mesh, int tri, int i, int j, int k) {
	if (0 <= tri && tri < mesh->triNum) {
		mesh->tri[3 * tri] = i;
		mesh->tri[3 * tri + 1] = j;
		mesh->tri[3 * tri + 2] = k;
	}
}

/* Returns a pointer to the trith triangle. For example:
	int *triangle13 = meshGetTrianglePointer(&mesh, 13);
	printf("%d, %d, %d\n", triangle13[0], triangle13[1], triangle13[2]); */
int *meshGetTrianglePointer(const meshMesh *mesh, int tri) {
	if (0 <= tri && tri < mesh->triNum)
		return &mesh->tri[tri * 3];
	else
		return NULL;
}

/* Sets the vertth vertex to have attributes attr. */
void meshSetVertex(meshMesh *mesh, int vert, const double attr[]) {
	int k;
	if (0 <= vert && vert < mesh->vertNum)
		for (k = 0; k < mesh->attrDim; k += 1)
			mesh->vert[mesh->attrDim * vert + k] = attr[k];
}

/* Returns a pointer to the vertth vertex. For example:
	double *vertex13 = meshGetVertexPointer(&mesh, 13);
	printf("x = %f, y = %f\n", vertex13[0], vertex13[1]); */
double *meshGetVertexPointer(const meshMesh *mesh, int vert) {
	if (0 <= vert && vert < mesh->vertNum)
		return &mesh->vert[vert * mesh->attrDim];
	else
		return NULL;
}

/* Deallocates the resources backing the mesh. This function must be called 
when you are finished using a mesh. */
void meshFinalize(meshMesh *mesh) {
	free(mesh->tri);
}



/*** Writing and reading files ***/

/* Helper function for meshInitializeFile. */
int meshFileError(
        meshMesh *mesh, FILE *file, const char *cause, const int line) {
	fprintf(stderr, "error: meshInitializeFile: %s at line %d\n", cause, line);
	fclose(file);
	meshFinalize(mesh);
	return 3;
}

/* Initializes a mesh from a mesh file. The file format is documented at 
meshSaveFile. This function does not do as much error checking as one might 
like. Use it only on trusted, non-corrupted files, such as ones that you have 
recently created using meshSaveFile. Returns 0 on success, non-zero on failure. 
Don't forget to invoke meshFinalize when you are done using the mesh. */
int meshInitializeFile(meshMesh *mesh, const char *path) {
	FILE *file = fopen(path, "r");
	if (file == NULL) {
		fprintf(stderr, "error: meshInitializeFile: fopen failed\n");
		return 1;
	}
	int year, month, day, triNum, vertNum, attrDim;
	// Future work: Check version.
	if (fscanf(file, "Carleton College CS 311 mesh version %d/%d/%d\n", &year, 
			&month, &day) != 3) {
		fprintf(stderr, "error: meshInitializeFile: bad header at line 1\n");
		fclose(file);
		return 1;
	}
	if (fscanf(file, "triNum %d\n", &triNum) != 1) {
		fprintf(stderr, "error: meshInitializeFile: bad triNum at line 2\n");
		fclose(file);
		return 2;
	}
	if (fscanf(file, "vertNum %d\n", &vertNum) != 1) {
		fprintf(stderr, "error: meshInitializeFile: bad vertNum at line 3\n");
		fclose(file);
		return 3;
	}
	if (fscanf(file, "attrDim %d\n", &attrDim) != 1) {
		fprintf(stderr, "error: meshInitializeFile: bad attrDim at line 4\n");
		fclose(file);
		return 4;
	}
	if (meshInitialize(mesh, triNum, vertNum, attrDim) != 0) {
		fclose(file);
		return 5;
	}
	int line = 5, *tri, j, check;
	if (fscanf(file, "%d Triangles:\n", &check) != 1 || check != triNum)
		return meshFileError(mesh, file, "bad header", line);
	for (line = 6; line < triNum + 6; line += 1) {
		tri = meshGetTrianglePointer(mesh, line - 6);
		if (fscanf(file, "%d %d %d\n", &tri[0], &tri[1], &tri[2]) != 3)
			return meshFileError(mesh, file, "bad triangle", line);
		if (0 > tri[0] || tri[0] >= vertNum || 0 > tri[1] || tri[1] >= vertNum 
				|| 0 > tri[2] || tri[2] >= vertNum)
			return meshFileError(mesh, file, "bad index", line);
	}
	double *vert;
	if (fscanf(file, "%d Vertices:\n", &check) != 1 || check != vertNum)
		return meshFileError(mesh, file, "bad header", line);
	for (line = triNum + 7; line < triNum + 7 + vertNum; line += 1) {
		vert = meshGetVertexPointer(mesh, line - (triNum + 7));
		for (j = 0; j < attrDim; j += 1) {
			if (fscanf(file, "%lf ", &vert[j]) != 1)
				return meshFileError(mesh, file, "bad vertex", line);
		}
		if (fscanf(file, "\n") != 0)
			return meshFileError(mesh, file, "bad vertex", line);
	}
	// Future work: Check EOF.
	fclose(file);
	return 0;
}

/* Saves a mesh to a file in a simple custom format (not any industry 
standard). Returns 0 on success, non-zero on failure. The first line is a 
comment of the form 'Carleton College CS 311 mesh version YYYY/MM/DD'.

I now describe version 2019/01/15. The second line says 'triNum [triNum]', 
where the latter is an integer value. The third and fourth lines do the same 
for vertNum and attrDim. The fifth line says '[triNum] Triangles:'. Then there 
are triNum lines, each holding three integers between 0 and vertNum - 1 
(separated by a space). Then there is a line that says '[vertNum] Vertices:'. 
Then there are vertNum lines, each holding attrDim floating-point numbers 
(terminated by a space). */
int meshSaveFile(const meshMesh *mesh, const char *path) {
	FILE *file = fopen(path, "w");
	if (file == NULL) {
		fprintf(stderr, "error: meshSaveFile: fopen failed\n");
		return 1;
	}
	fprintf(file, "Carleton College CS 311 mesh version 2019/01/15\n");
	fprintf(file, "triNum %d\n", mesh->triNum);
	fprintf(file, "vertNum %d\n", mesh->vertNum);
	fprintf(file, "attrDim %d\n", mesh->attrDim);
	fprintf(file, "%d Triangles:\n", mesh->triNum);
	int i, j;
	int *tri;
	for (i = 0; i < mesh->triNum; i += 1) {
		tri = meshGetTrianglePointer(mesh, i);
		fprintf(file, "%d %d %d\n", tri[0], tri[1], tri[2]);
	}
	fprintf(file, "%d Vertices:\n", mesh->vertNum);
	double *vert;
	for (i = 0; i < mesh->vertNum; i += 1) {
		vert = meshGetVertexPointer(mesh, i);
		for (j = 0; j < mesh->attrDim; j += 1)
			fprintf(file, "%f ", vert[j]);
		fprintf(file, "\n");
	}
	fclose(file);
	return 0;
}

/*** Rendering ***/

// Get the intersection between a side of a triangle and the near plane
void getTriangleNearPlaneIntersect(int varyDim, double v1[], double v2[], double nearPlaneIntersect[]) {
    double t = (v1[2] + v1[3]) / (v1[2] + v1[3] - v2[2] - v2[3]);
	double bMinusA[varyDim];
    vecSubtract(varyDim, v2, v1, bMinusA);
	vecScale(varyDim, t, bMinusA, bMinusA);
	vecAdd(varyDim, v1, bMinusA, nearPlaneIntersect);
}

// Perform viewport transformation on a vertice
void viewportTransform(const double viewport[4][4], int varyDim, double v[], double varySS[]) {
	vecCopy(varyDim, v, varySS);
	mat441Multiply(viewport, v, varySS);
	vecScale(varyDim, 1/varySS[3], varySS, varySS);
}

// Perform viewport transformation on the vertices and render a triangle
void clipFinal(const meshMesh *mesh, depthBuffer *buf, const double viewport[4][4], 
		const shaShading *sha, const double unif[], const texTexture *tex[], double v1[], double v2[], double v3[]) {
	double v1SS[sha->varyDim], v2SS[sha->varyDim], v3SS[sha->varyDim];
	viewportTransform(viewport, sha->varyDim, v1, v1SS);
	viewportTransform(viewport, sha->varyDim, v2, v2SS);
	viewportTransform(viewport, sha->varyDim, v3, v3SS);
	triRender(sha, buf, unif, tex, v1SS, v2SS, v3SS);
}

// Create and call clipFinal on triangles for clipping one vertex
void clipOneVertex(const meshMesh *mesh, depthBuffer *buf, const double viewport[4][4], 
		const shaShading *sha, const double unif[], const texTexture *tex[], double v1[], double v2[], double v3[]) {
	double x1[sha->varyDim], x2[sha->varyDim];
	getTriangleNearPlaneIntersect(sha->varyDim, v2, v3, x1);
	getTriangleNearPlaneIntersect(sha->varyDim, v1, v3, x2);
	clipFinal(mesh, buf, viewport, sha, unif, tex, v1, v2, x1);
	clipFinal(mesh, buf, viewport, sha, unif, tex, v1, x1, x2);
}

// Create and call clipFinal on triangles for clipping two vertices
void clipTwoVertex(const meshMesh *mesh, depthBuffer *buf, const double viewport[4][4], 
		const shaShading *sha, const double unif[], const texTexture *tex[], double v1[], double v2[], double v3[]) {
	double x1[sha->varyDim], x2[sha->varyDim];
	getTriangleNearPlaneIntersect(sha->varyDim, v1, v2, x1);
	getTriangleNearPlaneIntersect(sha->varyDim, v1, v3, x2);
	clipFinal(mesh, buf, viewport, sha, unif, tex, v1, x1, x2);
}

/* Renders the mesh. If the mesh and the shading have differing values for 
attrDim, then prints an error message and does not render anything. */
void meshRender(
        const meshMesh *mesh, depthBuffer *buf, const double viewport[4][4], 
		const shaShading *sha, const double unif[], const texTexture *tex[]) {
	// Check mesh and shading attrDim values
	if (sha->attrDim == mesh->attrDim) {

		// translate, project, viewport transform, and scale each vertice and store it in an 2D array
		double vary[mesh->vertNum][sha->varyDim];
		for (int i = 0; i < mesh->vertNum; i++) {
			sha->shadeVertex(sha->unifDim, unif, sha->attrDim, meshGetVertexPointer(mesh, i), sha->varyDim, vary[i]);
		}

		// Loop over each triangle
		for (int i = 0; i < mesh->triNum; i++) {
			// Get the vertices of the triangle and put into length 3 int array
			int *verticeIndices = meshGetTrianglePointer(mesh, i);	

            double *a = vary[verticeIndices[0]];
			double *b = vary[verticeIndices[1]];
			double *c = vary[verticeIndices[2]];
 
           	if (a[3] <= 0 || a[3] < - a[2]) { // Check A
                if (b[3] <= 0 || b[3] < - b[2]) { // Check B
                    // A and B are clipped
                    if (c[3] <= 0 || c[3] < - c[2]) {//(c[3] > 0 || c[3] >= - c[2]) { // If C is NOT clipped
						
                    } else {
						clipTwoVertex(mesh, buf, viewport, sha, unif, tex, c, a, b);
					}
                // A and C are clipped
                } else if (c[3] <= 0 || c[3] < - c[2]) { // Check C
					clipTwoVertex(mesh, buf, viewport, sha, unif, tex, b, c, a);
                // A is clipped
                } else {
					clipOneVertex(mesh, buf, viewport, sha, unif, tex, b, c, a);
                }
            } else if (b[3] <= 0 || b[3] < - b[2]) { // Check B
                // B and C are clipped
                if (c[3] <= 0 || c[3] < - c[2]) { // Check C
					clipTwoVertex(mesh, buf, viewport, sha, unif, tex, a, b, c);
                // B is clipped
                } else {
					clipOneVertex(mesh, buf, viewport, sha, unif, tex, c, a, b);
                }
            // C is clipped
            } else if (c[3] <= 0 || c[3] < - c[2]) { // Check C
				clipOneVertex(mesh, buf, viewport, sha, unif, tex, a, b, c);
            // Not A or B or C are clipped
            } else {
				clipFinal(mesh, buf, viewport, sha, unif, tex, a, b, c);
            }
		}
	}
}


