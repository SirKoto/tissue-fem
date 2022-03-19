#pragma once

#include <memory>

#include "Addon.hpp"
#include "sim/IFEM.hpp"

namespace gobj {

class ElasticSim : Addon {
public:

	ElasticSim();

	void render_ui(const Context& ctx, GameObject* parent) override final;
	void update(const Context& ctx, GameObject* parent) override final;

private:
	std::unique_ptr<sim::IFEM> m_sim;
};

} // namespace gobj