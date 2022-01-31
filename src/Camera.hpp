#pragma once

#include <glm/glm.hpp>

// Simple camera. Mostly extracted from LearnOpenGL
class Camera {
public:
	Camera();

	// Get input and update the camera state
	void update();

	// Get the Projection matrix mutliplied by the view matrix
	glm::mat4 getProjView() const { return mProjView;  }

	// Render some information into the UI
	void render_ui();

	const glm::vec3& get_eye() const { return mPosition; }

private:
	glm::vec3 mPosition;
	glm::vec3 mFront;
	glm::vec3 mUp;
	glm::vec3 mRight;
	float mYaw, mPitch;

	glm::mat4 mProjView;

	float mMouseSensitivity;
	float mSpeed;
	float mZoom;

	glm::mat4 computeProjView() const;
	void updateCameraVectors();
};