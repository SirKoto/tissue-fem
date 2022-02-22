#include "IFEM.hpp"

#include <glm/gtc/constants.hpp>


namespace sim {



Mat9x12 compute_dFdx(const Mat3& DmInv)
{
	const Float m = DmInv(0, 0);
	const Float n = DmInv(0, 1);
	const Float o = DmInv(0, 2);
	const Float p = DmInv(1, 0);
	const Float q = DmInv(1, 1);
	const Float r = DmInv(1, 2);
	const Float s = DmInv(2, 0);
	const Float t = DmInv(2, 1);
	const Float u = DmInv(2, 2);

	const Float t1 = -m - p - s;
	const Float t2 = -n - q - t;
	const Float t3 = -o - r - u;

	Mat9x12 PFPx;
	PFPx.setZero();
	PFPx(0, 0) = t1;
	PFPx(0, 3) = m;
	PFPx(0, 6) = p;
	PFPx(0, 9) = s;
	PFPx(1, 1) = t1;
	PFPx(1, 4) = m;
	PFPx(1, 7) = p;
	PFPx(1, 10) = s;
	PFPx(2, 2) = t1;
	PFPx(2, 5) = m;
	PFPx(2, 8) = p;
	PFPx(2, 11) = s;
	PFPx(3, 0) = t2;
	PFPx(3, 3) = n;
	PFPx(3, 6) = q;
	PFPx(3, 9) = t;
	PFPx(4, 1) = t2;
	PFPx(4, 4) = n;
	PFPx(4, 7) = q;
	PFPx(4, 10) = t;
	PFPx(5, 2) = t2;
	PFPx(5, 5) = n;
	PFPx(5, 8) = q;
	PFPx(5, 11) = t;
	PFPx(6, 0) = t3;
	PFPx(6, 3) = o;
	PFPx(6, 6) = r;
	PFPx(6, 9) = u;
	PFPx(7, 1) = t3;
	PFPx(7, 4) = o;
	PFPx(7, 7) = r;
	PFPx(7, 10) = u;
	PFPx(8, 2) = t3;
	PFPx(8, 5) = o;
	PFPx(8, 8) = r;
	PFPx(8, 11) = u;

	return PFPx;
}

Vec9 vec(const Mat3& m)
{
	Vec9 ret;
	ret << m(0, 0), m(1, 0), m(2, 0),
		m(0, 1), m(1, 1), m(2, 1),
		m(0, 2), m(1, 2), m(2, 2);

	return ret;
}

Mat3 cross_matrix(const Vec3& v)
{
	Mat3 m;
	m << 0, -v(2), v(1), 
		v(2), 0, -v(0),
		-v(1), v(0), 0;
	return m;
}

Vec9 compute_g3(const Mat3& F)
{
	Vec9 g3;

	g3.segment<3>(0) = F.col(1).cross(F.col(2));
	g3.segment<3>(3) = F.col(2).cross(F.col(0));
	g3.segment<3>(6) = F.col(0).cross(F.col(1));

	return g3;
}

Mat9 compute_H1(const Mat3& U, const Vec3& s, const Mat3& V)
{
	constexpr Float invSqrt2 = glm::one_over_root_two<Float>();

	Mat3 T0 = Mat3::Zero();
	T0(0, 1) = Float(-1.0);
	T0(1, 0) = Float(1.0);
	T0 = invSqrt2 * U * T0 * V.transpose();

	Mat3 T1 = Mat3::Zero();
	T1(2, 1) = Float(-1.0);
	T1(1, 2) = Float(1.0);
	T1 = invSqrt2 * U * T1 * V.transpose();

	Mat3 T2 = Mat3::Zero();
	T2(2, 0) = Float(-1.0);
	T2(0, 2) = Float(1.0);
	T2 = invSqrt2 * U * T2 * V.transpose();

	// flatten
	Vec9 t0 = vec(T0);
	Vec9 t1 = vec(T1);
	Vec9 t2 = vec(T2);

	Mat9 H = (Float(2) / (s.x() + s.y())) * (t0 * t0.transpose());
	H += (Float(2) / (s.y() + s.z())) * (t1 * t1.transpose());
	H += (Float(2) / (s.z() + s.x())) * (t2 * t2.transpose());

	return H;
}

Mat9 compute_H3(const Mat3& F)
{
	Mat9 H3 = Mat9::Zero();

	Mat3 f0 = cross_matrix(F.col(0));
	Mat3 f1 = cross_matrix(F.col(1));
	Mat3 f2 = cross_matrix(F.col(2));

	H3.block<3, 3>(0, 3) = -f2;
	H3.block<3, 3>(0, 6) = f1;
	H3.block<3, 3>(3, 0) = f2;
	H3.block<3, 3>(3, 6) = -f0;
	H3.block<3, 3>(6, 0) = -f1;
	H3.block<3, 3>(6, 3) = f0;

	return H3;
}

} // namespace sim