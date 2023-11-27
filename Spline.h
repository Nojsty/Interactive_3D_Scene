#pragma once
#include "pgr.h"


bool isVectorNull(const glm::vec3 &vect);

glm::mat4 alignObject(const glm::vec3 &position, const glm::vec3 &front, const glm::vec3 &up);

glm::vec3 evaluateCurveSegment(
	const glm::vec3& P0,
	const glm::vec3& P1,
	const glm::vec3& P2,
	const glm::vec3& P3,
	const float t
);

glm::vec3 evaluateCurveSegment_1stDerivative(
	const glm::vec3& P0,
	const glm::vec3& P1,
	const glm::vec3& P2,
	const glm::vec3& P3,
	const float t
);

glm::vec3 evaluateClosedCurve(
	const glm::vec3 points[],
	const size_t    count,			// = N
	const float     t
);

glm::vec3 evaluateClosedCurve_1stDerivative(
	const glm::vec3 points[],
	const size_t    count,			// = N
	const float     t
);

/// Cyclic clamping of a value.
/**
Makes sure that value is not outside the internal [\a minBound, \a maxBound].
If \a value is outside the interval it treated as periodic value with period equal to the size
of the interval. A necessary number of periods are added/subtracted to fit the value to the interval.

\param[in]  value              Value to be clamped.
\param[in]  minBound           Minimum bound of value.
\param[in]  maxBound           Maximum bound of value.
\return                        Value within range [minBound, maxBound].
\pre                           \a minBound is not greater that \maxBound.
*/
template <typename T>
T cyclic_clamp(const T value, const T minBound, const T maxBound) {

	T amp = maxBound-minBound;
	T val = fmod(value-minBound, amp);

	if (val < T(0))
		val += amp;

	return val+minBound;
}