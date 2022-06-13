#include "ConjugateGradient.hpp"

namespace sim {

ConjugateGradient::ConjugateGradient(size_t size)
{
	this->resize(size);
}

void ConjugateGradient::resize(size_t size)
{
	m_residual.resize((Eigen::Index)size);
	m_dir.resize((Eigen::Index)size);
	m_Adir.resize((Eigen::Index)size);
	m_A_res_precond.resize((Eigen::Index)size);
	m_jacobi_precond.resize((Eigen::Index)size);
}

bool ConjugateGradient::solve(const SMat& A, const Vec& b, Vec* x_)
{
	assert(x_ != nullptr);
	Vec& x = *x_;
	assert(A.rows() == m_residual.rows());
	assert(A.cols() == m_residual.rows());
	assert(b.rows() == m_residual.rows());
	assert(x.rows() == m_residual.rows());
	const Float max_error = Float(1e-4);
	const uint32_t max_iterations = (uint32_t)m_residual.rows();

	init_jacobi_precond(A);

	m_residual = b - A * x;
	

	// The first direction given by preconditioned matrix
	// We will build A-orthonormal directions from this
	apply_jacobi_precond(m_residual, &m_dir);
	Float delta = m_residual.dot(m_dir);
	const Float delta_zero = delta;

	uint32_t it = 0;
	while (it++ < max_iterations) {
		m_Adir.noalias() = A * m_dir;
		Float alpha = delta / (m_dir.dot(m_Adir));
		x += alpha * m_dir;

		// The resudual is recomputed by accumulation
		// This can accumulate error.
		// If problems arise, use r = b - Ax
		m_residual -= alpha * m_Adir;

		Float sqNorm = m_residual.squaredNorm();
		if (sqNorm < max_error) {
			break;
		}

		apply_jacobi_precond(m_residual, &m_A_res_precond);

		Float newDelta = m_residual.dot(m_A_res_precond);

		// Gram-Schmidt A-orthonormal new direction
		Float beta = newDelta / delta;
		m_dir = m_A_res_precond + beta * m_dir;

		delta = newDelta;
	}

	return it <= max_iterations;
}

void ConjugateGradient::apply_jacobi_precond(const Vec& b, Vec* x_) const
{
	assert(x_ != nullptr);
	Vec& x = *x_;
	assert(b.rows() == m_residual.rows());
	assert(x.rows() == m_residual.rows());

	x = m_jacobi_precond.cwiseProduct(b);
}

void ConjugateGradient::init_jacobi_precond(const SMat& A)
{
	for (Eigen::Index i = 0; i < A.rows(); ++i) {
		if (A.diagonal()(i) != Float(0)) {
			m_jacobi_precond(i) = (Float(1) / A.diagonal()(i));
		}
		else {
			m_jacobi_precond(i) = Float(1.0);
		}
	}
}

} // namespace sim