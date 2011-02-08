#ifndef __BUFFER_INCLUDE_GUARD__
#define __BUFFER_INCLUDE_GUARD__

#include <vector>
#include <memory>

namespace evx
{
	class buffer : protected std::vector<char>
	{
	public:
		typedef std::vector<char> base;
		typedef base::iterator iterator;
		typedef std::pair<iterator, iterator> range;
	
	private:
		iterator read_pos;
	
	public:
		buffer()
		 : read_pos(begin())
		{}
	
		// prepares a range of bytes at the end of the buffer of count bytes
		// and returns the iterator range. Use set_write_end(end) to truncate the
		// buffer after you have written the bytes you needed to write to it.
		// This will invalidate outstanding iterators. Eg:
		//  buffer_range output = buffer.prepare_write(1024);
		//  count = read(fd, &*output.first, 1024);
		//  set_write_end(output.first + count);
		range prepare_write(size_t count);
	
		// Truncates the buffer because your write buffer was larger than needed to store available
		// data.
		void set_write_end(iterator end)
		{
			resize(end - begin());
		}
	
		iterator read_begin()
		{
			return read_pos;
		}
	
		iterator read_end()
		{
			return end();
		}
	
		// Sets the buffer position that will be returned by the next call to read_begin().
		// This allows for the library to remove any data before the set begin iterator, 
		// and if it does truncate will cause an invalidation of outstanding iterators.
		iterator set_read_begin(iterator new_begin)
		{
			read_pos = new_begin;
			return read_pos;
		}
	
		// Removes all data from the buffer.
		void drain()
		{
			clear();
			read_pos = begin();
		}
	};
}
#endif