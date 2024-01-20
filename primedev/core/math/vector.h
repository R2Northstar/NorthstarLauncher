#include <cmath>

#pragma once

struct Vector3
{
	float x, y, z;

	void MakeValid()
	{
		if (std::isnan(x))
			x = 0;
		if (std::isnan(y))
			y = 0;
		if (std::isnan(z))
			z = 0;
	}

	// todo: more operators maybe
	bool operator==(const Vector3& other)
	{
		return x == other.x && y == other.y && z == other.z;
	}
};

struct QAngle
{
	float x, y, z, w;

	// todo: more operators maybe
	bool operator==(const QAngle& other)
	{
		return x == other.x && y == other.y && z == other.z && w == other.w;
	}
};
