#include "GameObject.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <glad/glad.h>
#include <imgui.h>
#include <ImGuizmo.h>


#include "Context.hpp"

GameObject::GameObject()
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

bool GameObject::load_tetgen(const std::filesystem::path& path, std::string* out_err)
{
	m_name = path.stem().string();
	m_mesh = std::make_shared<TetMesh>();
	return m_mesh->load_tetgen(path, out_err);;
}

void GameObject::start_simulation(const Context& ctx)
{
	if (!m_mesh) {
		return;
	}

	m_mesh->apply_transform(this->get_model_matrix());
	m_transform.set_identity();

	m_sim.start_simulation(ctx, *this);
}

void GameObject::render(const Context& ctx) const
{
	glm::mat4 model = get_model_matrix();
	glm::mat3 inv_t = glm::transpose(glm::inverse(glm::mat3(model)));
	
	m_mesh_draw_program.use_program();
	assert(glGetError() == GL_NO_ERROR);

	ShaderProgram::setUniform(0, model);
	ShaderProgram::setUniform(1, inv_t);
	ShaderProgram::setUniform(2, ctx.camera().getProjView());

	m_mesh->draw_triangles();
}

void GameObject::render_ui(const Context& gc)
{
	if (!m_mesh) {
		return;
	}

	ImGui::PushID(m_name.c_str());

	ImGui::SetNextItemOpen(true, ImGuiCond_Once);
	if (ImGui::TreeNode("Transform##transform")) {
		ImGui::BeginDisabled(gc.has_simulation_started());

		m_transform.draw_ui(gc);

		ImGui::EndDisabled();
		ImGui::TreePop();
	}

	ImGui::Separator();

	if (ImGui::TreeNode("Mesh ops##mesh ops")) {
		ImGui::BeginDisabled(gc.has_simulation_started());

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

	/*decltype(m_addons)::iterator it_addon = m_addons.begin();
	while(it_addon != m_addons.end()){
		std::unique_ptr<gobj::Addon>& addon = *it_addon;
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		bool is_open = ImGui::TreeNode((void*)addon.get(), addon->get_name());
		// Item context menu
		if (ImGui::BeginPopupContextItem()) {
			if (ImGui::Selectable("Remove")) {
				it_addon = m_addons.erase(it_addon);
				ImGui::EndPopup();
				if (is_open) {
					ImGui::TreePop();
				}
				continue;
			}
			ImGui::EndPopup();
		}

		// Body of the tree node
		if(is_open){
			addon->render_ui(gc, this);
			ImGui::Separator();
			ImGui::TreePop();
		}
		

		++it_addon;
	}

	// Add addon
	ImGui::Button("Add addon");
	if (ImGui::BeginPopupContextItem(0, ImGuiPopupFlags_None)) {
		if (ImGui::Selectable("Elastic Simulator")) {
			this->add_addon<gobj::ElasticSim>();
			ImGui::CloseCurrentPopup();
		}
		if (ImGui::Selectable("Primitive Selector")) {
			this->add_addon<gobj::PrimitiveSelector>();
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
	*/

	if (ImGui::TreeNode("Elastic Simulator")) {
		m_sim.render_ui(gc, this);
		ImGui::Separator();
		ImGui::TreePop();
	}

	ImGui::PopID();
}

void GameObject::update(const Context& gc)
{
	if (!m_mesh) {
		return;
	}

	m_sim.update(gc, this);
}

void GameObject::late_update(const Context& gc)
{
	if (!m_mesh) {
		return;
	}

	m_sim.late_update(gc, this);
	m_mesh->update();
}


template<class Archive>
void GameObject::serialize(Archive& archive)
{
	archive(TF_SERIALIZE_NVP_MEMBER(m_name));
	archive(TF_SERIALIZE_NVP_MEMBER(m_transform));
	archive(TF_SERIALIZE_NVP_MEMBER(m_sim));
	archive(TF_SERIALIZE_NVP_MEMBER(m_mesh));
}

TF_SERIALIZE_TEMPLATE_EXPLICIT_IMPLEMENTATION(GameObject)