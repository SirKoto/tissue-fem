#include "StaticColliderGameObject.hpp"

#include "Context.hpp"

StaticColliderGameObject::StaticColliderGameObject()
	: GameObject()
{
	std::vector<glm::vec3> vert = {
		glm::vec3(-1.0f, 0.0f, -1.0f),
		glm::vec3(1.0f, 0.0f, -1.0f),
		glm::vec3(-1.0f, 0.0f, 1.0f),
		glm::vec3(1.0f, 0.0f, 1.0f)
	};
	std::vector<glm::ivec3> faces = {
		glm::vec3(0, 2, 1),
		glm::vec3(1, 2, 3)
	};
	m_mesh.set_mesh(vert, faces);

	const std::filesystem::path proj_dir(PROJECT_DIR);
	const std::filesystem::path shad_dir = proj_dir / "resources/shaders";

	std::array<Shader, 3> mesh_shaders = {
		Shader((shad_dir / "simple_mesh.vert"), Shader::Type::Vertex),
		Shader((shad_dir / "generate_face_normal.geom"), Shader::Type::Geometry),
		Shader((shad_dir / "mesh.frag"), Shader::Type::Fragment)
	};
	m_mesh_draw_program = ShaderProgram(mesh_shaders.data(), (uint32_t)mesh_shaders.size());
}

void StaticColliderGameObject::start_simulation(Context& ctx)
{
	glm::vec2 max_uvs;
	glm::vec3 n = glm::vec3(get_model_matrix() * glm::vec4(0.f, 1.f, 0.f, 0.f));
	n = glm::normalize(n);
	glm::vec3 right = glm::vec3(get_model_matrix() * glm::vec4(1.f, 0.f, 0.f, 0.f));
	max_uvs[0] = glm::length(right);
	max_uvs[1] = glm::length(glm::vec3(get_model_matrix() * glm::vec4(0.f, 0.f, 1.f, 0.f)));

	right = glm::normalize(right);
	glm::vec3 cross = glm::cross(n, right);

	ctx.insert_static_physics_primitive(std::make_shared<Plane>(
		get_model_matrix()[3], n, right, cross, max_uvs));
}

void StaticColliderGameObject::render(const Context& ctx) const
{
	glm::mat4 model = get_model_matrix();
	glm::mat3 inv_t = glm::transpose(glm::inverse(glm::mat3(model)));

	m_mesh_draw_program.use_program();
	assert(glGetError() == GL_NO_ERROR);

	ShaderProgram::setUniform(0, model);
	ShaderProgram::setUniform(1, inv_t);
	ShaderProgram::setUniform(2, ctx.camera().getProjView());

	ShaderProgram::setUniform(3, m_color);

	m_mesh.draw_triangles();
}

void StaticColliderGameObject::render_ui(const Context& gc)
{
	this->GameObject::render_ui(gc);

	ImGui::ColorEdit3("Color", glm::value_ptr(m_color));
}

void StaticColliderGameObject::update(const Context& gc)
{
	this->GameObject::update(gc);
}



template<class Archive>
void StaticColliderGameObject::serialize(Archive& archive)
{
	archive(cereal::base_class<GameObject>(this));

	archive(TF_SERIALIZE_NVP_MEMBER(m_color));
}


TF_SERIALIZE_TYPE(StaticColliderGameObject)
TF_SERIALIZE_POLYMORPHIC_RELATION(GameObject, StaticColliderGameObject)

TF_SERIALIZE_TEMPLATE_EXPLICIT_IMPLEMENTATION(StaticColliderGameObject)