#pragma once

#include <vector>

template<typename T>
class CircularBuffer {
public:
	CircularBuffer(size_t max_size = 2000) : 
		m_max_size(max_size) {
		m_buffer.reserve(m_max_size);
	}

	void push(const T& e) {
		if (m_buffer.size() < m_max_size) {
			m_buffer.push_back(e);
		}
		else {
			m_buffer[m_next] = e;
			m_next = (m_next + 1) % m_max_size;
		}
	}

	const T& back() const {
		if (m_buffer.size() < m_max_size) {
			return m_buffer.back();
		}
		else {
			return m_buffer[m_next == 0 ? m_max_size - 1 : m_next - 1];
		}
	}

	const T* data() const { return m_buffer.data(); }
	size_t size() const { return m_buffer.size(); }
	size_t offset() const { return m_next; }

private:
	size_t m_max_size;
	size_t m_next = 0;
	std::vector<T> m_buffer;
};