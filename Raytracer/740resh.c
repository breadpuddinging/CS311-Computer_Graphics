// Nathaniel Li

/* A resh is a ray-tracing mesh. It has no geometry uniforms outside the 
attached meshMesh. */
#define reshUNIFDIM 0

/* Given vectors a, b - a, and c - a describing a triangle, with the first three 
entries being XYZ. Given point x, with the first three entries being XYZ, such 
that (up to numerical precision) x - a = p (b - a) + q (c - a). Computes p and 
q. Returns 1 on success or 0 on failure. */
int reshGetPQ(
        const double a[], const double bMinusA[], const double cMinusA[], 
        const double x[], double pq[2]) {
    /* For the 3x2 matrix A with columns b - a, c - a, compute A^T A. */
    double aTA[2][2];
    aTA[0][0] = vecDot(3, bMinusA, bMinusA);
    aTA[0][1] = vecDot(3, bMinusA, cMinusA);
    aTA[1][0] = aTA[0][1];
    aTA[1][1] = vecDot(3, cMinusA, cMinusA);
    /* Compute the 2x2 matrix (A^T A)^-1 if possible. */
    double aTAInv[2][2];
    if (mat22Invert(aTA, aTAInv) == 0.0)
        return 0;
    /* Compute the 2x3 matrix (A^T A)^-1 A^T. */
    double aTAInvAT[2][3];
    for (int i = 0; i < 2; i += 1)
        for (int j = 0; j < 3; j += 1)
            aTAInvAT[i][j] = 
                aTAInv[i][0] * bMinusA[j] + aTAInv[i][1] * cMinusA[j];
    /* Then pq = (A^T A)^-1 A^T (x - a). */
    double xMinusA[3];
    vecSubtract(3, x, a, xMinusA);
    pq[0] = vecDot(3, aTAInvAT[0], xMinusA);
    pq[1] = vecDot(3, aTAInvAT[1], xMinusA);
    return 1;
}

double reshGetTriangleIntersection(const double pLocal[3], const double dLocal[3], const double aLocal[3], 
        const double bLocal[3], const double cLocal[3], double bound) {
    double bMinA[3], cMinA[3];
        // Compute c-a and b-a and normal
        double uNormal[3];
        vecSubtract(3, bLocal, aLocal, bMinA);
        vecSubtract(3, cLocal, aLocal, cMinA);
        vec3Cross(cMinA, bMinA, uNormal);
        //vecUnit(3, uNormal, uNormal);
        double nDotD = vecDot(3, uNormal, dLocal);
        if (nDotD == 0.0) {
            return rayNONE;
        } else {
            double aMinP[3];
            vecSubtract(3, aLocal, pLocal, aMinP);
            double t = vecDot(3, uNormal, aMinP) / nDotD;
            if (rayEPSILON < t && t < bound) { // Check if plane of triangle has been hit
                double td[3], xLocal[3];
                vecScale(3, t, dLocal, td); 
                vecAdd(3, pLocal, td, xLocal);
                
                double pq[2];
                if (reshGetPQ(aLocal, bMinA, cMinA, xLocal, pq) != 1) {
                    return rayNONE;
                }
                if (0.0 <= pq[0] && 0.0 <= pq[1] && (pq[0] + pq[1]) <= 1.0) { // Check if triangle has been hit
                    return t;
                }
            } 
            return rayNONE;
        }
}

/* An implementation of getIntersection for bodies that are reshes. Assumes that 
the data parameter points to an underlying meshMesh with attribute structure 
XYZSTNOP. */
void reshGetIntersection(
        int unifDim, const double unif[], const void *data, 
        const isoIsometry *isom, const double p[3], const double d[3], 
        double bound, rayIntersection* inter) {
    meshMesh *mesh = (meshMesh *)data;
    double pLocal[3], dLocal[3], *aLocal, *bLocal, *cLocal;
    isoUntransformPoint(isom, p, pLocal);
    isoUnrotateDirection(isom, d, dLocal);
    inter->t = rayNONE;
    for (int i = 0; i < mesh->triNum; i++) {
        // Get the vertices of the triangle and put into length 3 int array
        int *verticeIndices = meshGetTrianglePointer(mesh, i);	
        // Get the vertices
        aLocal = meshGetVertexPointer(mesh, verticeIndices[0]);
        bLocal = meshGetVertexPointer(mesh, verticeIndices[1]);
        cLocal = meshGetVertexPointer(mesh, verticeIndices[2]);
        
        double t = reshGetTriangleIntersection(pLocal, dLocal, aLocal, bLocal, cLocal, bound);
        if (t != rayNONE) {
            bound = t;
            inter->t = t;
            inter->index = i;
        }
    }
}

/* An implementation of getTexCoordsAndNormal for bodies that are reshes. 
Assumes that the data parameter points to an underlying meshMesh with attribute 
structure XYZSTNOP. */
void reshGetTexCoordsAndNormal(
        int unifDim, const double unif[], const void *data, 
        const isoIsometry *isom, const double p[3], const double d[3], 
        const rayIntersection *inter, double texCoords[2], double normal[3]) {
    meshMesh *mesh = (meshMesh *)data;
    // Transform p and d to local space
    double pLocal[3], dLocal[3], td[3];
    isoUntransformPoint(isom, p, pLocal);
    isoUnrotateDirection(isom, d, dLocal);
    // Get the vertices of the triangle and put into length 3 int array
    int *verticeIndices = meshGetTrianglePointer(mesh, inter->index);	
    
    // Get the vertices
    double *aLocal = meshGetVertexPointer(mesh, verticeIndices[0]);
    double *bLocal = meshGetVertexPointer(mesh, verticeIndices[1]);
    double *cLocal = meshGetVertexPointer(mesh, verticeIndices[2]);
    double bMinA[8], cMinA[8];
    // Compute c-a and b-a
    vecSubtract(mesh->attrDim, bLocal, aLocal, bMinA);
    vecSubtract(mesh->attrDim, cLocal, aLocal, cMinA);
    // Compute p and q
    double x[mesh->attrDim];
    vecScale(3, inter->t, dLocal, td);
    vecAdd(3, pLocal, td, x);
    double pq[2];
    reshGetPQ(aLocal, bMinA, cMinA, x, pq); 
    // Compute texCoords and normal
    vecScale(mesh->attrDim, pq[0], bMinA, bMinA);
    vecScale(mesh->attrDim, pq[1], cMinA, cMinA);
    vecAdd(mesh->attrDim, bMinA, cMinA, bMinA);
    vecAdd(mesh->attrDim, bMinA, aLocal, x);
    texCoords[0] = x[3];
    texCoords[1] = x[4];
    
    double normalLocal[3] = {x[5], x[6], x[7]};
    isoRotateDirection(isom, normalLocal, normal);
    vecUnit(3, normal, normal);
}


