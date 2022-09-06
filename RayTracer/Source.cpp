#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <Windows.h>
#include "structs.h"

const LPCWSTR WNDCLASSNAME = L"GRAPHICS!";
const int WINDOW_WIDTH = 700;
const int WINDOW_HEIGHT = 700;
const int MAX_TRACE_RECURSIONS = 2;
bool quit = false;
bool draw = true;

HDC hDc;
HWND hWnd;

Sphere spheres[] = {
	Sphere(Vector3(0, -1, 3.5), 1.0, Vector3(255, 0, 0), 60.0, 0.15),		 // Red
	Sphere(Vector3(2, 0, 5.0), 1.0, Vector3(0, 0, 255), 700.0, 0.35),		 // Blue
	Sphere(Vector3(-2, 0.5, 4.5), 1.0, Vector3(0, 255, 0), 400.0, 0.3),		 // Green
	Sphere(Vector3(0, -5001, 0), 5000, Vector3(255, 255, 0), 1000.0, 0.20),  // Yellow (floor)
	Sphere(Vector3(0, 3.0, 14.0), 2.0, Vector3(255, 255, 255), 100.0, 0.55)  // White
};

Light lights[] = {
	Light(LightType::AMBIENT_L, 0.2),
	Light(LightType::POINT_L, 0.6, Vector3(2, 1, 0)),
	Light(LightType::DIRECTIONAL_L, 0.25, Vector3(1, 4, 4))
};

// Converts (0,0)-is-top-left, increasing-y-goes-down coord to standard graph
// Necessary to make the viewport perspective correct
Vector3 WindowToCanvasCoord(int x, int y) {
	return Vector3(x - WINDOW_WIDTH / 2, -y + WINDOW_HEIGHT / 2, 0);
}

Vector3 CanvasToWindowCoord(int x, int y) {
	return Vector3(x + WINDOW_WIDTH / 2, -(y - WINDOW_HEIGHT / 2), 0);
}

Vector3 CanvasToViewportCoord(int canvasX, int canvasY, int vpWidth, int vpHeight) {
	double wRatio = (double)vpWidth / WINDOW_WIDTH;
	double hRatio = (double)vpHeight / WINDOW_HEIGHT;
	return Vector3(canvasX * wRatio, canvasY * hRatio, 1);
}

// INFINITY indicates no solution. t2 = INFINITY means one solution
QFSolutions SolveQuadraticEquation(double a, double b, double c) {
	double discriminant = (b * b) - (4 * a * c);
	if (discriminant < 0) {
		return QFSolutions(INFINITY, INFINITY);
	}

	double t1 = (-b + sqrt(discriminant)) / (2 * a);

	if (discriminant == 0.0) {
		return QFSolutions(t1, INFINITY);
	}

	double t2 = (-b - sqrt(discriminant)) / (2 * a);

	return QFSolutions(t1, t2);
}

RayIntersection ClosestIntersection(const Vector3& origin, const Vector3& dir, double tMin, double tMax) {
	int numSpheres = sizeof(spheres) / sizeof(spheres[0]);
	double closestT = INFINITY;
	Sphere closestSphere = spheres[0];
	
	for (int i = 0; i < numSpheres; i++) {
		Sphere currSphere = spheres[i];
		Vector3 sphereToOrigin = origin - currSphere.center;

		double a = dir.Dot(dir);
		double b = 2 * sphereToOrigin.Dot(dir);
		double c = sphereToOrigin.Dot(sphereToOrigin) - (currSphere.radius * currSphere.radius);

		QFSolutions tVals = SolveQuadraticEquation(a, b, c);

		// Solution doesn't exist if it equals INFINITY
		if (tVals.t1 != INFINITY && tVals.t1 > tMin && tVals.t1 < tMax) {
			if (tVals.t1 < closestT) {
				closestT = tVals.t1;
				closestSphere = spheres[i];
			}
		}

		if (tVals.t2 != INFINITY && tVals.t2 > tMin && tVals.t2 < tMax) {
			if (tVals.t2 < closestT) {
				closestT = tVals.t2;
				closestSphere = spheres[i];
			}
		}
	}

	return RayIntersection(closestSphere, closestT);
}

Vector3 ReflectAcrossNormal(const Vector3& ray, const Vector3& normal) {
	return 2 * normal * normal.Dot(ray) - ray;
}

double CalculateLightAtPoint(
		const Vector3& point, 
		const Vector3& normal, 
		const Vector3& camera, 
		double specularExp) 
{
	double totalIntensity = 0.0;
	int numLights = sizeof(lights) / sizeof(lights[0]);
	for (int i = 0; i < numLights; i++) {
		Light currLight = lights[i];
		if (currLight.type == LightType::AMBIENT_L) {
			totalIntensity += currLight.intensity;
		}
		else {
			// Calculate the diffusion for this point
			Vector3 pointToLight;
			int tMax;
			if (currLight.type == LightType::POINT_L) {
				pointToLight = currLight.direction - point;
				tMax = 1.0;
			}
			else if (currLight.type == LightType::DIRECTIONAL_L) {
				pointToLight = currLight.direction;
				tMax = INFINITY;
			}
			else {
				printf("ERROR! Unknown light type: %d", currLight.type);
				return 1.0; // Don't change existing light in case of error
			}

			// Check if this light source is obstructed
			RayIntersection blocker = ClosestIntersection(point, pointToLight, 0.0001, tMax);

			if (blocker.t != INFINITY) {
				break;
			}

			double normalDotDir = normal.Dot(pointToLight);
			if (normalDotDir > 0) {
				double diffuse = currLight.intensity * normalDotDir 
								/ (normal.Magnitude() * pointToLight.Magnitude());
				totalIntensity += diffuse;
			}
			
			if (specularExp > 0.0) {
				Vector3 pointToCamera = camera - point;
				Vector3 reflection = ReflectAcrossNormal(pointToLight, normal);
				if (reflection.Dot(pointToCamera) > 0) {
					double specular = currLight.intensity * pow(
						reflection.Dot(pointToCamera)
						/ (reflection.Magnitude() * pointToCamera.Magnitude()), specularExp
					);
					totalIntensity += specular;
				}
			}
		}
	}

	// Clamp at 1.0 because Windows will overflow RGB values over 255 with mod 255.
	return min(totalIntensity, 1.0);
}

Vector3 TraceRay(const Vector3& origin, const Vector3& to, double tMin, double tMax, int recursions) {
	Vector3 defaultColor = Vector3(45, 0, 65);
	RayIntersection closest = ClosestIntersection(origin, to, tMin, tMax);

	if (closest.t == INFINITY) {
		// Ray did not intersect any sphere in the scene
		return defaultColor;
	}

	Vector3 point = origin + closest.t * to; // Get intersect point from t
	Vector3 intersectNormal = (point - closest.sphere.center).Normalized();
	Vector3 localColor = closest.sphere.color *
		CalculateLightAtPoint(point, intersectNormal, -1 * to, closest.sphere.specularExp);

	if (closest.sphere.reflection <= 0.0 || recursions >= MAX_TRACE_RECURSIONS) {
		return localColor;
	}
	Vector3 reflectedColor = TraceRay(
		point, ReflectAcrossNormal(-to, intersectNormal), 0.01, tMax, recursions + 1);

	return localColor * (1 - closest.sphere.reflection) + reflectedColor * closest.sphere.reflection;
}


// Processes all messages sent to the window.
LRESULT CALLBACK WinProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
	switch (Msg) {
		// Sent when window is closed
		case WM_CLOSE: {
			PostQuitMessage(0);
			break;
		}
	}
	// Send everything else to default processor
	return DefWindowProc(hWnd, Msg, wParam, lParam);
}

// This is mostly just setting window params and then the main loop
int WINAPI WinMain(
	HINSTANCE hInstance, 
	HINSTANCE hPrevInstance, 
	LPSTR lpCmdLine, 
	int nShowCmd) 
{
	MSG msg;
	WNDCLASSEX ex;
	ex.cbSize = sizeof(WNDCLASSEX);
	ex.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC; // combine styles w/ bitwise OR
	ex.lpfnWndProc = WinProc;
	ex.cbClsExtra = 0;
	ex.cbWndExtra = 0;
	ex.hInstance = hInstance;
	ex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	ex.hCursor = LoadCursor(NULL, IDC_ARROW);
	ex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	ex.lpszMenuName = NULL;
	ex.lpszClassName = (LPCWSTR)WNDCLASSNAME;
	ex.hIconSm = NULL;
	RegisterClassEx(&ex);
	
	// Finally making the window
	hWnd = CreateWindowEx(
		NULL, 
		(LPCWSTR)WNDCLASSNAME, 
		L"Graphical!",
		WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX ^ WS_MINIMIZEBOX,
		100, 100, WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL, hInstance, NULL);
	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);

	hDc = GetDC(hWnd);
	Vector3 cameraCoord = Vector3(0, 0, 0);

	// Message loop
	while (!quit) {
		if (draw) {
			for (int x = 0; x < WINDOW_WIDTH; x++) {
				for (int y = 0; y < WINDOW_HEIGHT; y++) {
					Vector3 canvasCoord = WindowToCanvasCoord(x, y);
					Vector3 vpCoord = CanvasToViewportCoord(canvasCoord.x, canvasCoord.y, 1, 1);
					Vector3 color = TraceRay(cameraCoord, vpCoord, 1.0, INFINITY, 0);
					SetPixel(hDc, x, y, RGB(color.x, color.y, color.z));
				}
			}
			draw = false;
		}
		if (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE)) {
			quit = (msg.message == WM_QUIT);
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	ReleaseDC(hWnd, hDc);

	return msg.lParam;
}




int main() {
	printf("Ages ago, life was born in the primitive sea.\n");

	srand(time(NULL));
	
	return WinMain(GetModuleHandle(NULL), NULL, NULL, 0);
}