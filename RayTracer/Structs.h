#pragma once

#include "Vector3.cpp"

struct Sphere {
	Sphere() = default;
	Sphere(Vector3 center, double radius, Vector3 color, double specularExp, double reflection) {
		this->center = center;
		this->radius = radius;
		this->color = color;
		this->specularExp = specularExp;
		this->reflection = reflection;
	}
	Vector3 center;
	double radius;
	Vector3 color;
	double specularExp;
	double reflection;
};

enum LightType { AMBIENT_L, POINT_L, DIRECTIONAL_L };
struct Light {
	Light() = default;
	Light(LightType type, double intensity, Vector3 direction) {
		this->type = type;
		this->intensity = intensity;
		this->direction = direction;
	}

	// Convenience constructor for ambient light
	Light(LightType type, double intensity) {
		this->type = type;
		this->intensity = intensity;
		this->direction = Vector3(0, 0, 0);
	}
	LightType type;
	double intensity;
	Vector3 direction;
};

struct QFSolutions {
	QFSolutions() = default;
	QFSolutions(double t1, double t2) {
		this->t1 = t1;
		this->t2 = t2;
	}
	double t1;
	double t2;
};

struct RayIntersection {
	RayIntersection() = default;
	RayIntersection(Sphere sphere, double t) {
		this->sphere = sphere;
		this->t = t;
	}
	Sphere sphere;
	double t;
};