#pragma once

#include "IFEM.hpp"

#include <Eigen/Sparse>
#include <Eigen/Dense>

#include <array>

#include "GameObject.hpp"
#include "solvers/ConjugateGradient.hpp"



namespace sim {

class SimpleFem : public IFEM {
public:

	SimpleFem(std::shared_ptr<GameObject> obj, Float young, Float nu);

	void step(Float dt) override final;

	void update_objects() override final;

	void add_constraint(uint32_t node, const glm::vec3& v) override final;

	void clear_constraints() override final { m_z.setZero();  m_constraints.setOnes(); }

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
	Vec m_constraints;
	Vec m_Sc;
	SVec m_z;

	std::vector<Mat3> m_DmInvs;

	std::vector<Float> m_volumes;

	std::vector<Eigen::Vector4i> m_elements;
	std::vector<Vec3> m_nodes;

	ConjugateGradient solver;

	struct hash_pair {
		template <class T1, class T2>
		size_t operator()(const std::pair<T1, T2>& p) const
		{
			auto hash1 = std::hash<T1>{}(p.first);
			auto hash2 = std::hash<T2>{}(p.second);
			return hash1 ^ hash2;
		}
	};

	// sparse_cache has pointers to the 3x3 region of the sparse matrix where node i
	// determines node j. In particular, it stores the 3 pointers to the first elements
	// of the 3 columns of the 3x3 region.
	typedef std::array<Float*, 3> SMatPtrs;
	std::unordered_map<std::pair<uint32_t, uint32_t>, SMatPtrs, hash_pair> m_sparse_cache;


	void build_sparse_system();
	void assign_sparse_block(const Eigen::Block<const Mat12, 3, 3>& m, uint32_t i, uint32_t j);
	void set_system_to_zero();

	// Energy model for BW08
	struct BW08_Data {
		Mat3 F;
		Float I3;
		Float logI3;
		Vec9 g3;
		Mat9 H3;

		BW08_Data(const Mat3& F, Float mu, Float lambda);
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

		CoRot_Data(const Mat3& F, Float mu, Float lambda);
	};

	Mat3 compute_pk1(const CoRot_Data& d) const;
	Mat9 compute_hessian(const CoRot_Data& d) const;

	struct HookeanSmith19_Data {
		Mat3 pk1;
		Mat9 hessian;

		HookeanSmith19_Data(const Mat3& F, Float mu, Float lambda);
	};

	const Mat3& compute_pk1(const HookeanSmith19_Data& d) const { return d.pk1; }
	const Mat9& compute_hessian(const HookeanSmith19_Data& d) const { return d.hessian; }


};

} // namespace sim