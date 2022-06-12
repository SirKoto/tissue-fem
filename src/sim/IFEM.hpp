#pragma once

#include <Eigen/Dense>
#include <Eigen/Sparse>
#include <glm/glm.hpp>

#include "meshes/TetMesh.hpp"
#include "utils/serialization.hpp"

namespace sim
{

typedef double Float;
typedef Eigen::SparseMatrix<Float> SMat;
typedef Eigen::SparseVector<Float> SVec;
typedef Eigen::Matrix<Float, Eigen::Dynamic, 1> Vec;
typedef Eigen::Matrix<Float, 3, 1> Vec3;
typedef Eigen::Matrix<Float, 9, 1> Vec9;
typedef Eigen::Matrix<Float, 12, 1> Vec12;
typedef Eigen::Vector4i Vec4i;
typedef Eigen::Matrix<Float, 3, 3> Mat3;
typedef Eigen::Matrix<Float, 9, 9> Mat9;
typedef Eigen::Matrix<Float, 12, 12> Mat12;
typedef Eigen::Matrix<Float, 9, 12> Mat9x12;

inline glm::vec3 cast_vec3(const Vec3& v) { return glm::vec3((float)v.x(), (float)v.y(), (float)v.z()); }
inline Vec3 cast_vec3(const glm::vec3& v) { return Vec3(v.x, v.y, v.z); }

Mat9x12 compute_dFdx(const Mat3& DmInv);

inline Mat3 compute_Ds(const Vec4i& element, const std::vector<Vec3>& nodes) {
	Mat3 Ds;
	Ds.col(0) = nodes[element(1)] - nodes[element(0)];
	Ds.col(1) = nodes[element(2)] - nodes[element(0)];
	Ds.col(2) = nodes[element(3)] - nodes[element(0)];

	return Ds;
}

Vec9 vec_slow(const Mat3& m);
Mat3 cross_matrix(const Vec3& v);

inline Float compute_I1(const Mat3& S) { return S.trace(); }
inline Float compute_I2(const Mat3& F) { return (F.transpose() * F).trace(); }
inline Float compute_I3(const Mat3& F) { return F.determinant(); }

inline const Eigen::Reshaped<const Mat3, 9, 1> compute_g1(const Mat3& R) { return R.reshaped(); }
inline const Vec9 compute_g2(const Mat3& F) { return Float(2.0) * F.reshaped(); }
Vec9 compute_g3(const Mat3& F);

Mat9 compute_H1(const Mat3& U, const Vec3& singular_values, const Mat3& V);
inline Mat9 compute_H2() { return Float(2.0) * Mat9::Identity(); }
Mat9 compute_H3(const Mat3& F);

class Parameters;
class IFEM {
public:

	virtual void step(Float dt, const Parameters& params) = 0;

	virtual void initialize(const std::vector<const TetMesh*>& meshes) = 0;

	virtual void update_objects(TetMesh* mesh,
		uint32_t from_sim_idx, uint32_t to_sim_idx,
		bool add_position_alteration = false) = 0;

	// Add a velocity constraint 
	virtual void add_constraint(uint32_t node, const glm::vec3& v, 
		const glm::vec3& dir,
		Float friction = Float(0)) = 0;
	// Constrain all degrees of freedom
	virtual void add_constraint(uint32_t node, const glm::vec3& v) = 0;
	virtual void erase_constraint(uint32_t node) = 0;
	virtual void add_position_alteration(uint32_t node, const glm::vec3& dx) = 0;
	virtual const Vec3& get_node(uint32_t node) const = 0;
	virtual Vec3 get_velocity(uint32_t node) const = 0;
	virtual Vec3 get_force_constraint(uint32_t node) const = 0;

	virtual Float compute_volume() const = 0;

	virtual void clear_frame_alterations() = 0;

	struct MetricTimes {
		float step = 0.0f;
		float set_zero = 0.0f;
		float blocks_assign = 0.0f;
		float system_finish = 0.0f;
		float constraints = 0.0f;
		float solve = 0.0f;
	};
	MetricTimes get_metric_times() const { return m_metric_time; }
	bool simulation_converged() const { return m_converged; }

protected:
	MetricTimes m_metric_time;
	bool m_converged = true;
};

class EnergyDensity {
public:
	void HookeanSmith19(const Mat3& F, Float mu, Float lambda);
	void HookeanSmith19Eigendecomposition(const Mat3& F, Float mu, Float lambda);
	void Corrotational(const Mat3& F, Float mu, Float lambda);
	void HookeanBW08(const Mat3& F, Float mu, Float lambda);

	const Mat3& pk1() const { return this->m_pk1; }
	const Mat9& hessian() const { return this->m_hessian; }

private:
	Mat3 m_pk1;
	Mat9 m_hessian;

};

enum EnergyFunction {
	HookeanSmith19 = 0,
	Corrotational = 1,
	HookeanSmith19Eigen = 2,
	HookeanBW08 = 3,
};

class Parameters {
public:
	Parameters() { this->update_lame(); }
	Parameters(Float young, Float nu) :
		m_young(young), m_nu(nu) { this->update_lame(); }

	const Float& young() const { return m_young; }
	const Float& nu() const { return m_nu; }
	const Float& gravity() const { return m_gravity; }
	const Float& mu() const { return m_mu; }
	const Float& lambda() const { return m_lambda; }
	const Float& alpha_rayleigh() const { return m_alpha_rayleigh; }
	const Float& beta_rayleigh() const { return m_beta_rayleigh; }
	const Float& mass() const { return m_node_mass; }
	const EnergyFunction& energy_function() const { return m_enum_energy; }

	void draw_ui();


private:
	Float m_young;
	Float m_nu;

	Float m_node_mass = 1.0e-2f;
	Float m_gravity = 9.8f;
	Float m_mu = 0.0f;
	Float m_lambda = 0.0f;
	Float m_alpha_rayleigh = 0.01f;
	Float m_beta_rayleigh = 0.001f;

	EnergyFunction m_enum_energy = EnergyFunction::HookeanSmith19;

	

	void update_lame();

	// Serialization
	template<typename Archive>
	void serialize(Archive& archive);

	TF_SERIALIZE_PRIVATE_MEMBERS
};

} // namespace sim