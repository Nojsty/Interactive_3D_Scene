#include "Spline.h"

// control points of curve
glm::vec3 curveData[] = { 
	glm::vec3 ( -5.6f, 3.0f, -6.3f ), 
	glm::vec3 ( -6.8f, 2.0f, -8.7f ),
	glm::vec3 ( -8.7f, 3.0f, -9.3f ), 
	glm::vec3 ( -9.2f, 3.0f, -7.0f ),
	glm::vec3 ( -9.7f, 2.0f, -5.1f ),
	glm::vec3 ( -7.4f, 1.0f, -4.7f ),
	glm::vec3 ( -5.1f, 2.0f, -2.0f )
};

// number of control points
size_t curveSize = 7;

//=================================================================================

bool isVectorNull ( const glm::vec3 &vect ) 
{
	return !vect.x && !vect.y && !vect.z;
}

/// Align (rotate and move) current coordinate system to given parameters.
/**
This function works similarly to \ref gluLookAt, however it is used for object transform
rather than view transform. The current coordinate system is moved so that origin is moved
to the \a position. Object's local front (-Z) direction is rotated to the \a front and
object's local up (+Y) direction is rotated so that angle between its local up direction and
\a up vector is minimum.

\param[in]  position           Position of the origin.
\param[in]  front              Front direction.
\param[in]  up                 Up vector.
*/
glm::mat4 alignObject(const glm::vec3 &position, const glm::vec3 &front, const glm::vec3 &up)
{

	glm::vec3 z = -glm::normalize(front);

	if (isVectorNull(z))
		z = glm::vec3(0.0, 0.0, 1.0);

	glm::vec3 x = glm::normalize(glm::cross(up, z));

	if (isVectorNull(x))
		x = glm::vec3(1.0, 0.0, 0.0);

	glm::vec3 y = glm::cross(z, x);
	//mat4 matrix = mat4(1.0f);
	glm::mat4 matrix = glm::mat4(
		x.x, x.y, x.z, 0.0,
		y.x, y.y, y.z, 0.0,
		z.x, z.y, z.z, 0.0,
		position.x, position.y, position.z, 1.0
	);

	return matrix;
}


glm::vec3 evaluateCurveSegment(
	const glm::vec3& P0,
	const glm::vec3& P1,
	const glm::vec3& P2,
	const glm::vec3& P3,
	const float t
) {
	glm::vec3 result(0.0, 0.0, 0.0);

	float a = pow(t, 3) * -1.0f + pow(t, 2) * 2.0f + t * -1.0f + 1 * 0.0f;
	float b = pow(t, 3) * 3.0f + pow(t, 2) * -5.0f + t * 0.0f + 1 * 2.0f;
	float c = pow(t, 3) * -3.0f + pow(t, 2) * 4.0f + t * 1.0f + 1 * 0.0f;
	float d = pow(t, 3) * 1.0f + pow(t, 2) * -1.0f + t * 0.0f + 1 * 0.0f;

	result = a * P0 + b * P1 + c * P2 + d * P3;
	result /= 2;

	return result;
}

glm::vec3 evaluateCurveSegment_1stDerivative(
	const glm::vec3& P0,
	const glm::vec3& P1,
	const glm::vec3& P2,
	const glm::vec3& P3,
	const float t
) {
	glm::vec3 result(1.0, 0.0, 0.0);

	float a = 3 * pow(t, 2) * -1.0f + 2 * t * 2.0f + 1 * -1.0f + 0 * 0.0f;
	float b = 3 * pow(t, 2) * 3.0f + 2 * t * -5.0f + 1 * 0.0f + 0 * 2.0f;
	float c = 3 * pow(t, 2) * -3.0f + 2 * t * 4.0f + 1 * 1.0f + 0 * 0.0f;
	float d = 3 * pow(t, 2) * 1.0f + 2 * t * -1.0f + 1 * 0.0f + 0 * 0.0f;

	result = a * P0 + b * P1 + c * P2 + d * P3;
	result /= 2;

	return result;
}

glm::vec3 evaluateClosedCurve(
	const glm::vec3 points[],
	const size_t    count,			// = N
	const float     t
) {
	glm::vec3 result(0.0, 0.0, 0.0);

	size_t i = (size_t)t;

	// (a % b + b) % b;
	size_t modI = ((i - 1) + count) % count;

	glm::vec3 P0 = points[modI];
	glm::vec3 P1 = points[i % count];
	glm::vec3 P2 = points[(i + 1) % count];
	glm::vec3 P3 = points[(i + 2) % count];

	float returnT = t - (int)t;

	result = evaluateCurveSegment(P0, P1, P2, P3, returnT);

	return result;
}

glm::vec3 evaluateClosedCurve_1stDerivative(
	const glm::vec3 points[],
	const size_t    count,			// = N
	const float     t
) {
	glm::vec3 result(1.0, 0.0, 0.0);

	size_t i = (size_t)t;
	size_t modI = ((i - 1) + count) % count;

	glm::vec3 P0 = points[modI];
	glm::vec3 P1 = points[i % count];
	glm::vec3 P2 = points[(i + 1) % count];
	glm::vec3 P3 = points[(i + 2) % count];

	float returnT = t - (int)t;

	result = evaluateCurveSegment_1stDerivative(P0, P1, P2, P3, returnT);

	return result;
}
