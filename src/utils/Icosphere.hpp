#pragma once

#include <vector>

namespace icosphere {

	// Generate the vertices and normals of an icosphere
	// centered at the origin, with top/botom-most vertices (0,+-1,0)
	// Returns: [x,y,z,nx,ny,nz] x 3 per face
	// The normals are averaged for smoth interpolation in shading
	std::vector<float> genIcosphere();

}