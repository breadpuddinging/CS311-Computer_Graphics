// Nathaniel Li

// Returns the min. of two ints
int min(int a, int b) {
    if (a < b) {
        return a;
    }
    return b;
}

// Returns the max. of two ints
int max(int a, int b) {
    if (a < b) {
        return b;
    }
    return a;
}

/* Returns the Y value the inner loop of rasterization should start from
   Makes sure the the loop does not continue outside the bounds of the window */
int getYLoopStart(const double v1[], const double v2[], int x) {
    int y = ceil(v1[1] + (v2[1] - v1[1])/(v2[0] - v1[0]) * (x - v1[0]));
    if (y > 0) {
        return y;
    }
    return 0;
}

/* Returns the Y value the inner loop of rasterization should end at
   Makes sure the the loop does not continue outside the bounds of the window */
int getYLoopEnd(int bufHeight, const double v1[], const double v2[], int x) {
    int y = floor(v1[1] + (v2[1]  - v1[1]) / (v2[0] - v1[0]) * (x - v1[0]));
    if (y < bufHeight) {
        return y;
    }
    return bufHeight;
}

void findPixelColor(
        const shaShading *sha, depthBuffer *buf, const double unif[], const texTexture *tex[],
        const double a[], const double b[], const double c[], double mInverse[][2], int x, int y) {
    
    // Below code produces a vector containing the "p and q" values in vectorBlend
    double pixelPos[2] = {x, y};
    double axVector[2];
    vecSubtract(2, pixelPos, a, axVector);
    double vectorBlend[2]; 
    mat221Multiply(mInverse, axVector, vectorBlend);

    // Varying vector. 
    double vary[sha->varyDim];
    for (int i = 0; i < sha->varyDim; i++) {
        vary[i] = a[i] + vectorBlend[0] * (b[i] - a[i]) + vectorBlend[1] * (c[i] - a[i]);
    }

    double rgbd[4];
    
    // Pass varying vector to the fragment shader
    sha->shadeFragment(sha->unifDim, unif, sha->texNum, tex, sha->varyDim, vary, rgbd);

    // Do not draw the pixel if it is occluded by another
    if (rgbd[3] < depthGetDepth(buf, x, y)) {
        pixSetRGB(x, y, rgbd[0], rgbd[1], rgbd[2]);
        depthSetDepth(buf, x, y, rgbd[3]);
    }
}

/* Renders a triangle assuming that A is the leftmost point */
void triRenderALeft( 
        const shaShading *sha, depthBuffer *buf, const double unif[], const texTexture *tex[], 
        const double a[], const double b[], const double c[]) {
    
    // Check if the triangle is facing the camera and do not render it if it is not.
    double m[2][2] = {
        {b[0] - a[0], c[0] - a[0]},
        {b[1] - a[1], c[1] - a[1]}};
    double mInverse[2][2];
    if (mat22Invert(m, mInverse) <= 0) {
        return;
    }

    double finalRgb[3];

    /* This means C is in between A and B, horizontally */
    if (c[0] <= b[0]) {
        /* Check and account for vertical lines in the triangle */
        if (c[0] == a[0]) {
            for (int x = max(0, ceil(a[0])); x <= min(buf->width, floor(b[0])); x++) {
                for (int y = getYLoopStart(a, b, x); y <= getYLoopEnd(buf->height, b, c, x); y++) {
                        findPixelColor(sha, buf, unif, tex, a, b, c, mInverse, x, y);
                }
            }
        }
        else if (c[0] == b[0]) {
            for (int x = max(0, ceil(a[0])); x <= floor(c[0]); x++) {
                for (int y = getYLoopStart(a, b, x); y <= getYLoopEnd(buf-> height, a, c, x); y++) {
                        findPixelColor(sha, buf, unif, tex, a, b, c, mInverse, x, y);
                }
            }
        }
        /* Triangle has no vertical lines, so it must be rendered in halves */
        else {
            for (int x = max(0, ceil(a[0])); x <= min(buf->width, floor(c[0])); x++) {
                for (int y = getYLoopStart(a, b, x); y <= getYLoopEnd(buf-> height, a, c, x); y++) {
                        findPixelColor(sha, buf, unif, tex, a, b, c, mInverse, x, y);
                }
            }
            for (int x = max(0, floor(c[0]) + 1); min(buf->width, x <= floor(b[0])); x++) {
                for (int y = getYLoopStart(a, b, x); y <= getYLoopEnd(buf-> height, b, c, x); y++) {
                        findPixelColor(sha, buf, unif, tex, a, b, c, mInverse, x, y);
                }
            }
        } 
    }
    /* B is in between A and C */
    else  {
        /* Check and account for vertical lines in the triangle */
        if (a[0] == b[0]) {
            for (int x = max(0, ceil(a[0])); x <= min(buf->width, floor(c[0])); x++) {
                for (int y = getYLoopStart(c, b, x); y <= getYLoopEnd(buf-> height, a, c, x); y++) {
                        findPixelColor(sha, buf, unif, tex, a, b, c, mInverse, x, y);
                }
            }
        }
        /* Triangle has no vertical lines, so it must be rendered in halves */
        else {
            for (int x = max(0, ceil(a[0])); x <= min(buf->width, floor(b[0])); x++) {
                for (int y = getYLoopStart(a, b, x); y <= getYLoopEnd(buf-> height, a, c, x); y++) {
                        findPixelColor(sha, buf, unif, tex, a, b, c, mInverse, x, y);
                }
            }
            for (int x = max(0, floor(b[0])) + 1; x <= min(buf->width, floor(c[0])); x++) {
                for (int y = getYLoopStart(c, b, x); y <= getYLoopEnd(buf-> height, a, c, x); y++) {
                        findPixelColor(sha, buf, unif, tex, a, b, c, mInverse, x, y);
                }
            }
        } 
    }
}

/* Assumes that the 0th and 1th elements of a, b, c are the 'x' and 'y' 
coordinates of the vertices, respectively (used in rasterization, and to 
interpolate the other elements of a, b, c). */
void triRender(
        const shaShading *sha, depthBuffer *buf, const double unif[], const texTexture *tex[], 
        const double a[], const double b[], const double c[]) {

    /* reorganize point labeling and associated colors so that the triangle has point A on the left */
    if (a[0] <= b[0] && a[0] <= c[0]) {
        triRenderALeft(sha, buf, unif, tex, a, b, c);
    }
    else if (b[0] <= a[0] && b[0] <= c[0]) {
        triRenderALeft(sha, buf, unif, tex, b, c, a);
    }
    else if (c[0] <= a[0] && c[0] <= b[0]) {
        triRenderALeft(sha, buf, unif, tex, c, a, b);
    }
}

