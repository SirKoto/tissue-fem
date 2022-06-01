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

	bool solve(const SMat& A, const Vec& b, Vec* x);

private:

	Vec m_residual;
	Vec m_dir;
	Vec m_Adir;
	Vec m_A_res_precond;

	void apply_jacobi_precond(const SMat& A, const Vec&b, Vec* x) const;

}; // class ConjugateGradient

} // namespace sim