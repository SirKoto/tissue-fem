#pragma once

#include <glm/glm.hpp>
#include "utils/serialization.hpp"

// Simple camera. Mostly extracted from LearnOpenGL
class Camera {
public:
	Camera();

	// Get input and update the camera state
	void update();

	// Get the Projection matrix mutliplied by the view matrix
	const glm::mat4& getProjView() const { return m_proj_vew; }
	const glm::mat4& getProj() const { return m_proj; }
	const glm::mat4& getView() const { return m_view; }

	// Render some information into the UI
	void draw_ui();

	const glm::vec3& get_eye() const { return m_position; }

private:
	glm::vec3 m_position;
	glm::vec3 m_front;
	glm::vec3 m_up;
	glm::vec3 m_right;
	float m_yaw, m_pitch;

	glm::mat4 m_proj_vew;
	glm::mat4 m_proj;
	glm::mat4 m_view;

	float m_mouse_sensitivity;
	float m_speed;
	float m_zoom;

	void computeProjView();
	void updateCameraVectors();

	// Serialization
	template<typename Archive>
	void serialize(Archive& archive);

	TF_SERIALIZE_PRIVATE_MEMBERS
};