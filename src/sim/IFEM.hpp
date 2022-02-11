#pragma once

namespace sim
{

typedef float Float;

class IFEM {
public:

	virtual void step(Float dt) = 0;

};

} // namespace sim