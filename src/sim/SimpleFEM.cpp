#include "SimpleFEM.hpp"

#undef NDEBUG
#include <assert.h>
#include <iostream>
#include <imgui.h>
#include <glm/gtc/constants.hpp>

#include "utils/Timer.hpp"

namespace sim {
SimpleFem::SimpleFem() 
{
}

void SimpleFem::initialize(const std::vector<const TetMesh*>& meshes)
{
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
	m_S.resize(3 * m_nodes.size(), 3 * m_nodes.size());
	// Reserve 3 values per column, as it is the maximum that will be in a constrain matrix
	m_S.reserve(Eigen::VectorXi::Constant(3 * m_nodes.size(), 3));


	// Build the sparse matrix
	this->build_sparse_system();

	m_cg_solver.resize(3 * m_nodes.size());
}


void SimpleFem::assign_sparse_block(const Eigen::Block<const Mat12, 3, 3>& m, uint32_t node_i, uint32_t node_j) {

	const SMatPtrs& cols = m_sparse_cache.at(std::make_pair(node_i, node_j));

	for (uint32_t s = 0; s < 3; ++s) {
		Float* col = cols[s];
		for (uint32_t t = 0; t < 3; ++t) {
			*(col + t) += m(t, s);
			// m_dfdx_system.coeffRef(3 * node_i + t, 3 * node_j + s) += m(t, s); // Set one by one DEBUG
		}
	}
}

void SimpleFem::set_system_to_zero()
{
	for (Eigen::Index i = 0; i < m_dfdx_system.nonZeros(); ++i) {
		m_dfdx_system.valuePtr()[i] = Float(0);
	}
}


void SimpleFem::step(Float dt, const Parameters& cfg)
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
	
	EnergyDensity energy;
	const EnergyFunction functionType = cfg.energy_function();
	// Add contribution of each element
	for (size_t i = 0; i < m_elements.size(); ++i) {
		const Vec4i& element = m_elements[i];
		const Mat3 F = compute_Ds(element, m_nodes) * m_DmInvs[i];
		// Compute the energy function
		if (functionType == EnergyFunction::HookeanSmith19) {
			energy.HookeanSmith19(F, cfg.mu(), cfg.lambda());
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
			m_rhs.segment<3>(3 * node_j) += dt * f.segment<3>(3 * j);

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
	// add (df/dx * v) to the rhs
	m_rhs += dt * dt * (m_dfdx_system * m_v);
	// add Δt * df/dx * y
	m_rhs += dt * (m_dfdx_system * m_position_alteration);

	// subtract gravity from the y entries
	for (size_t i = 0; i < m_nodes.size(); ++i) {
		m_rhs(3 * i + 1) -= dt * cfg.mass() * cfg.gravity();
	}

	// Apply rayleigh damping df/dv = -alpha * M - beta * df/dx
	m_dfdx_system *=  - (dt * dt) - cfg.beta_rayleigh() * dt;
	m_dfdx_system.diagonal().array() += cfg.mass() * (Float(1.0) - cfg.alpha_rayleigh() * dt);

	m_metric_time.system_finish = (float)timer.getDuration<Timer::Seconds>().count();

	// Fill S
	m_S.setZero();
	std::map<uint32_t, Constraint>::const_iterator c = m_constraints3.begin();
	for (uint32_t i = 0;
		i < m_nodes.size(); ++i) {
		const uint32_t idx = 3 * i;
		if (c != m_constraints3.end() && c->first == i) {
			for (uint32_t j = 0; j < 3; ++j) {
				for (uint32_t k = 0; k < 3; ++k) {
					m_S.insert(idx + k, idx + j) = c->second.constraint(k, j);
				}
			}
			++c;
		}
		else {
			m_S.insert(idx + 0, idx + 0) = Float(1.0);
			m_S.insert(idx + 1, idx + 1) = Float(1.0);
			m_S.insert(idx + 2, idx + 2) = Float(1.0);
		}
	}

	// Pre-filtered Preconditioned Conjugate Gradient
	// (SAS^T + I - S)y = Sc
	//                y = x - z
	//                c = b - Az
	
	// Compute rhs
	m_Sc = m_S * (m_rhs - m_dfdx_system * m_z);
	

	// Apply SAS^T, S symetric
	m_system = m_S * m_dfdx_system * m_S;

	// SAS^T + I - S
	m_system -= m_S;
	m_system.diagonal().array() += Float(1.0);

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
	if (solver.info() != Eigen::Success) {
		std::cerr << "System did not converge" << std::endl;
	}
#else
	m_cg_solver.solve(m_system, m_Sc, &m_delta_v);
#endif
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
	for (size_t i = 0; i < m_nodes.size(); ++i) {
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

void SimpleFem::update_objects(TetMesh* mesh, 
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

void SimpleFem::add_constraint(uint32_t node, const glm::vec3& v, const glm::vec3& dir)
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
			Mat3::Identity() - (d * d.transpose())
		}
		);
}

void SimpleFem::add_constraint(uint32_t node, const glm::vec3& v)
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

void SimpleFem::erase_constraint(uint32_t node)
{
	m_constraints3.erase(node);
}

void SimpleFem::add_position_alteration(uint32_t node, const glm::vec3& dx)
{
	m_position_alteration.coeffRef(3 * node + 0) = dx.x;
	m_position_alteration.coeffRef(3 * node + 1) = dx.y;
	m_position_alteration.coeffRef(3 * node + 2) = dx.z;
}

void SimpleFem::clear_frame_alterations()
{	
	m_z.setZero();
	m_position_alteration.setZero();
}
const Vec3& SimpleFem::get_node(uint32_t node) const
{
	return m_nodes[node];
}

Vec3 SimpleFem::get_velocity(uint32_t node) const
{
	return m_v.segment<3>(3 * node);
}

Vec3 SimpleFem::get_force_constraint(uint32_t node) const
{
	assert(m_constraints3.count(node));
	return m_constraint_forces.segment<3>(node * 3);
}

Float SimpleFem::compute_volume() const
{
	Float vol = Float(0);
	for (size_t i = 0; i < m_elements.size(); ++i) {
		const Mat3 Ds = compute_Ds(m_elements[i], m_nodes);
		vol += std::abs(Ds.determinant()) / Float(6.0);
	}

	return vol;
}


void SimpleFem::build_sparse_system()
{
	m_sparse_cache.clear();
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


// TODO: NOT WORKING
/*
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
*/
} // namespace sim