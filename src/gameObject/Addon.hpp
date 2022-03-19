#pragma once

class Context;
namespace gobj {

class GameObject;
class Addon {
public:
	virtual void render_ui(const Context& ctx, GameObject* parent) = 0;
	virtual void update(const Context& ctx, GameObject* parent) = 0;
};

} // namespace gobj