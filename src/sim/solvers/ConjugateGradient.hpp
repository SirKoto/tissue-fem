#pragma once

#include <Eigen/Sparse>
#include <Eigen/Dense>

#include "sim/IFEM.hpp"

namespace sim {

class ConjugateGradient {
public:

	ConjugateGradient() = default;
	ConjugateGradient(size_t size);

	void resize(size_t size);

	void solve(const SMat& A, const Vec& b, Vec* x);

private:

	Vec m_residual;
	Vec m_dir;
	Vec m_Adir;

}; // class ConjugateGradient

} // namespace sim