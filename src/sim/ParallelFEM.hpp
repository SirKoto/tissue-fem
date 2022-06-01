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

class ParallelFEM final : public IFEM {
public:

	ParallelFEM();

	void initialize(const std::vector<const TetMesh*>& meshes) override final;

	void step(Float dt, const Parameters& params) override final;

	void update_objects(TetMesh* mesh,
		uint32_t from_sim_idx, uint32_t to_sim_idx,
		bool add_position_alteration) override final;

	void add_constraint(uint32_t node, const glm::vec3& v,
		const glm::vec3& dir, Float friction) override final;

	void add_constraint(uint32_t node, const glm::vec3& v) override final;

	void erase_constraint(uint32_t node) override final;

	void add_position_alteration(uint32_t node, const glm::vec3& dx) override final;

	void clear_frame_alterations() override final;

	const Vec3& get_node(uint32_t node) const override final;
	Vec3 get_velocity(uint32_t node) const override final;
	Vec3 get_force_constraint(uint32_t node) const override final;

	Float compute_volume() const override final;

private:
	Vec m_delta_v;
	Vec m_v;
	Vec m_rhs;
	SMat m_dfdx_system;
	SMat m_system;
	struct Constraint;
	std::map<uint32_t, Constraint> m_constraints3;
	Vec m_Sc;
	Vec m_constraint_forces;
	Vec m_z;
	SVec m_position_alteration;

	std::vector<Mat3> m_DmInvs;

	std::vector<Float> m_volumes;


	std::vector<Eigen::Vector4i> m_elements;
	std::vector<Vec3> m_nodes;

	ConjugateGradient m_cg_solver;

	struct hash_pair {
		template <class T1, class T2>
		size_t operator()(const std::pair<T1, T2>& p) const
		{
			auto hash1 = std::hash<T1>{}(p.first);
			auto hash2 = std::hash<T2>{}(p.second);
			return hash1 ^ (hash2 << 1);
		}
	};

	struct Constraint {
		Vec3 dir;
		Mat3 constraint;
		Float friction = Float(0);
	};

	// sparse_cache has pointers to the 3x3 region of the sparse matrix where node i
	// determines node j. In particular, it stores the 3 pointers to the first elements
	// of the 3 columns of the 3x3 region.
	typedef std::array<Float*, 3> SMatPtrs;
	std::unordered_map<std::pair<uint32_t, uint32_t>, SMatPtrs, hash_pair> m_sparse_cache;


	void build_sparse_system();

	template<typename T>
	void assign_sparse_block(const Eigen::Block<const T, 3, 3>& m, uint32_t i, uint32_t j);

	void set_system_to_zero();

};

} // namespace sim