#include "SimpleFEM.hpp"

#include <iostream>
#include <glm/gtc/constants.hpp>

namespace sim {
SimpleFem::SimpleFem(std::shared_ptr<GameObject> obj, Float young, Float nu) :
	m_obj(obj),
	m_mu(young / (2 + 2 * nu)),
	m_lambda(young * nu / ((1 + nu) * (1 - 2 * nu)))
{
	m_elements = m_obj->get_mesh().elements();
	m_nodes.resize(m_obj->get_mesh().nodes().size());
	for (size_t i = 0; i < m_nodes.size(); ++i) {
		m_nodes[i] = m_obj->get_mesh().nodes()[i].cast<Float>();
	}

	m_DmInvs.resize(m_elements.size());
	m_volumes.resize(m_elements.size());
	for (size_t i = 0; i < m_elements.size(); ++i) {
		const Mat3 Ds = compute_Ds(m_elements[i], m_nodes);
		m_volumes[i] = std::abs(Ds.determinant()) / Float(6.0);
		m_DmInvs[i] = Ds.inverse();
	}


	m_dfdx_system = SMat(3 * m_nodes.size(), 3 * m_nodes.size());
	m_dfdx_system.reserve(Eigen::VectorXi::Constant(3 * m_nodes.size(), 3 * 4 * 6));

	m_v.resize(3 * m_nodes.size());
	m_v.setZero();
	m_rhs.resize(3 * m_nodes.size());
}

bool isNull(const SMat& mat, int row, int col)
{
	for (SMat::InnerIterator it(mat, col); it; ++it) {
		if (it.row() == row) return false;
	}
	return true;
}

template<typename Derived>
void assign_sparse_block(const Eigen::Block<const Derived, 3, 3>& m, uint32_t i, uint32_t j, SMat* out) {

	for (uint32_t s = 0; s < 3; ++s) {
		for (uint32_t t = 0; t < 3; ++t) {
			/*if ( i != j && std::abs(m(t, s)) < 1e-4) {
				continue;
			}*/

			/*if (!isNull(*out, i + t, j + s)) {
				/*if (std::abs(out->coeff(i + t, j + s) - m(t, s)) > 1e-4) {
					std::cerr << "In: " << out->coeff(i + t, j + s) << " but trying " << m(t, s) << std::endl;
					assert(false);
				}* /
				// std::cout << "Stored " << out->coeff(i + t, j + s) << " value " << m(t, s) << std::endl;
				out->coeffRef(i + t, j + s) += m(t, s);
			}
			else {
				out->insert(i + t, j + s) = m(t, s);
			}*/
			out->coeffRef(i + t, j + s) += m(t, s);
		}
	}
}

Mat9 SimpleFem::check_eigenvalues_BW08(const Mat3& F) {
	Eigen::JacobiSVD<Mat3> svd(F, Eigen::ComputeFullU | Eigen::ComputeFullV);

	Float sigma0 = svd.singularValues()(0);
	Float sigma1 = svd.singularValues()(1);
	Float sigma2 = svd.singularValues()(2);

	Mat3 U = svd.matrixU();
	Mat3 V = svd.matrixV();
	Mat3 L = Mat3::Identity();
	L(2, 2) = (U * V.transpose()).determinant();
	sigma2 *= L(2, 2);
	Float detU = U.determinant();
	Float detV = V.determinant();
	if (detU < Float(0.0) && detV > Float(0.0)) {
		U = U * L;
	}
	else if (detU > Float(0.0) && detV < Float(0.0)) {
		V = V * L;
	}

	const Float lambda = m_lambda;
	const Float mu = m_mu;
	const Float I3 = compute_I3(F);
	const Float logI3 = std::log(I3);


	Vec9 eigenvalues;
	eigenvalues(0) = (lambda + mu - lambda * logI3) / (sigma0 * sigma0);
	eigenvalues(1) = (lambda + mu - lambda * logI3) / (sigma1 * sigma1);
	eigenvalues(2) = (lambda + mu - lambda * logI3) / (sigma2 * sigma2);
	eigenvalues(3) = (I3 * mu - 2 * mu + 2 * lambda * logI3) / (2 * sigma1 * sigma2);
	eigenvalues(4) = (I3 * mu - 2 * mu + 2 * lambda * logI3) / (2 * sigma0 * sigma1);
	eigenvalues(5) = (I3 * mu - 2 * mu + 2 * lambda * logI3) / (2 * sigma0 * sigma2);
	eigenvalues(6) = -(I3 * mu - 2 * mu + 2 * lambda * logI3) / (2 * sigma0 * sigma1);
	eigenvalues(7) = -(I3 * mu - 2 * mu + 2 * lambda * logI3) / (2 * sigma1 * sigma2);
	eigenvalues(8) = -(I3 * mu - 2 * mu + 2 * lambda * logI3) / (2 * sigma0 * sigma2);

	constexpr Float invSqrt2 = glm::one_over_root_two<Float>();
	Mat9 eigenvectors;
	eigenvectors << 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
		0.0, 0.0, 0.0, invSqrt2 / 2.0, 0.0, 0.0, invSqrt2 / 2.0, 0.0, 0.0,
		0.0, 0.0, 0.0, 0.0, 0.0, -invSqrt2 / 2.0, 0.0, 0.0, invSqrt2 / 2.0,
		0.0, 0.0, 0.0, -invSqrt2 / 2.0, 0.0, 0.0, invSqrt2 / 2.0, 0.0, 0.0,
		0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
		0.0, 0.0, 0.0, 0.0, -invSqrt2 / 2.0, 0.0, 0.0, invSqrt2 / 2.0, 0.0,
		0.0, 0.0, 0.0, 0.0, 0.0, invSqrt2 / 2.0, 0.0, 0.0, invSqrt2 / 2.0,
		0.0, 0.0, 0.0, 0.0, invSqrt2 / 2.0, 0.0, 0.0, invSqrt2 / 2.0, 0.0,
		0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0;

	Mat9 H;
	H.setZero();
	for (uint32_t i = 0; i < 9; ++i) {
		const Mat3 twist = eigenvectors.col(i).reshaped(3, 3);
		const Mat3 rot = U * twist * V;

		H += std::max(eigenvalues(i), Float(9.0)) * (rot.reshaped() * rot.reshaped().transpose());
	}
	
	return H;
}

void SimpleFem::step(Float dt)
{
	m_dfdx_system.setZero();

	for (size_t i = 0; i < m_elements.size(); ++i) {
		const Vec4i& element = m_elements[i];
		const Mat3 F = compute_Ds(element, m_nodes) * m_DmInvs[i];

		const Mat9x12 dFdx = compute_dFdx(m_DmInvs[i]);
		// const Mat9 H = hessian_BW08(F);
		const Mat9 H = check_eigenvalues_BW08(F);
		const Mat12 dfdx = m_volumes[i] * (dFdx.transpose() * H * dFdx);
		
		// Assign the force gradient to the system
		for (uint32_t j = 0; j < 4; ++j) {
			const uint32_t node_j = 3 * element[j];
			// diagonal
			assign_sparse_block(dfdx.block<3, 3>(3 * j, 3 * j), node_j, node_j, &m_dfdx_system);
			// off-diagonal
			for (uint32_t k = j + 1; k < 4; ++k) {
				const uint32_t node_k = 3 * element[k];
				assign_sparse_block(dfdx.block<3, 3>(3 * k, 3 * j), node_k, node_j, &m_dfdx_system);
				assign_sparse_block(dfdx.block<3, 3>(3 * j, 3 * k), node_j, node_k, &m_dfdx_system);
			}
		}

	}

	m_dfdx_system.makeCompressed();

	m_rhs = dt * dt * (m_dfdx_system * m_v);

	// subtract gravity from the y entries
	for (size_t i = 0; i < m_nodes.size(); ++i) {
		m_rhs(3 * i + 1) -= dt * m_node_mass * m_gravity;
	}

	m_dfdx_system *= -dt * dt;
	m_dfdx_system.diagonal().array() += m_node_mass;

	Eigen::ConjugateGradient<SMat> solver(m_dfdx_system);
	solver.setTolerance(1e-4);
	solver.setMaxIterations(20 * m_nodes.size());
	if (solver.info() != Eigen::Success) {
		std::cerr << "Can't build system" << std::endl;
	}
	m_v += solver.solve(m_rhs);
	if (solver.info() != Eigen::Success) {
		std::cerr << "System did not converge" << std::endl;
	}

	for (size_t i = 0; i < m_nodes.size(); ++i) {
		m_nodes[i].x() += dt * m_v[3 * i + 0];
		m_nodes[i].y() += dt * m_v[3 * i + 1];
		m_nodes[i].z() += dt * m_v[3 * i + 2];

		/*if (m_nodes[i].y() < Float(0.0)) {
			m_nodes[i].y() = Float(0.0);
			m_v[3 * i + 1] = Float(0.0);
		}*/
	}
}

void SimpleFem::update_objects()
{
	for (size_t i = 0; i < m_nodes.size(); ++i) {
		m_obj->get_mesh().update_node(i, m_nodes[i].cast<float>());
	}
	m_obj->get_mesh().upload_to_gpu(true, false);
}

void SimpleFem::pancake()
{
	for (size_t i = 0; i < m_nodes.size(); ++i) {
		m_nodes[i].y() *= Float(0.4f);
	}

	update_objects();
}

Mat9 SimpleFem::hessian_BW08(const Mat3& F) const
{
	Vec9 g3 = compute_g3(F);
	Mat9 H3 = compute_H3(F);
	Float I3 = compute_I3(F);
	Float log_I3 = std::log(I3);

	Float g_fact = (m_mu + m_lambda * (Float(1.0) - log_I3)) / (I3 * I3);
	Float H_fact = (m_lambda * log_I3 - m_mu) / I3;

	return m_mu * Mat9::Identity() + g_fact * g3 * g3.transpose() + H_fact * H3;
}

} // namespace sim