#include "ParallelFEM.hpp"

#undef NDEBUG
#include <assert.h>
#include <iostream>
#include <imgui.h>
#include <glm/gtc/constants.hpp>

#include "utils/Timer.hpp"

namespace sim {

ParallelFEM::ParallelFEM()
{
}

void ParallelFEM::initialize(const std::vector<const TetMesh*>& meshes)
{
	m_converged = true;

	uint32_t num_elements = 0;
	uint32_t num_nodes = 0;
	std::vector<uint32_t> offsets;
	offsets.reserve(meshes.size());
	for (const TetMesh* mesh : meshes) {
		assert(mesh != nullptr);
		offsets.push_back(num_nodes);
		num_elements += (uint32_t)mesh->elements().size();
		num_nodes += (uint32_t)mesh->nodes().size();
	}

	// Load elements
	m_elements.reserve(num_elements);
	m_nodes.reserve(num_nodes);
	for (size_t mesh_idx = 0; mesh_idx < meshes.size(); ++mesh_idx) {
		const TetMesh* mesh = meshes[mesh_idx];
		assert(mesh != nullptr);

		for (const Eigen::Vector3f& p : mesh->nodes()) {
			m_nodes.push_back(p.cast<Float>());
		}

		for (size_t e = 0; e < mesh->elements().size(); ++e) {
			Vec4i element = mesh->elements()[e];
			element[0] += offsets[mesh_idx];
			element[1] += offsets[mesh_idx];
			element[2] += offsets[mesh_idx];
			element[3] += offsets[mesh_idx];
			m_elements.push_back(element);
		}
	}

	// Precompute volumes and matrix to build the deformation gradient
	m_DmInvs.resize(m_elements.size());
	m_volumes.resize(m_elements.size());
	for (size_t i = 0; i < m_elements.size(); ++i) {
		const Mat3 Ds = compute_Ds(m_elements[i], m_nodes);
		m_volumes[i] = std::abs(Ds.determinant()) / Float(6.0);
		m_DmInvs[i] = Ds.inverse();
	}

	// Reserve enough memory to solve the linear system
	m_dfdx_system.resize(3 * m_nodes.size(), 3 * m_nodes.size());
	m_dfdx_system.reserve(Eigen::VectorXi::Constant(3 * m_nodes.size(), 3 * 4 * 6));

	m_v.resize(3 * m_nodes.size());
	m_v.setZero();
	m_delta_v.resize(3 * m_nodes.size());
	m_delta_v.setZero();
	m_rhs.resize(3 * m_nodes.size());
	m_z.resize(3 * m_nodes.size()); m_z.setZero();
	m_position_alteration.resize(3 * m_nodes.size());
	m_constraint_forces.resize(3 * m_nodes.size());

	m_tmp.resize(3 * m_nodes.size());

	// Build the sparse matrix
	this->build_sparse_system();
	m_system = m_dfdx_system;

	m_cg_solver.resize(3 * m_nodes.size());
}


template<typename T>
void ParallelFEM::assign_sparse_block(const Eigen::Block<const T, 3, 3>& m, uint32_t node_i, uint32_t node_j) {

	const SMatPtrs& cols = m_sparse_cache.at(std::make_pair(node_i, node_j));

	for (uint32_t s = 0; s < 3; ++s) {
		Float* col = cols[s];
		for (uint32_t t = 0; t < 3; ++t) {
#pragma omp atomic
			*(col + t) += m(t, s);
			// m_dfdx_system.coeffRef(3 * node_i + t, 3 * node_j + s) += m(t, s); // Set one by one DEBUG
		}
	}
}

void ParallelFEM::set_system_to_zero()
{
	for (Eigen::Index i = 0; i < m_dfdx_system.nonZeros(); ++i) {
		m_dfdx_system.valuePtr()[i] = Float(0);
	}
}


void ParallelFEM::step(Float dt, const Parameters& cfg)
{
	Timer step_timer;
	Timer timer;

	// Reset system
	this->set_system_to_zero();
	m_rhs.setZero();

	m_metric_time.set_zero = (float)timer.getDuration<Timer::Seconds>().count();
	timer.reset();

	// We are building the system
	// 	   [M - Δt * df/dv - Δt^2 * df/dx] * Δv = Δt * f + Δt^2 * df/dx * v + Δt * df/dx * y
	// The last bit Δt * df/dx * y comes from the forced position alteration Δx=Δt(v0+Δv)+y

	const EnergyFunction functionType = cfg.energy_function();
	// Add contribution of each element
#pragma omp parallel for
	for (int32_t i = 0; i < (int32_t)m_elements.size(); ++i) {
		EnergyDensity energy;
		const Vec4i& element = m_elements[i];
		const Mat3 F = compute_Ds(element, m_nodes) * m_DmInvs[i];
		// Compute the energy function
		if (functionType == EnergyFunction::HookeanSmith19) {
			energy.HookeanSmith19(F, cfg.mu(), cfg.lambda());
		}
		else if (functionType == EnergyFunction::HookeanSmith19Eigen) {
			energy.HookeanSmith19Eigendecomposition(F, cfg.mu(), cfg.lambda());
		}
		else if (functionType == EnergyFunction::Corrotational) {
			energy.Corrotational(F, cfg.mu(), cfg.lambda());
		}
		else {
			energy.HookeanBW08(F, cfg.mu(), cfg.lambda());
		}

		// Compute force derivative df/dx = -vol * ddPhi/ddx = -vol * ( dF/dx * ddPhi/ddF * dF/dx )
		const Mat9x12 dFdx = compute_dFdx(m_DmInvs[i]);
		const Mat9& H = energy.hessian();
		//const Mat9 H = check_eigenvalues_BW08(F);
		const Mat12 dfdx = -m_volumes[i] * (dFdx.transpose() * H * dFdx);

		// Compute force f = -vol * dPhi/dx
		const Mat3& pk1 = energy.pk1();
		const Vec12 f = -m_volumes[i] * (dFdx.transpose() * pk1.reshaped());

		// Assign the force gradient to the system
		for (uint32_t j = 0; j < 4; ++j) {
			const uint32_t node_j = element[j];
			// add forces to rhs
#pragma omp atomic
			m_rhs(3 * node_j + 0) += dt * f(3 * j + 0);
#pragma omp atomic
			m_rhs(3 * node_j + 1) += dt * f(3 * j + 1);
#pragma omp atomic
			m_rhs(3 * node_j + 2) += dt * f(3 * j + 2);

			// diagonal
			assign_sparse_block(dfdx.block<3, 3>(3 * j, 3 * j), node_j, node_j);
			// off-diagonal
			for (uint32_t k = j + 1; k < 4; ++k) {
				const uint32_t node_k = element[k];
				assign_sparse_block(dfdx.block<3, 3>(3 * k, 3 * j), node_k, node_j);
				assign_sparse_block(dfdx.block<3, 3>(3 * j, 3 * k), node_j, node_k);
			}
		}
	}

	m_metric_time.blocks_assign = (float)timer.getDuration<Timer::Seconds>().count();
	timer.reset();
	// add Δt^2 * (df/dx * v) + Δt * df/dx * y to the rhs
	m_tmp.noalias() = dt * m_v;
	m_tmp += m_position_alteration;	// add Δt * df/dx * y
	m_rhs.noalias() += dt * (m_dfdx_system * m_tmp);

	// Apply rayleigh damping df/dv = -alpha * M - beta * df/dx
	// Optimized:	M - Δt^2 * df/dx - Δt * df/dv 
	//				M - Δt^2 * df/dx - Δt * (-alpha * M - beta * df/dx)
	//				M * (1 - Δt * alpha) - * df/dx (Δt^2 + Δt * beta)
	m_dfdx_system *= -(dt * dt) - cfg.beta_rayleigh() * dt;
	{
		const Float value_alpha = cfg.mass() * (Float(1.0) - cfg.alpha_rayleigh() * dt);
#pragma omp parallel for
		for (int32_t i = 0; i < m_nodes.size(); ++i) {
			const SMatPtrs& cols = m_sparse_cache.at(std::make_pair(i, i));
			cols[0][0] += value_alpha;
			cols[1][1] += value_alpha;
			cols[2][2] += value_alpha;

			// subtract gravity from the y entries
			m_rhs(3 * i + 1) -= dt * cfg.mass() * cfg.gravity();
		}
	}

	// Add tangential friction forces f = -k * v
	// And tangential friction damping as df/dv= -k * (I - n · n^T)
	for (const std::pair<uint32_t, Constraint>& c : m_constraints3) {
		if (c.second.friction != Float(0)) {
			const Float k = c.second.friction;
			const Mat3 friction_dfdv =  dt * k * c.second.constraint;
			assign_sparse_block(friction_dfdv.block<3, 3>(0, 0), c.first, c.first);

			m_rhs.segment<3>(3 * c.first) -= dt * k * m_v.segment<3>(3 * c.first);
		}
	}

	// Fill S
	m_metric_time.system_finish = (float)timer.getDuration<Timer::Seconds>().count();
	timer.reset();

	// Pre-filtered Preconditioned Conjugate Gradient
	// (SAS^T + I - S)y = Sc
	//                y = x - z
	//                c = b - Az

	// Compute rhs
	m_Sc.noalias() = (m_rhs - m_dfdx_system * m_z);
	for (const std::pair<uint32_t, Constraint>& c : m_constraints3) {
		const uint32_t idx = 3 * c.first;
		m_Sc.segment<3>(idx) = c.second.constraint * m_Sc.segment<3>(idx);
	}

	// Apply SAS^T, S symetric
	m_system = m_dfdx_system;
	assert(m_system.isCompressed());
#pragma omp parallel for
	for (int32_t col = 0; col < m_nodes.size(); ++col) {
		SMat::InnerIterator it0(m_system, 3 * col + 0);
		SMat::InnerIterator it1(m_system, 3 * col + 1);
		SMat::InnerIterator it2(m_system, 3 * col + 2);
		std::map<uint32_t, Constraint>::const_iterator c_col = m_constraints3.find(col);
		while (it0) {
			uint32_t row = it0.index() / 3;
			std::map<uint32_t, Constraint>::const_iterator c_row = m_constraints3.find(row);
			bool needs_update = c_col != m_constraints3.end() || c_row != m_constraints3.end();
			if (needs_update) {
				Mat3 A;
				for (uint32_t i = 0; i < 3; ++i) {
					A(i, 0) = (&it0.value())[i];
					A(i, 1) = (&it1.value())[i];
					A(i, 2) = (&it2.value())[i];
				}

				Mat3 SAS;
				if (c_col != m_constraints3.end() && c_row != m_constraints3.end()) {
					SAS = c_row->second.constraint * A * c_col->second.constraint;
				}
				else if (c_col != m_constraints3.end()) {
					SAS = A * c_col->second.constraint ;
				}
				else {
					SAS = c_row->second.constraint * A;
				}

				for (uint32_t i = 0; i < 3; ++i) {
					(&it0.valueRef())[i] = SAS(i, 0);
					(&it1.valueRef())[i] = SAS(i, 1);
					(&it2.valueRef())[i] = SAS(i, 2);
				}

				// SAS^T + I - S
				if (row == col) {
					for (uint32_t i = 0; i < 3; ++i) {
						(&it0.valueRef())[i] -= c_row->second.constraint(i, 0);
						(&it1.valueRef())[i] -= c_row->second.constraint(i, 1);
						(&it2.valueRef())[i] -= c_row->second.constraint(i, 2);
					}
					(&it0.valueRef())[0] += Float(1);
					(&it1.valueRef())[1] += Float(1);
					(&it2.valueRef())[2] += Float(1);
				}
			}

			// advance
			for (uint32_t i = 0; i < 3; ++i) {
				++it0; ++it1; ++it2;
			}
		}
	}

	
	m_metric_time.constraints = (float)timer.getDuration<Timer::Seconds>().count();
	timer.reset();

#if false
	Eigen::ConjugateGradient<SMat> solver(m_system);
	//solver.setTolerance(1e-4);
	//solver.setMaxIterations(20 * m_nodes.size());
	if (solver.info() != Eigen::Success) {
		std::cerr << "Can't build system" << std::endl;
	}
	m_delta_v = solver.solve(m_Sc);
	m_converged = solver.info() != Eigen::Success;
#elif false
	Eigen::SimplicialLDLT<SMat> solver(m_system);
	//solver.setTolerance(1e-4);
	//solver.setMaxIterations(20 * m_nodes.size());
	if (solver.info() != Eigen::Success) {
		std::cerr << "Can't build system" << std::endl;
	}
	m_delta_v = solver.solve(m_Sc);
	converged = solver.info() != Eigen::Success;
#else
	m_converged = m_cg_solver.solve(m_system, m_Sc, &m_delta_v);
#endif

	if (!m_converged) {
		std::cerr << "System did not converge" << std::endl;
	}

	m_v += m_delta_v + m_z;

	m_metric_time.solve = (float)timer.getDuration<Timer::Seconds>().count();
	timer.reset();

	// Compute constraint forces
	if (m_constraints3.empty()) {
		m_constraint_forces.setZero();
	}
	else {
		m_constraint_forces = m_dfdx_system * m_delta_v - m_rhs;
	}

	// Assign new positions to the nodes
#pragma omp parallel for
	for (int32_t i = 0; i < m_nodes.size(); ++i) {
		m_nodes[i].x() += dt * m_v[3 * i + 0];
		m_nodes[i].y() += dt * m_v[3 * i + 1];
		m_nodes[i].z() += dt * m_v[3 * i + 2];
	}

	// Set position alteration
	for (SVec::InnerIterator it(m_position_alteration); it;)
	{
		const uint32_t node_idx = (uint32_t)it.index() / 3;
		// There must be values for the x y z
		m_nodes[node_idx].x() += it.value(); ++it;
		m_nodes[node_idx].y() += it.value(); ++it;
		m_nodes[node_idx].z() += it.value(); ++it;
	}

	m_metric_time.step = (float)step_timer.getDuration<Timer::Seconds>().count();
}

void ParallelFEM::update_objects(TetMesh* mesh,
	uint32_t from_sim_idx, uint32_t to_sim_idx,
	bool add_position_alteration)
{
	assert(mesh != nullptr);

	SVec::InnerIterator it_dx(m_position_alteration);
	if (add_position_alteration && from_sim_idx > 0) {
		while (it_dx && it_dx.index() < (size_t)from_sim_idx) {
			++it_dx;
		}
	}

	for (uint32_t i = from_sim_idx; i < to_sim_idx; ++i) {
		Eigen::Vector3f pos = m_nodes[i].cast<float>();
		if (add_position_alteration && it_dx && it_dx.index() == 3 * i) {
			pos.x() += (float)it_dx.value(); ++it_dx;
			pos.y() += (float)it_dx.value(); ++it_dx;
			pos.z() += (float)it_dx.value(); ++it_dx;
		}
		mesh->update_node((int32_t)(i - from_sim_idx), pos);
	}
}

void ParallelFEM::add_constraint(uint32_t node, const glm::vec3& v, 
	const glm::vec3& dir, Float friction)
{
	std::map<uint32_t, Constraint>::iterator it = m_constraints3.find(node);

	if (it != m_constraints3.end()) {
		m_constraints3.erase(it);
	}

	m_z(3 * node + 0) = v.x - m_v[3 * node + 0];
	m_z(3 * node + 1) = v.y - m_v[3 * node + 1];
	m_z(3 * node + 2) = v.z - m_v[3 * node + 2];

	const Vec3 d(dir.x, dir.y, dir.z);

	m_constraints3.emplace(node,
		Constraint{
			d,
			Mat3::Identity() - (d * d.transpose()),
			friction
		}
	);
}

void ParallelFEM::add_constraint(uint32_t node, const glm::vec3& v)
{

	std::map<uint32_t, Constraint>::iterator it = m_constraints3.find(node);

	if (it != m_constraints3.end()) {
		if (it->second.dir.isZero() || it->second.dir.dot(cast_vec3(v)) < Float(0.0)) {
			return;
		}
		else {
			m_constraints3.erase(it);
		}
	}

	m_z(3 * node + 0) = v.x - m_v[3 * node + 0];
	m_z(3 * node + 1) = v.y - m_v[3 * node + 1];
	m_z(3 * node + 2) = v.z - m_v[3 * node + 2];

	m_constraints3.emplace(node,
		Constraint{
			Vec3::Zero(),
			Mat3::Zero()
		}
	);
}

void ParallelFEM::erase_constraint(uint32_t node)
{
	m_constraints3.erase(node);
}

void ParallelFEM::add_position_alteration(uint32_t node, const glm::vec3& dx)
{
	m_position_alteration.coeffRef(3 * node + 0) = dx.x;
	m_position_alteration.coeffRef(3 * node + 1) = dx.y;
	m_position_alteration.coeffRef(3 * node + 2) = dx.z;
}

void ParallelFEM::clear_frame_alterations()
{
	m_z.setZero();
	m_position_alteration.setZero();
}
const Vec3& ParallelFEM::get_node(uint32_t node) const
{
	return m_nodes[node];
}

Vec3 ParallelFEM::get_velocity(uint32_t node) const
{
	return m_v.segment<3>(3 * node);
}

Vec3 ParallelFEM::get_force_constraint(uint32_t node) const
{
	assert(m_constraints3.count(node));
	return m_constraint_forces.segment<3>(node * 3);
}

Float ParallelFEM::compute_volume() const
{
	Float vol = Float(0);
	for (size_t i = 0; i < m_elements.size(); ++i) {
		const Mat3 Ds = compute_Ds(m_elements[i], m_nodes);
		vol += std::abs(Ds.determinant()) / Float(6.0);
	}

	return vol;
}


void ParallelFEM::build_sparse_system()
{
	m_sparse_cache.clear();
	m_sparse_cache.reserve(m_elements.size() * 4 * 4);
	m_dfdx_system.setZero();

	// Add all nodes with connectivity
	for (size_t e = 0; e < m_elements.size(); ++e) {
		const Vec4i& element = m_elements[e];

		// Assign the force gradient to the system
		for (uint32_t i = 0; i < 4; ++i) {
			const uint32_t node_i = element[i];
			// diagonal
			m_sparse_cache.emplace(std::make_pair(node_i, node_i), SMatPtrs());
			// off-diagonal
			for (uint32_t j = i + 1; j < 4; ++j) {
				const uint32_t node_j = element[j];
				m_sparse_cache.emplace(std::make_pair(node_i, node_j), SMatPtrs());
				m_sparse_cache.emplace(std::make_pair(node_j, node_i), SMatPtrs());
			}
		}
	}

	typedef Eigen::Triplet<Float> Triplet;
	std::vector<Triplet> triplets;
	triplets.reserve(m_sparse_cache.size() * 9);

	for (const auto& it : m_sparse_cache) {
		const uint32_t base_i = it.first.first * 3;
		const uint32_t base_j = it.first.second * 3;
		for (uint32_t i = 0; i < 3; ++i) {
			for (uint32_t j = 0; j < 3; ++j) {
				triplets.emplace_back(Triplet(base_i + i, base_j + j, Float(0)));
			}
		}
	}

	m_dfdx_system.setFromTriplets(triplets.begin(), triplets.end());

	// Store start of each of the xyz components of the columns into m_sparse_cache
	for (auto& it : m_sparse_cache) {
		const uint32_t base_i = it.first.first * 3;
		const uint32_t base_j = it.first.second * 3;
		for (uint32_t j = 0; j < 3; ++j) {
			it.second[j] = &m_dfdx_system.coeffRef(base_i, base_j + j);
		}
	}

}

} // namespace sim