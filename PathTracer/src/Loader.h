#ifndef __LOADER_H_
#define __LOADER_H_


#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <Scene.h>

bool LoadModel(Scene *scene, std::string filename, float materialId);
bool LoadScene(Scene *scene, const char* filename);

/*struct Vertex : public Vec3f
{
	Vec3f _normal;
	// ambient occlusion of this vertex (pre-calculated in e.g. MeshLab)

	Vertex(float x, float y, float z, float nx, float ny, float nz, float amb = 60.f)
		:
		Vec3f(x, y, z), _normal(Vec3f(nx, ny, nz))
	{
		// assert |nx,ny,nz| = 1
	}
};

struct Triangle {
	// indexes in vertices array
	unsigned _idx1;
	unsigned _idx2;
	unsigned _idx3;
};

using std::string;

struct face {
	std::vector<int> vertex;
	std::vector<int> texture;
	std::vector<int> normal;
};

extern std::vector<face> faces;

namespace enums {
	enum ColorComponent {
		Red = 0,
		Green = 1,
		Blue = 2
	};
}

using namespace enums;

// Rescale input objects to have this size...
const float MaxCoordAfterRescale = 1.2f;

struct TriangleMesh
{
	std::vector<Vec3f> verts;
	std::vector<Vec3i> faces;
	Vec3f bounding_box[2];   // mesh bounding box
};

extern unsigned verticesNo;
extern Vertex* vertices;
extern unsigned int trianglesNo;
extern Triangle* triangles;

void load_object(const char *filename);*/

#endif