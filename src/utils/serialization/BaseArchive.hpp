#pragma once


class Context;
namespace tf {

class BaseArchive {
public:
	BaseArchive(const Context& ctx) : m_ctx(ctx) {}

private:
	const Context& m_ctx;
};

} // namespace tf