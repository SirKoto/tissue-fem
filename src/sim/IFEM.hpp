#pragma once

#include <Eigen/Dense>
#include <Eigen/Sparse>

namespace sim
{

typedef double Float;
typedef Eigen::SparseMatrix<Float> SMat;
typedef Eigen::Matrix<Float, Eigen::Dynamic, 1> Vec;
typedef Eigen::Matrix<Float, 3, 1> Vec3;
typedef Eigen::Matrix<Float, 9, 1> Vec9;
typedef Eigen::Matrix<Float, 12, 1> Vec12;
typedef Eigen::Vector4i Vec4i;
typedef Eigen::Matrix<Float, 3, 3> Mat3;
typedef Eigen::Matrix<Float, 9, 9> Mat9;
typedef Eigen::Matrix<Float, 12, 12> Mat12;
typedef Eigen::Matrix<Float, 9, 12> Mat9x12;


Mat9x12 compute_dFdx(const Mat3& DmInv);

inline Mat3 compute_Ds(const Vec4i& element, const std::vector<Vec3>& nodes) {
	Mat3 Ds;
	Ds.col(0) = nodes[element(1)] - nodes[element(0)];
	Ds.col(1) = nodes[element(2)] - nodes[element(0)];
	Ds.col(2) = nodes[element(3)] - nodes[element(0)];

	return Ds;
}

Vec9 vec(const Mat3& m);
Mat3 cross_matrix(const Vec3& v);

inline Float compute_I1(const Mat3& S) { return S.trace(); }
inline Float compute_I2(const Mat3& F) { return (F.transpose() * F).trace(); }
inline Float compute_I3(const Mat3& F) { return F.determinant(); }

inline Vec9 compute_g1(const Mat3& R) { return vec(R); }
inline Vec9 compute_g2(const Mat3& F) { return Float(2.0) * vec(F); }
Vec9 compute_g3(const Mat3& F);

Mat9 compute_H1(const Mat3& U, const Vec3& singular_values, const Mat3& V);
inline Mat9 compute_H2() { return Float(2.0) * Mat9::Identity(); }
Mat9 compute_H3(const Mat3& F);

class IFEM {
public:

	virtual void step(Float dt) = 0;

	virtual void update_objects() = 0;
};

} // namespace sim