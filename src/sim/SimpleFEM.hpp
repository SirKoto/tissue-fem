#pragma once

#include "IFEM.hpp"

#include <Eigen/Sparse>
#include <Eigen/Dense>

#include "GameObject.hpp"


namespace sim {

class SimpleFem : public IFEM {
public:

	SimpleFem(std::shared_ptr<GameObject> obj);

	void step(Float dt) override final;

	void update_objects() override final;


private:

	const std::shared_ptr<GameObject> m_obj;

	const Float m_node_mass = 1.0f;
	const Float m_gravity = 9.8f;
	const Float m_mu = 0.0f;
	const Float m_lambda = 0.0f;

	Vec m_delta_v;
	Vec m_v;
	Vec m_rhs;
	SMat m_dfdx_system;

	std::vector<Mat3> m_DmInvs;

	std::vector<Float> m_volumes;

	std::vector<Eigen::Vector4i> m_elements;
	std::vector<Vec3> m_nodes;

	Mat9 hessian_BW08(const Mat3& F) const;

};

} // namespace sim