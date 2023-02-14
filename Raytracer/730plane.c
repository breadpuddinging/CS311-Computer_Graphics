// Nathaniel Li

/* A plane has no geometry uniforms. */
#define plaUNIFDIM 0

/* An implementation of getIntersection for bodies that are planes.*/
void plaGetIntersection(
        int unifDim, const double unif[], const void *geomData, const isoIsometry *isom, 
        const double p[3], const double d[3], double bound, 
        rayIntersection* inter) {
    double localP[3], localD[3];
    isoUntransformPoint(isom, p, localP);
    isoUnrotateDirection(isom, d, localD);
    if (localD[2] == 0) {
        inter->t = rayNONE;
        return;
    }
    double t = -localP[2] / localD[2];
    if (rayEPSILON <= t && t <= bound) {
        inter->t = t;
        return;
    }
    inter->t = rayNONE;
}

/* An implementation of getTexCoordsAndNormal for bodies that are planes. */
void plaGetTexCoordsAndNormal(
        int unifDim, const double unif[], const void *geomData, const isoIsometry *isom, 
        const double p[3], const double d[3], const rayIntersection *inter, 
        double texCoords[2], double normal[3]) {
    // Normal
    double localNormal[3] = {0, 0, 1};
    isoRotateDirection(isom, localNormal, normal);

    // Tex Coords
    double localP[3];
    double localD[3];
    isoUntransformPoint(isom, p, localP);
    isoUntransformPoint(isom, d, localD);
    vecScale(2, inter->t, localD, localD);
    vecAdd(2, localP, localD, texCoords);
}


