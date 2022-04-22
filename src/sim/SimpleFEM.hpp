#pragma once

#include "IFEM.hpp"

#include <Eigen/Sparse>
#include <Eigen/Dense>

#include <array>
#include <memory>

#include "GameObject.hpp"
#include "meshes/TetMesh.hpp"
#include "solvers/ConjugateGradient.hpp"



namespace sim {

class SimpleFem final : public IFEM {
public:

	SimpleFem(const std::shared_ptr<TetMesh>& mesh, Float young, Float nu);

	void step(Float dt) override final;

	void update_objects() override final;
	
	void add_constraint(uint32_t node, const glm::vec3& v, const glm::vec3& dir) override final;

	void add_constraint(uint32_t node, const glm::vec3& v) override final;

	void add_position_alteration(uint32_t node, const glm::vec3& dx) override final;

	void clear_constraints() override final;

	const Vec3& get_node(uint32_t node) const override final;
	Vec3 get_velocity(uint32_t node) const override final;

	void pancake();

	void draw_ui() override final;

private:

	std::weak_ptr<TetMesh> m_mesh;

	Float m_young;
	Float m_nu;

	Float m_node_mass = 1.0f;
	Float m_gravity = 9.8f;
	Float m_mu = 0.0f;
	Float m_lambda = 0.0f;
	Float m_alpha_rayleigh = 0.01f;
	Float m_beta_rayleigh = 0.001f;

	Vec m_delta_v;
	Vec m_v;
	Vec m_rhs;
	SMat m_dfdx_system;
	SMat m_system;
	struct Constraint;
	std::vector<Constraint> m_constraints3;
	SMat m_S;
	Vec m_Sc;
	SVec m_z;
	SVec m_position_alteration;

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

	struct Constraint {
		uint32_t node;
		Mat3 constraint;

		bool operator<(const Constraint& o) const {
			return this->node < o.node;
		}
	};

	// sparse_cache has pointers to the 3x3 region of the sparse matrix where node i
	// determines node j. In particular, it stores the 3 pointers to the first elements
	// of the 3 columns of the 3x3 region.
	typedef std::array<Float*, 3> SMatPtrs;
	std::unordered_map<std::pair<uint32_t, uint32_t>, SMatPtrs, hash_pair> m_sparse_cache;


	enum class EnergyFunction {
		HookeanSmith19 = 0,
		Corrotational = 1,
		HookeanBW08 = 2,
	};
	EnergyFunction m_enum_energy = EnergyFunction::HookeanSmith19;

	void build_sparse_system();

	
	void assign_sparse_block(const Eigen::Block<const Mat12, 3, 3>& m, uint32_t i, uint32_t j);

	void set_system_to_zero();

	void update_lame();

};

} // namespace sim