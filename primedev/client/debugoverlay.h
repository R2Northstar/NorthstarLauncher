#pragma once

// Render Line
typedef void (*RenderLineType)(const Vector3& v1, const Vector3& v2, Color c, bool bZBuffer);
inline RenderLineType RenderLine;

// Render box
typedef void (*RenderBoxType)(
	const Vector3& vOrigin, const QAngle& angles, const Vector3& vMins, const Vector3& vMaxs, Color c, bool bZBuffer, bool bInsideOut);
inline RenderBoxType RenderBox;

// Render wireframe box
inline RenderBoxType RenderWireframeBox;

// Render swept box
typedef void (*RenderWireframeSweptBoxType)(
	const Vector3& vStart, const Vector3& vEnd, const QAngle& angles, const Vector3& vMins, const Vector3& vMaxs, Color c, bool bZBuffer);
inline RenderWireframeSweptBoxType RenderWireframeSweptBox;

// Render Triangle
typedef void (*RenderTriangleType)(const Vector3& p1, const Vector3& p2, const Vector3& p3, Color c, bool bZBuffer);
inline RenderTriangleType RenderTriangle;

// Render Axis
typedef void (*RenderAxisType)(const Vector3& vOrigin, float flScale, bool bZBuffer);
inline RenderAxisType RenderAxis;

// I dont know
typedef void (*RenderUnknownType)(const Vector3& vUnk, float flUnk, bool bUnk);
inline RenderUnknownType RenderUnknown;

// Render Sphere
typedef void (*RenderSphereType)(const Vector3& vCenter, float flRadius, int nTheta, int nPhi, Color c, bool bZBuffer);
inline RenderSphereType RenderSphere;
