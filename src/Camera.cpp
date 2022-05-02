#include "Camera.hpp"

#include <imgui.h>
#include <ImGuizmo.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


Camera::Camera() : m_position(5,5,15), m_front(0,0,-1), 
	m_up(0,1,0), m_yaw(-90.f),m_pitch(0), 
	m_mouse_sensitivity(0.1f), m_speed(4.5f), m_zoom(45.f)
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
			float speed = m_speed * io.DeltaTime;
			if (io.KeysDown[GLFW_KEY_LEFT_SHIFT]) {
				speed *= 5.0f;
			}

			if (io.KeysDown[GLFW_KEY_W]) {
				m_position += m_front * speed;
			}
			if (io.KeysDown[GLFW_KEY_S]) {
				m_position -= m_front * speed;
			}
			if (io.KeysDown[GLFW_KEY_D]) {
				m_position += m_right * speed;
			}
			if (io.KeysDown[GLFW_KEY_A]) {
				m_position -= m_right * speed;
			}
		}

		// Mouse
		m_yaw += m_mouse_sensitivity * io.MouseDelta.x;
		m_pitch -= m_mouse_sensitivity * io.MouseDelta.y;

		m_pitch = glm::clamp(m_pitch, -89.0f, 89.0f);

		// update Front, Right and Up Vectors using the updated Euler angles
		updateCameraVectors();
	}
	else {
		ImGuizmo::Enable(true);
	}


	// zoom
	m_zoom -= io.MouseWheel;
	m_zoom = glm::clamp(m_zoom, 30.0f, 90.0f);

	// Update matrices
	this->computeProjView();
}

void Camera::computeProjView()
{
	ImGuiIO& io = ImGui::GetIO();
	if (io.DisplaySize.y == 0 || io.DisplaySize.x == 0) {
		m_view = glm::mat4(1.0f);
		m_proj = glm::mat4(1.0f);
		m_proj_vew = glm::mat4(1.0f);
		return;
	}
	m_view = glm::lookAt(m_position, m_position + m_front, m_up);
	m_proj = glm::perspective(glm::radians(m_zoom), io.DisplaySize.x / io.DisplaySize.y, 0.1f, 100.0f);
	m_proj_vew = m_proj * m_view;
}

void Camera::draw_ui()
{
	ImGui::PushID("Camera");

	bool update = false;
	update |= ImGui::DragFloat3("Position", glm::value_ptr(m_position));
	update |= ImGui::DragFloat("Yaw", &m_yaw, 1.0f);
	update |= ImGui::DragFloat("Pitch", &m_pitch, 1.0f);
	update |= ImGui::DragFloat("Zoom", &m_zoom, 1.0f, 30.0f, 90.0f);

	ImGui::Text("Front (%.3f, %.3f, %.3f)", m_front.x, m_front.y, m_front.z);
	ImGui::Text("Up (%.3f, %.3f, %.3f)", m_up.x, m_up.y, m_up.z);
	ImGui::Text("Right (%.3f, %.3f, %.3f)", m_right.x, m_right.y, m_right.z);
	
	ImGui::InputFloat("Camera speed", &m_speed, 1.0f);

	ImGui::PopID();

	if (update) {
		this->updateCameraVectors();
		this->computeProjView();
	}
}

void Camera::updateCameraVectors()
{
	glm::vec3 front;
	front.x = std::cos(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch));
	front.y = std::sin(glm::radians(m_pitch));
	front.z = std::sin(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch));
	m_front = glm::normalize(front);
	// also re-calculate the Right and Up vector
	m_right = glm::normalize(glm::cross(m_front, glm::vec3(0,1,0)));
	m_up = glm::normalize(glm::cross(m_right, m_front));
}


template<typename Archive>
inline void Camera::serialize(Archive& archive)
{
	archive(TF_SERIALIZE_NVP_MEMBER(m_position));
	archive(TF_SERIALIZE_NVP_MEMBER(m_front));
	archive(TF_SERIALIZE_NVP_MEMBER(m_up));
	archive(TF_SERIALIZE_NVP_MEMBER(m_right));
	archive(TF_SERIALIZE_NVP_MEMBER(m_yaw));
	archive(TF_SERIALIZE_NVP_MEMBER(m_pitch));
	archive(TF_SERIALIZE_NVP_MEMBER(m_mouse_sensitivity));
	archive(TF_SERIALIZE_NVP_MEMBER(m_speed));
	archive(TF_SERIALIZE_NVP_MEMBER(m_zoom));

	this->updateCameraVectors();
	this->computeProjView();
}

TF_SERIALIZE_TEMPLATE_EXPLICIT_IMPLEMENTATION(Camera)