#pragma once

#include <filesystem>

class Context;
namespace tf {

class BaseArchive {
public:
	BaseArchive(const Context& ctx, 
		const std::filesystem::path& scene_path) : m_ctx(ctx), m_scene_path(scene_path) {
		assert(m_scene_path.is_absolute());
	}

	const std::filesystem::path& save_path() const { return m_scene_path; }

private:
	const Context& m_ctx;
	const std::filesystem::path m_scene_path;
};

} // namespace tf