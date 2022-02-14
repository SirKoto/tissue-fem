#include "SimpleFEM.hpp"

namespace sim {
SimpleFem::SimpleFem(std::shared_ptr<GameObject> obj) : 
	m_obj(obj)
{
	m_elements = m_obj->get_mesh().elements();
	m_nodes = m_obj->get_mesh().nodes();

	m_DmInvs.resize(m_elements.size());
	m_volumes.resize(m_elements.size());
	for (size_t i = 0; i < m_elements.size(); ++i) {
		const Mat3 Ds = compute_Ds(m_elements[i], m_nodes);
		m_volumes[i] = std::abs(Ds.determinant()) / Float(6.0);
		m_DmInvs[i] = Ds.inverse();
	}


	m_dfdx_system = SMat(3 * m_nodes.size(), 3 * m_nodes.size());
	m_dfdx_system.reserve(Eigen::VectorXi::Constant(3 * m_nodes.size(), 3 * 4 * 6));
}

template<typename Derived>
void assign_sparse_block(const Eigen::Block<const Derived, 3, 3>& m, uint32_t i, uint32_t j, SMat* out) {

	for (uint32_t s = 0; s < 3; ++s) {
		for (uint32_t t = 0; t < 3; ++t) {
			out->insert(i + t, j + s) = m(t, s);
		}
	}
}

void SimpleFem::step(Float dt)
{
	m_dfdx_system.setZero();

	for (size_t i = 0; i < m_elements.size(); ++i) {
		const Vec4i& element = m_elements[i];
		const Mat3 F = compute_Ds(element, m_nodes) * m_DmInvs[i];
		const Mat9 dfdx = m_volumes[i] * hessian_BW08(F);

		// Assign the force gradient to the system
		for (uint32_t j = 0; j < 4; ++j) {
			const uint32_t node_j = 3 * element[j];
			// diagonal
			assign_sparse_block(dfdx.block<3, 3>(3 * j, 3 * j), node_j, node_j, &m_dfdx_system);
			// off-diagonal
			for (uint32_t k = j + 1; k < 4; ++j) {
				const uint32_t node_k = 3 * element[k];
				assign_sparse_block(dfdx.block<3, 3>(3 * k, 3 * j), node_k, node_j, &m_dfdx_system);
				assign_sparse_block(dfdx.block<3, 3>(3 * j, 3 * k), node_j, node_k, &m_dfdx_system);
			}
		}

	}

	m_dfdx_system.makeCompressed();

}

void SimpleFem::update_objects()
{
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