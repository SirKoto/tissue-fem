#include "Camera.hpp"

#include <imgui.h>
#include <ImGuizmo.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


Camera::Camera() : mPosition(5,5,15), mFront(0,0,-1), 
	mUp(0,1,0), mYaw(-90.f),mPitch(0), 
	mMouseSensitivity(0.1f), mSpeed(4.5f), mZoom(45.f)
{
	updateCameraVectors();
}

void Camera::update()
{
	ImGuiIO& io = ImGui::GetIO();
	if (io.WantCaptureMouse) {
		return;
	}

	// movement
	if (io.MouseDown[ImGuiMouseButton_Right]) {
		ImGuizmo::Enable(false);
		// Keyboard
		if (!io.WantCaptureKeyboard) {
			float speed = mSpeed * io.DeltaTime;
			if (io.KeysDown[GLFW_KEY_LEFT_SHIFT]) {
				speed *= 5.0f;
			}

			if (io.KeysDown[GLFW_KEY_W]) {
				mPosition += mFront * speed;
			}
			if (io.KeysDown[GLFW_KEY_S]) {
				mPosition -= mFront * speed;
			}
			if (io.KeysDown[GLFW_KEY_D]) {
				mPosition += mRight * speed;
			}
			if (io.KeysDown[GLFW_KEY_A]) {
				mPosition -= mRight * speed;
			}
		}

		// Mouse
		mYaw += mMouseSensitivity * io.MouseDelta.x;
		mPitch -= mMouseSensitivity * io.MouseDelta.y;

		mPitch = glm::clamp(mPitch, -89.0f, 89.0f);

		// update Front, Right and Up Vectors using the updated Euler angles
		updateCameraVectors();
	}
	else {
		ImGuizmo::Enable(true);
	}


	// zoom
	mZoom -= io.MouseWheel;
	mZoom = glm::clamp(mZoom, 30.0f, 90.0f);

	// Update matrices
	this->computeProjView();
}

void Camera::computeProjView()
{
	ImGuiIO& io = ImGui::GetIO();
	if (io.DisplaySize.y == 0 || io.DisplaySize.x == 0) {
		mView = glm::mat4(1.0f);
		mProj = glm::mat4(1.0f);
		mProjView = glm::mat4(1.0f);
		return;
	}
	mView = glm::lookAt(mPosition, mPosition + mFront, mUp);
	mProj = glm::perspective(glm::radians(mZoom), io.DisplaySize.x / io.DisplaySize.y, 0.1f, 100.0f);
	mProjView = mProj * mView;
}

void Camera::draw_ui()
{
	ImGui::PushID("Camera");

	bool update = false;
	update |= ImGui::DragFloat3("Position", glm::value_ptr(mPosition));
	update |= ImGui::DragFloat("Yaw", &mYaw, 1.0f);
	update |= ImGui::DragFloat("Pitch", &mPitch, 1.0f);
	update |= ImGui::DragFloat("Zoom", &mZoom, 1.0f, 30.0f, 90.0f);

	ImGui::Text("Front (%.3f, %.3f, %.3f)", mFront.x, mFront.y, mFront.z);
	ImGui::Text("Up (%.3f, %.3f, %.3f)", mUp.x, mUp.y, mUp.z);
	ImGui::Text("Right (%.3f, %.3f, %.3f)", mRight.x, mRight.y, mRight.z);
	
	ImGui::InputFloat("Camera speed", &mSpeed, 1.0f);

	ImGui::PopID();

	if (update) {
		this->updateCameraVectors();
		this->computeProjView();
	}
}

void Camera::updateCameraVectors()
{
	glm::vec3 front;
	front.x = std::cos(glm::radians(mYaw)) * std::cos(glm::radians(mPitch));
	front.y = std::sin(glm::radians(mPitch));
	front.z = std::sin(glm::radians(mYaw)) * std::cos(glm::radians(mPitch));
	mFront = glm::normalize(front);
	// also re-calculate the Right and Up vector
	mRight = glm::normalize(glm::cross(mFront, glm::vec3(0,1,0)));
	mUp = glm::normalize(glm::cross(mRight, mFront));
}
