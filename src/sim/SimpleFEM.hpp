#pragma once

#include "IFEM.hpp"

#include <Eigen/Sparse>
#include <Eigen/Dense>


namespace sim {

class SimpleFem : public IFEM {
public:


	void step(Float dt) override final;

private:

	typedef Eigen::SparseMatrix<Float> Mat;
	typedef Eigen::Matrix<Float, 0, 1> Vec;

};

} // namespace sim