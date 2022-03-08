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
}

void ConjugateGradient::solve(const SMat& A, const Vec& b, Vec* x_)
{
	assert(x_ != nullptr);
	Vec& x = *x_;
	assert(A.rows() == m_residual.rows());
	assert(A.cols() == m_residual.rows());
	assert(b.rows() == m_residual.rows());
	assert(x.rows() == m_residual.rows());
	const Float max_error = Float(1e-4);
	const uint32_t max_iterations = m_residual.rows();

	m_residual = b - A * x;
	Float sqNorm = m_residual.squaredNorm();
	if (sqNorm < max_error) {
		return;
	}

	// The first direction is the first residual
	// We will build A-orthonormal directions from this
	m_dir = m_residual;
	uint32_t it = 0;
	while (it++ < max_iterations) {
		m_Adir = A * m_dir;
		Float alpha = sqNorm / (m_residual.dot(m_Adir));
		x += alpha * m_dir;

		m_residual -= alpha * m_Adir;

		Float newSqNorm = m_residual.squaredNorm();
		if (newSqNorm < max_error) {
			break;
		}
		// Gram-Schmidt A-orthonormal new direction
		Float beta = newSqNorm / sqNorm;
		m_dir = m_residual + beta * m_dir;

		sqNorm = newSqNorm;
	}
}

} // namespace sim