#include "evx/buffer.hpp"

namespace evx
{
	buffer::range buffer::prepare_write(size_t count)
	{
		size_t relative_pos = read_pos - begin();
		resize(size() + count);
		read_pos = begin() + relative_pos;
		return range(end() - count, end());
	}
}
