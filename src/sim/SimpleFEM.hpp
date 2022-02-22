#pragma once

#include "IFEM.hpp"

#include <Eigen/Sparse>
#include <Eigen/Dense>

#include "GameObject.hpp"


namespace sim {

class SimpleFem : public IFEM {
public:

	SimpleFem(std::shared_ptr<GameObject> obj, Float young, Float nu);

	void step(Float dt) override final;

	void update_objects() override final;

	void pancake();

private:

	const std::shared_ptr<GameObject> m_obj;

	const Float m_node_mass = 1.0f;
	const Float m_gravity = 9.8f;
	const Float m_mu = 0.0f;
	const Float m_lambda = 0.0f;
	const Float m_alpha_rayleigh = 0.01;
	const Float m_beta_rayleigh = 0.001;

	Vec m_delta_v;
	Vec m_v;
	Vec m_rhs;
	SMat m_dfdx_system;

	std::vector<Mat3> m_DmInvs;

	std::vector<Float> m_volumes;

	std::vector<Eigen::Vector4i> m_elements;
	std::vector<Vec3> m_nodes;

	// Energy model for BW08
	struct BW08_Data {
		Mat3 F;
		Float I3;
		Float logI3;
		Vec9 g3;
		Mat9 H3;

		BW08_Data(const Mat3& F);
	};

	Mat3 compute_pk1(const BW08_Data& d) const;
	Mat9 compute_hessian(const BW08_Data& d) const;
	Mat9 check_eigenvalues_BW08(const Mat3& F);

	// Energy model for Corrotational
	struct CoRot_Data {
		Mat3 F;
		Mat3 U;
		Vec3 s;
		Mat3 V;
		Mat3 R;
		Mat3 S;

		CoRot_Data(const Mat3& F);
	};

	Mat3 compute_pk1(const CoRot_Data& d) const;
	Mat9 compute_hessian(const CoRot_Data& d) const;

};

} // namespace sim