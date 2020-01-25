uniform bool isCameraMoving;
uniform bool useEnvMap;
uniform vec3 randomVector;
uniform vec2 screenResolution;
uniform float hdrTexSize;
uniform int tileX;
uniform int tileY;
uniform float invTileWidth;
uniform float invTileHeight;

uniform sampler2D accumTexture;
uniform isampler2D BVH;
uniform sampler2D BBoxMin;
uniform sampler2D BBoxMax;
uniform isampler2D vertexIndicesTex;
uniform sampler2D verticesTex;
uniform sampler2D normalsTex;
uniform sampler2D materialsTex;
uniform sampler2D transformsTex;
uniform sampler2D lightsTex;
uniform sampler2DArray textureMapsArrayTex;

uniform sampler2D hdrTex;
uniform sampler2D hdrMarginalDistTex;
uniform sampler2D hdrCondDistTex;
uniform float hdrResolution;
uniform float hdrMultiplier;

uniform int numOfLights;
uniform int maxDepth;
uniform int topBVHIndex;
uniform int vertIndicesSize;