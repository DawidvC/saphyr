/* Saphyr, a C++ style compiler using LLVM
 * Copyright (C) 2009-2016, Justin Madru (justin.jdm64@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __NODE_H__
#define __NODE_H__

#include <vector>

using namespace std;

class Node
{
public:
	virtual ~Node() {};
};

template<typename T>
class NodeList
{
	typedef vector<T*> container;
	typedef typename container::iterator iterator;

	bool doDelete;

protected:
	container list;

public:
	explicit NodeList(bool doDelete = true)
	: doDelete(doDelete) {}

	template<typename L>
	L* move()
	{
		auto other = new L;
		for (const auto item : *this)
			other->add(item);

		doDelete = false;
		delete this;

		return other;
	}

	void setDelete(bool DoDelete)
	{
		doDelete = DoDelete;
	}

	void reserve(size_t size)
	{
		list.reserve(size);
	}

	void clear()
	{
		list.clear();
	}

	bool empty() const
	{
		return list.empty();
	}

	int size() const
	{
		return list.size();
	}

	iterator begin()
	{
		return list.begin();
	}

	iterator end()
	{
		return list.end();
	}

	void add(T* item)
	{
		list.push_back(item);
	}

	void addFront(T* item)
	{
		list.insert(list.begin(), item);
	}

	void addAll(NodeList<T>& other)
	{
		list.insert(list.end(), other.begin(), other.end());
	}

	T* at(int i)
	{
		return list.at(i);
	}

	T* front()
	{
		return list.empty()? nullptr : list.front();
	}

	T* back()
	{
		return list.empty()? nullptr : list.back();
	}

	~NodeList()
	{
		if (doDelete) {
			for (auto i : list)
				delete i;
		}
	}
};

#endif
