#pragma once

#include <glm/glm.hpp>
#include <memory>

#include "gameObject/Transform.hpp"
#include "utils/serialization.hpp"

class Context;
class GameObject
{
public:

	GameObject();

	GameObject(const GameObject&) = delete;
	GameObject(GameObject&&) noexcept = default;
	GameObject& operator=(const GameObject&) = delete;
	GameObject& operator=(GameObject&&) = default;


	virtual void start_simulation(const Context& ctx);

	virtual void render(const Context& ctx) const = 0;

	virtual void render_ui(const Context& gc);

	virtual void update(const Context& gc);
	virtual void late_update(const Context& gc);

	const glm::mat4& get_model_matrix() const { return m_transform.mat4(); }
	const glm::mat4& get_inv_model_matrix() const { return m_transform.inverse(); }
	const gobj::Transform& get_transform() const { return m_transform; }
	gobj::Transform& get_transform() { return m_transform; }

	const std::string& get_name() const { return m_name; }
	std::string& get_name() { return m_name; }

protected:

	std::string m_name;

	gobj::Transform m_transform;
	//std::list<std::unique_ptr<gobj::Addon>> m_addons;
	
	bool m_disable_interaction = false;

	// Serialization
	template<typename Archive>
	void serialize(Archive& archive);

	TF_SERIALIZE_PRIVATE_MEMBERS

};
