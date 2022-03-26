#pragma once

class Context;
class GameObject;

namespace gobj {
class Addon {
public:
	virtual void render_ui(const Context& ctx, GameObject* parent) = 0;
	virtual void update(const Context& ctx, GameObject* parent) = 0;
	virtual void late_update(const Context& ctx, GameObject* parent) = 0;
	virtual const char* get_name() const = 0;
};

} // namespace gobj