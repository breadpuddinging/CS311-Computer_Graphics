// Nathaniel Li

typedef struct shaShading shaShading;

struct shaShading {
    int unifDim;
    int attrDim;
    int texNum;
    int varyDim;
    void (*shadeVertex) (int unifDim, const double unif[], int attrDim, const double attr[], 
        int varyDim, double vary[]);
    void (*shadeFragment) (int unifDim, const double unif[], int texNum, const texTexture *tex[], 
        int varyDim, const double vary[], double rgbd[4]);
};