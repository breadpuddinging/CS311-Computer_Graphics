// Nathaniel


/* On macOS, compile with...
    clang 740mainMeshes.c 040pixel.o -lglfw -framework OpenGL -framework Cocoa -framework IOKit
On Ubuntu, compile with...
    cc 640mainSpheres.c 040pixel.o -lglfw -lGL -lm -ldl
*/
#include <stdio.h>
#include <math.h>
#include <GLFW/glfw3.h>
#include "040pixel.h"

#include "650vector.c"
#include "280matrix.c"
#include "300isometry.c"
#include "300camera.c"
#include "730ray.c"
#include "150texture.c"
#include "730sphere.c"
#include "730body.c"
#include "680light.c"
#include "730plane.c"
#include "730mesh.c"
#include "250mesh3D.c"
#include "740resh.c"

#define SCREENWIDTH 512
#define SCREENHEIGHT 512


/*** ARTWORK ******************************************************************/
camCamera camera;
double cameraTarget[3] = {0.0, 0.0, 0.0};
double cameraRho = 10.0, cameraPhi = M_PI / 3.0, cameraTheta = M_PI / 3.0;

/* Meshes */
meshMesh mesh;

/* Bodies */
bodyBody bodies[6];
int bodyNum = 6;

/* Lights */
lightLight lights[2];
int lightNum = 2;
double cAmbient[3] = {0.2, 0.2, 0.2};

/* Textures */
texTexture texture;
texTexture *textures[1] = {&texture};
texTexture **tex = textures;
int texNum = 1;

void getDirectionalLighting(
        int unifDim, const double unif[], const isoIsometry *isometry, 
        const double x[3], lightLighting *lighting) {
    vecCopy(unifDim, unif, lighting->cLight);
    lighting->distance = rayINFINITY;
    double unit[3] = {0, 0, 1};
    isoRotateDirection(isometry, unit, lighting->uLight);
}

void getPositionalLighting(
        int unifDim, const double unif[], const isoIsometry *isometry, 
        const double x[3], lightLighting *lighting) {
    vecCopy(unifDim, unif, lighting->cLight);
    double dLight[3];
    vecSubtract(3, isometry->translation, x, dLight);
    vecUnit(3, dLight, lighting->uLight);
    lighting->distance = vecLength(3, dLight);
}

/* Based on the uniforms, textures, rayIntersection, and texture coordinates, 
outputs a material. */
void getPhongMaterial(
        int unifDim, const double unif[], const void *materData, int texNum, const texTexture *tex[], 
        const rayIntersection *inter, const double texCoords[2], 
        rayMaterial *material) {
    material->hasAmbient = 1;
    material->hasDiffuse = 1;
    material->hasSpecular = 1;
    material->hasMirror = 0;
    vecCopy(3, unif, material->cSpecular);
    material->shininess = unif[3];
    texSample(tex[0], texCoords[0], texCoords[1], material->cDiffuse);
}

void getMirrorMaterial(
        int unifDim, const double unif[], const void *materData, int texNum, const texTexture *tex[], 
        const rayIntersection *inter, const double texCoords[2], 
        rayMaterial *material) {
    double white[3] = {1, 1, 1};
    material->hasAmbient = 0;
    material->hasDiffuse = 0;
    material->hasSpecular = 0;
    material->hasMirror = 1;
    vecCopy(3, white, material->cMirror);
}

int initializeArtwork(void) {
    double cSpecular[4] = {1, 1, 1, 64}; // R, G, B, Shininess
    camSetProjectionType(&camera, camPERSPECTIVE);
    camSetFrustum(
        &camera, M_PI / 6.0, cameraRho, 10.0, SCREENWIDTH, SCREENHEIGHT);
    camLookAt(&camera, cameraTarget, cameraRho, cameraPhi, cameraTheta);
    /* Textures */
    if (texInitializeFile(&texture, "jupiter.jpg") != 0) {
        texFinalize(&texture);
        return 1;
    }

    /* Bodies */
    double transl[3];
    double rot[3][3] = {{1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}};

    /*    Mesh */
    if (mesh3DInitializeCylinder(&mesh, .2, 1, 5) != 0) {
        meshFinalize(&mesh);
        return 9;
    }
    if (bodyInitialize(&bodies[5], 0, 4, texNum, &reshGetIntersection, &reshGetTexCoordsAndNormal, &getPhongMaterial) != 0) {
        bodyFinalize(&bodies[5]);
        texFinalize(&texture);
        return 10;
    }
    bodySetGeometryData(&bodies[5], &mesh);
    bodySetTexture(&bodies[5], 0, &texture);
    isoSetRotation(&(bodies[5].isometry), rot);
    bodySetMaterialUniforms(&bodies[5], 0, cSpecular, 4);
    vec3Set(1.0, 1.0, 0.5, transl);
    isoSetTranslation(&(bodies[5].isometry), transl);

    /*    Spheres */
    if (bodyInitialize(&bodies[0], 1, 4, texNum, &sphGetIntersection, &sphGetTexCoordsAndNormal, &getPhongMaterial) != 0) {
            bodyFinalize(&bodies[0]);
            texFinalize(&texture);
            return 2;
        }
    bodySetTexture(&bodies[0], 0, &texture);
    isoSetRotation(&(bodies[0].isometry), rot);
    bodySetMaterialUniforms(&bodies[0], 0, cSpecular, 4);
    bodies[0].geomUnif[0] = 1;
    vec3Set(0.0, 0.0, 0.0, transl);
    isoSetTranslation(&(bodies[0].isometry), transl);

    if (bodyInitialize(&bodies[1], 1, 4, texNum, &sphGetIntersection, &sphGetTexCoordsAndNormal, &getMirrorMaterial) != 0) {
            bodyFinalize(&bodies[1]);
            texFinalize(&texture);
            return 3;
        }
    bodySetTexture(&bodies[1], 0, &texture);
    isoSetRotation(&(bodies[1].isometry), rot);
    bodySetMaterialUniforms(&bodies[1], 0, cSpecular, 4);
    bodies[1].geomUnif[0] = .5;
    vec3Set(2.0, 0.0, 0.0, transl);
    isoSetTranslation(&(bodies[1].isometry), transl);

    if (bodyInitialize(&bodies[2], 1, 4, texNum, &sphGetIntersection, &sphGetTexCoordsAndNormal, &getMirrorMaterial) != 0) {
            bodyFinalize(&bodies[2]);
            texFinalize(&texture);
            return 4;
        }
    bodySetTexture(&bodies[2], 0, &texture);
    isoSetRotation(&(bodies[2].isometry), rot);
    bodySetMaterialUniforms(&bodies[2], 0, cSpecular, 4);
    bodies[2].geomUnif[0] = .5;
    vec3Set(0.0, 2.0, 0.0, transl);
    isoSetTranslation(&(bodies[2].isometry), transl);

    if (bodyInitialize(&bodies[3], 1, 4, texNum, &sphGetIntersection, &sphGetTexCoordsAndNormal, &getPhongMaterial) != 0) {
            bodyFinalize(&bodies[3]);
            texFinalize(&texture);
            return 5;
        }
    bodySetTexture(&bodies[3], 0, &texture);
    isoSetRotation(&(bodies[3].isometry), rot);
    bodySetMaterialUniforms(&bodies[3], 0, cSpecular, 4);
    bodies[3].geomUnif[0] = .5;
    vec3Set(0.0, 0.0, 2.0, transl);
    isoSetTranslation(&(bodies[3].isometry), transl);

    /*    Plane */
    if (bodyInitialize(&bodies[4], 0, 4, texNum, &plaGetIntersection, &plaGetTexCoordsAndNormal, &getPhongMaterial) != 0) {
        bodyFinalize(&bodies[4]);
        texFinalize(&texture);
        return 6;
    }
    bodySetTexture(&bodies[4], 0, &texture);
    isoSetRotation(&(bodies[4].isometry), rot);
    bodySetMaterialUniforms(&bodies[4], 0, cSpecular, 4);
    vec3Set(0.0, 0.0, -1.0, transl);
    isoSetTranslation(&(bodies[4].isometry), transl);

    /* Lights */
    double cLight[3] = {1, 1, 1};
    if (lightInitialize(&lights[0], 3, &getDirectionalLighting) != 0) {
        lightFinalize(&lights[0]);
        return 7;
    }
    isoSetRotation(&lights[0].isometry, rot);
    lightSetUniforms(&lights[0], 0, cLight, 3);

    double pLight[3] = {5, 0, 1};
    if (lightInitialize(&lights[1], 3, &getPositionalLighting) != 0) {
        lightFinalize(&lights[1]);
        return 8;
    }
    lightSetUniforms(&lights[1], 0, cLight, 3);
    isoSetTranslation(&lights[1].isometry, pLight);

    return 0;
}

void finalizeArtwork(void) {
    for (int i = 0; i < lightNum; i++) {
        lightFinalize(&lights[i]);
    }
    for (int i = 0; i < bodyNum; i++) {
        bodyFinalize(&bodies[i]);
    }
    meshFinalize(&mesh);
    texFinalize(&texture);
    return;
}



/*** RENDERING ****************************************************************/
/* Casts the ray x(t) = p + t d into the scene. Returns 0 if it hits no body or 
1 if it hits any body. Used to determine whether a fragment is in shadow. */
int getSceneShadow(
        int bodyNum, const bodyBody bodies[], const double p[3], 
        const double d[3]) {
    rayIntersection inter;
    for (int i = 0; i < bodyNum; i++) {
        bodyGetIntersection(&bodies[i], p, d, rayINFINITY, &inter);
        if (inter.t > rayEPSILON && inter.t < rayINFINITY) {
            return 1;
        }
    }
    return 0;
}

/* Given a ray x(t) = p + t d. Finds the color where that ray hits the scene (or 
the background) and loads the color into the rgb parameter. */
void getSceneColor(
        int recDepth, int bodyNum, const bodyBody bodies[], const double cAmbient[3], 
        int lightNum, const lightLight lights[], const double p[3], 
        const double d[3], double rgb[3]) {
    double bound = rayINFINITY;
    int interBodyInd = -1;
    rayIntersection inter;
    rayIntersection winningInter;
    // Check for intersections with every body and return the color of the closest intersected one
    for (int i = 0; i < bodyNum; i++) {
        bodyGetIntersection(&bodies[i], p, d, rayINFINITY, &inter);
        if (inter.t > rayEPSILON && inter.t < bound) {
            bound = inter.t;
            interBodyInd = i;
            winningInter = inter;
        }
    }

    // Clear the rgb value from last time. Also serves to set the background color to black
    double black[3] = {0, 0, 0};
    vecCopy(3, black, rgb);
    if (interBodyInd != -1) {
        double texCoords[2];
        double uNormal[3];
        double sample[3];
        rayMaterial mat;
        double uCam[3];
        vecScale(3, -1, d, uCam);
        vecUnit(3, uCam, uCam);

        bodyGetTexCoordsAndNormal(&bodies[interBodyInd], p, d, &winningInter, texCoords, uNormal);
        bodyGetMaterial(&bodies[interBodyInd], &winningInter, texCoords, &mat);
        
        lightLighting lighting;
        double x[3];
        vecScale(3, winningInter.t, d, x);
        vecAdd(3, p, x, x);
        // Diffuse and specular lighting
        double diffIntensity;
        double specIntensity;
        double diffuse[3];
        double specular[3];
        double mirror[3];
        // If the material has either diffuse or specular, loop through all the lights
        if (mat.hasDiffuse || mat.hasSpecular) {
            for (int i = 0; i < lightNum; i++) {
                lightGetLighting(&lights[i], x, &lighting);
                // If the fragment is not shadowed, calculate lighting
                if (getSceneShadow(bodyNum, bodies, x, lighting.uLight) == 0) {
                    diffIntensity = vecDot(3, uNormal, lighting.uLight);
                    if (diffIntensity < 0) {
                        diffIntensity = 0;
                    }
                    if (mat.hasDiffuse) {
                        vecModulate(3, lighting.cLight, mat.cDiffuse, diffuse);
                        vecScale(3, diffIntensity, diffuse, diffuse);
                        vecAdd(3, diffuse, rgb, rgb);
                    }
                    if (mat.hasSpecular) {
                        if (diffIntensity <= 0) {
                            specIntensity = 0;
                        } else {
                            double uReflectedDirectional[3];
                            vecScale(3, 2 * vecDot(3, uNormal, lighting.uLight), uNormal, uReflectedDirectional);
                            vecSubtract(3, uReflectedDirectional, lighting.uLight, uReflectedDirectional);
                            vecUnit(3, uReflectedDirectional, uReflectedDirectional);
                            specIntensity = pow(vecDot(3, uReflectedDirectional, uCam), mat.shininess);
                            if (specIntensity < 0) {
                                specIntensity = 0;
                            }
                            vecModulate(3, lighting.cLight, mat.cSpecular, specular);
                            vecScale(3, specIntensity, specular, specular);
                            vecAdd(3, specular, rgb, rgb);
                        }
                    }
                }
            }
        } 
        // Mirroring
        if (mat.hasMirror && recDepth > 0) {
            double dReflected[3];
            vecScale(3, -2 * vecDot(3, uNormal, d), uNormal, dReflected);
            vecSubtract(3, dReflected, d, dReflected);
            getSceneColor(recDepth - 1, bodyNum, bodies, cAmbient, lightNum, lights, x, dReflected, mirror);
            vecModulate(3, mat.cMirror, mirror, mirror);
            vecAdd(3, mirror, rgb, rgb);
        }
        // Ambient lighting
        if (mat.hasAmbient) {
            double ambient[3];
            vecModulate(3, cAmbient, mat.cDiffuse, ambient);
            vecAdd(3, ambient, rgb, rgb);
        }
    }
}



void render(void) {
    /* Build a 4x4 matrix that (along with homogeneous division) takes screen 
    coordinates (x0, x1, 0, 1) to the corresponding world coordinates. */
    double viewInv[4][4], projInv[4][4], camTrans[4][4], pvInv[4][4], transform[4][4];
    mat44InverseViewport(SCREENWIDTH, SCREENHEIGHT, viewInv);
    if (camera.projectionType == camORTHOGRAPHIC) {
        camGetInverseOrthographic(&camera, projInv);
    } else {
        camGetInversePerspective(&camera, projInv);
    }
    isoGetHomogeneous(&camera.isometry, camTrans);
    mat444Multiply(projInv, viewInv, pvInv);
    mat444Multiply(camTrans, pvInv, transform);

    double p[4], d[3];
    if (camera.projectionType == camORTHOGRAPHIC) {
        double camDir[3] = {0, 0, -1};
        mat331Multiply(camera.isometry.rotation, camDir, d);
    }
    /* Each screen point is chosen to be on the near plane. */
    double screen[4] = {0.0, 0.0, 0.0, 1.0};
    for (int i = 0; i < SCREENWIDTH; i += 1) {
        screen[0] = i;
        for (int j = 0; j < SCREENHEIGHT; j += 1) {
            screen[1] = j;
            mat441Multiply(transform, screen, p);
            vecScale(4, 1/p[3], p, p);
            if (camera.projectionType == camPERSPECTIVE){
                vecSubtract(3, p, camera.isometry.translation, d);
            }
            /* Set the pixel to the color of that ray. */
            double rgb[3];
            getSceneColor(3, bodyNum, bodies, cAmbient, lightNum, lights, p, d, rgb);
            pixSetRGB(i, j, rgb[0], rgb[1], rgb[2]);
        }
    }
}



/*** USER INTERFACE ***********************************************************/

void handleKey(
        int key, int shiftIsDown, int controlIsDown, int altOptionIsDown, 
        int superCommandIsDown) {
    if (key == GLFW_KEY_I)
        cameraPhi -= 0.1;
    else if (key == GLFW_KEY_K)
        cameraPhi += 0.1;
    else if (key == GLFW_KEY_J)
        cameraTheta -= 0.1;
    else if (key == GLFW_KEY_L)
        cameraTheta += 0.1;
    else if (key == GLFW_KEY_U)
        cameraRho *= 1.1;
    else if (key == GLFW_KEY_O)
        cameraRho *= 0.9;
    else if (key == GLFW_KEY_P) {
        if (camera.projectionType == camORTHOGRAPHIC)
            camSetProjectionType(&camera, camPERSPECTIVE);
        else
            camSetProjectionType(&camera, camORTHOGRAPHIC);
    }
    camSetFrustum(
        &camera, M_PI / 6.0, cameraRho, 10.0, SCREENWIDTH, SCREENHEIGHT);
    camLookAt(&camera, cameraTarget, cameraRho, cameraPhi, cameraTheta);
}

void handleTimeStep(double oldTime, double newTime) {
    if (floor(newTime) - floor(oldTime) >= 1.0)
        printf(
            "info: handleTimeStep: %f frames/s\n", 1.0 / (newTime - oldTime));
    double rotAxis[3] = {1.0 / sqrt(3.0), 1.0 / sqrt(3.0), 1.0 / sqrt(3.0)};
    double rotMatrix[3][3];
    mat33AngleAxisRotation(newTime, rotAxis, rotMatrix);
    for (int k = 0; k < bodyNum - 2; k += 1)
        isoSetRotation(&(bodies[k].isometry), rotMatrix);
    render();
}

int main(void) {
    if (pixInitialize(SCREENWIDTH, SCREENHEIGHT, "640mainSpheres") != 0)
        return 1;
    if (initializeArtwork() != 0) {
        pixFinalize();
        return 2;
    }
    pixSetKeyDownHandler(handleKey);
    pixSetKeyRepeatHandler(handleKey);
    pixSetTimeStepHandler(handleTimeStep);
    pixRun();
    finalizeArtwork();
    pixFinalize();
    return 0;
}


