
struct Vertex {
  vec3 vPos;
  vec3 vNormal;
  vec4 vTangent;
  vec2 vTexcoord;
};

struct Material {
  mat4 modelMatrix;
  vec4 baseColorFactor;
  vec3 emissiveFactor;
  vec3 envFactor;
  vec3 mrFactor;
  vec4 intensity;
  int  textureUseSetting[4];
  int  textureIndices[1024];
};

struct ObjDesc {
  uint64_t vertexAddress;
  uint64_t indexAddress;
  uint64_t materialAddress;
  uint64_t primitiveMatIdxAddress;
};

struct hitPayLoad {
  uint  seed;
  int   envSample;
  int   recursiveDepth;
  vec3  lastNormal;
  vec3  origin;
  vec3  direction;
  vec3  weight;
  vec3  hitValue;
  vec3  brdf;
  float pdf;
};
