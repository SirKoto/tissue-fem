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


}

void SimpleFem::step(Float dt)
{
	SMat dfdx_system(3 * m_nodes.size(), 3 * m_nodes.size());

	for (size_t i = 0; i < m_elements.size(); ++i) {
		const Mat3 F = compute_Ds(m_elements[i], m_nodes) * m_DmInvs[i];
		const Mat9 dfdx = m_volumes[i] * hessian_BW08(F);
		for (uint32_t j = 0; j < 4; ++j) {
			for (uint32_t k = j + 1; k < 4; ++j) {
				//TODO
			}
		}

	}
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