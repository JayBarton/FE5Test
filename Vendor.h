#pragma once
#include <vector>

//Not sure if the vendor needs any properties besides this vector. If it doesn't I can just use that vector, we'll see.
struct Vendor
{
	std::vector<int> items;
};