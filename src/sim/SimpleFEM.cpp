#include "SimpleFEM.hpp"

#include <iostream>
#include <imgui.h>
#include <glm/gtc/constants.hpp>

#include "utils/Timer.hpp"

namespace sim {
SimpleFem::SimpleFem(std::shared_ptr<GameObject> obj, Float young, Float nu) :
	m_obj(obj),
	m_young(young), m_nu(nu)
{
	update_lame();

	// Load elements
	m_elements = m_obj->get_mesh().elements();
	m_nodes.resize(m_obj->get_mesh().nodes().size());
	for (size_t i = 0; i < m_nodes.size(); ++i) {
		m_nodes[i] = m_obj->get_mesh().nodes()[i].cast<Float>();
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
	m_dfdx_system = SMat(3 * m_nodes.size(), 3 * m_nodes.size());
	m_dfdx_system.reserve(Eigen::VectorXi::Constant(3 * m_nodes.size(), 3 * 4 * 6));

	m_v.resize(3 * m_nodes.size());
	m_v.setZero();
	m_delta_v.resize(3 * m_nodes.size());
	m_delta_v.setZero();
	m_rhs.resize(3 * m_nodes.size());
	m_z.resize(3 * m_nodes.size());
	m_constraints.resize(3 * m_nodes.size());
	m_constraints.setOnes();

	// Build the sparse matrix
	this->build_sparse_system();

	solver.resize(3 * m_nodes.size());
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

void SimpleFem::update_lame()
{
	m_mu = m_young / (Float(2) + Float(2) * m_nu);
	m_lambda = (m_young * m_nu / ((Float(1) + m_nu) * (Float(1) - Float(2) * m_nu)));
}


void SimpleFem::step(Float dt)
{
	Timer step_timer;
	Timer timer;

	// Reset system
	this->set_system_to_zero();
	m_rhs.setZero();

	m_metric_time.set_zero = (float)timer.getDuration<Timer::Seconds>().count();
	timer.reset();

	// We are building the system
	// 	   [M - Δt * df/dv - Δt^2 * df/dx] * Δv = Δt * f + Δt^2 * df/dx * v
	
	EnergyDensity energy;
	const EnergyFunction functionType = m_enum_energy;
	// Add contribution of each element
	for (size_t i = 0; i < m_elements.size(); ++i) {
		const Vec4i& element = m_elements[i];
		const Mat3 F = compute_Ds(element, m_nodes) * m_DmInvs[i];
		// Compute the energy function
		if (functionType == EnergyFunction::HookeanSmith19) {
			energy.HookeanSmith19(F, m_mu, m_lambda);
		}
		else if (functionType == EnergyFunction::Corrotational) {
			energy.Corrotational(F, m_mu, m_lambda);
		}
		else {
			energy.HookeanBW08(F, m_mu, m_lambda);
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

	// subtract gravity from the y entries
	for (size_t i = 0; i < m_nodes.size(); ++i) {
		m_rhs(3 * i + 1) -= dt * m_node_mass * m_gravity;
	}

	// Apply rayleigh damping df/dv = -alpha * M - beta * df/dx
	m_dfdx_system *=  - (dt * dt) - m_beta_rayleigh * dt;
	m_dfdx_system.diagonal().array() += m_node_mass * (Float(1.0) - m_alpha_rayleigh * dt);

	// Pre-filtered Preconditioned Conjugate Gradient
	// (SAS^T + I - S)y = Sc
	//                y = x - z
	//                c = b - Az
	
	// Compute rhs
	m_Sc = m_constraints.asDiagonal() * (m_rhs - m_dfdx_system * m_z);

	// Apply SAS^T
	for (Eigen::Index i = 0; i < m_dfdx_system.outerSize(); ++i) {
		for (SMat::InnerIterator it(m_dfdx_system, i); it; ++it) {
			it.valueRef() *= m_constraints[it.row()] * m_constraints[it.col()];
		}
	}
	m_dfdx_system.diagonal().array() += Float(1.0);
	m_dfdx_system.diagonal() -= m_constraints;

	m_metric_time.system_finish = (float)timer.getDuration<Timer::Seconds>().count();
	timer.reset();

	/*Eigen::ConjugateGradient<SMat> solver(m_dfdx_system);
	solver.setTolerance(1e-4);
	//solver.setMaxIterations(20 * m_nodes.size());
	if (solver.info() != Eigen::Success) {
		std::cerr << "Can't build system" << std::endl;
	}
	m_v += solver.solve(m_rhs);
	if (solver.info() != Eigen::Success) {
		std::cerr << "System did not converge" << std::endl;
	}*/
	solver.solve(m_dfdx_system, m_Sc, &m_delta_v);
	m_v += m_delta_v + m_z;
	m_metric_time.solve = (float)timer.getDuration<Timer::Seconds>().count();
	timer.reset();

	// Assign new positions to the nodes
	for (size_t i = 0; i < m_nodes.size(); ++i) {
		m_nodes[i].x() += dt * m_v[3 * i + 0];
		m_nodes[i].y() += dt * m_v[3 * i + 1];
		m_nodes[i].z() += dt * m_v[3 * i + 2];

		/*if (m_nodes[i].y() < Float(0.0)) {
			m_nodes[i].y() = Float(0.0);
			m_v[3 * i + 1] = Float(0.0);
		}*/
	}

	m_metric_time.step = (float)step_timer.getDuration<Timer::Seconds>().count();
}

void SimpleFem::update_objects()
{
	for (size_t i = 0; i < m_nodes.size(); ++i) {
		m_obj->get_mesh().update_node(i, m_nodes[i].cast<float>());
	}
	m_obj->get_mesh().upload_to_gpu(true, false);
}

void SimpleFem::add_constraint(uint32_t node, const glm::vec3& v)
{
	m_z.coeffRef(3 * node + 0) = v.x - m_v[3 * node + 0];
	m_z.coeffRef(3 * node + 1) = v.y - m_v[3 * node + 1];
	m_z.coeffRef(3 * node + 2) = v.z - m_v[3 * node + 2];

	m_constraints.segment(3 * node, 3).setZero();

}

void SimpleFem::pancake()
{
	for (size_t i = 0; i < m_nodes.size(); ++i) {
		m_nodes[i].y() *= Float(0.9f);
	}

	update_objects();
}

void SimpleFem::draw_ui()
{
	ImGui::PushID("SimpleFem");
	ImGui::Text("Simple Fem Configuration");

	constexpr ImGuiDataType dtype = sizeof(Float) == 4 ? ImGuiDataType_Float : ImGuiDataType_Double;

	const Float stepYoung = 100;
	bool updateLame = ImGui::InputScalar("Young", dtype, &m_young, &stepYoung, nullptr, "%f", ImGuiInputTextFlags_CharsScientific);
	const Float stepNu = 0.01;
	updateLame |= ImGui::InputScalar("Nu", dtype, &m_nu, &stepNu, nullptr, "%.3f", ImGuiInputTextFlags_CharsScientific);
	ImGui::InputScalar("Node mass", dtype, &m_node_mass, nullptr, nullptr, "%f", ImGuiInputTextFlags_CharsScientific);
	ImGui::Text("Model mass: %f", (float)m_node_mass * (float)m_nodes.size());
	ImGui::InputScalar("Alpha Rayleigh", dtype, &m_alpha_rayleigh, &stepNu, nullptr, "%f", ImGuiInputTextFlags_CharsScientific);
	ImGui::InputScalar("Beta Rayleigh", dtype, &m_beta_rayleigh, &stepNu, nullptr, "%f", ImGuiInputTextFlags_CharsScientific);

	if (updateLame) {
		this->update_lame();
	}

	ImGui::Combo("Energy function",
		reinterpret_cast<int*>(&m_enum_energy),
		"HookeanSmith19\0Corrotational\0HookeanBW08\0");

	ImGui::PopID();
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