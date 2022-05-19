#include "SimulatedGameObject.hpp"


#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <glad/glad.h>
#include <imgui.h>
#include <ImGuizmo.h>


#include "Context.hpp"


SimulatedGameObject::SimulatedGameObject() : GameObject()
{
	const std::filesystem::path proj_dir(PROJECT_DIR);
	const std::filesystem::path shad_dir = proj_dir / "resources/shaders";


	/*std::array<Shader, 2> mesh_shaders = {
		Shader((shad_dir / "mesh.vert"), Shader::Type::Vertex),
		Shader((shad_dir / "mesh.frag"), Shader::Type::Fragment)
	};*/
	std::array<Shader, 3> mesh_shaders = {
		Shader((shad_dir / "simple_mesh.vert"), Shader::Type::Vertex),
		Shader((shad_dir / "generate_face_normal.geom"), Shader::Type::Geometry),
		Shader((shad_dir / "mesh.frag"), Shader::Type::Fragment)
	};
	m_mesh_draw_program = ShaderProgram(mesh_shaders.data(), (uint32_t)mesh_shaders.size());

}

void SimulatedGameObject::start_simulation(Context& ctx)
{
	this->GameObject::start_simulation(ctx);

	if (!m_mesh) {
		return;
	}

	// Disable gizmos
	m_disable_interaction = true;

	m_mesh->apply_transform(this->get_model_matrix());
	m_transform.set_identity();

	// TODO
	ctx.get_simulator().add_simulated_object(this);
}

void SimulatedGameObject::render(const Context& ctx) const
{
	glm::mat4 model = get_model_matrix();
	glm::mat3 inv_t = glm::transpose(glm::inverse(glm::mat3(model)));

	m_mesh_draw_program.use_program();
	assert(glGetError() == GL_NO_ERROR);

	ShaderProgram::setUniform(0, model);
	ShaderProgram::setUniform(1, inv_t);
	ShaderProgram::setUniform(2, ctx.camera().getProjView());

	m_mesh->draw_triangles();

	m_selector.render(ctx, *this);
}

void SimulatedGameObject::render_ui(const Context& gc)
{
	this->GameObject::render_ui(gc);

	ImGui::Separator();

	if (!m_mesh) {
		return;
	}

	ImGui::PushID(m_name.c_str());


	if (ImGui::TreeNode("Mesh ops##mesh ops")) {
		ImGui::BeginDisabled(m_disable_interaction);

		if (ImGui::Button("Flip face orientation")) {
			m_mesh->flip_face_orientation();
		}
		if (ImGui::Button("Recompute normals")) {
			m_mesh->generate_normals();
		}

		ImGui::EndDisabled();
		ImGui::TreePop();
	}

	ImGui::Separator();


	m_selector.render_ui(gc, this);

	ImGui::PopID();
}

void SimulatedGameObject::update(const Context& gc)
{
	this->GameObject::update(gc);

	if (!m_mesh) {
		return;
	}

	m_selector.update(gc, this);
}

void SimulatedGameObject::late_update(const Context& gc)
{
	this->GameObject::late_update(gc);

	if (!m_mesh) {
		return;
	}

	m_selector.late_update(gc, this);
	m_mesh->update();
}


bool SimulatedGameObject::load_tetgen(const std::filesystem::path& path, std::string* out_err)
{
	m_name = path.stem().string();
	m_mesh = std::make_shared<TetMesh>();
	return m_mesh->load_tetgen(path, out_err);;
}


template<class Archive>
void SimulatedGameObject::serialize(Archive& archive)
{
	archive(cereal::base_class<GameObject>(this));

	archive(TF_SERIALIZE_NVP_MEMBER(m_mesh));
	archive(TF_SERIALIZE_NVP_MEMBER(m_selector));
}


TF_SERIALIZE_TYPE(SimulatedGameObject)
TF_SERIALIZE_POLYMORPHIC_RELATION(GameObject, SimulatedGameObject)

TF_SERIALIZE_TEMPLATE_EXPLICIT_IMPLEMENTATION(SimulatedGameObject)