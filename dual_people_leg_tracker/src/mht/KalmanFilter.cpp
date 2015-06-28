/*
 * KalmanFilter.cpp
 *
 *  Created on: Jun 27, 2015
 *      Author: alex
 */

#include <dual_people_leg_tracker/mht/KalmanFilter.h>
#include <iostream>


using namespace mht;

KalmanFilter::KalmanFilter(Eigen::Matrix<double,2,1> initialState) {

	// Set the state
	initial_state_[0] = initialState[0];
	initial_state_[1] = initialState[1];
	initial_state_[2] = 0.0;
	initial_state_[3] = 0.0;

	// Current estimation is the initial state
	state_estimated_ = initial_state_;
	state_predicted_ = initial_state_;

	// Define the System matrix
	A_ = Eigen::Matrix<double,-1,-1>::Identity(4,4);


	// Define the measurement Matrix
	H_ << 1, 0, 0, 0,
		  0, 1, 0,  0;

	P_post_ <<  0.01, 0, 0, 0,
			    0, 0.01, 0, 0,
				0, 0, 1, 0,
				0, 0, 0, 1;
	P_prior_  = P_post_;

	// Process Covariance
	Q_ = Eigen::Matrix<double,-1,-1>::Identity(4,4)*0.01;

	// Measurement Covariance
	R_ = Eigen::Matrix<double,-1,-1>::Identity(2,2)*0.001;



//	std::cout << "A Kalman filter was created, current state " << std::endl << state_estimated_ <<  std::endl;
//	std::cout << "System is " << std::endl << A_ <<  std::endl;
//	std::cout << "Meas is " << std::endl << H_ <<  std::endl;
//	std::cout << "P_prior_ is " << std::endl << P_prior_ <<  std::endl;
std::cout << "Q_ is " << std::endl << Q_ <<  std::endl;
std::cout << "R_ is " << std::endl << R_ <<  std::endl;

}

/*KalmanFilter::KalmanFilter(const KalmanFilter &obj):
	initial_state_(obj.initial_state_),
	state_estimated_(obj.state_estimated_),
	state_predicted_(obj.state_predicted_),
	A_(obj.A_),
	H_(obj.H_),
	P_prior_(obj.P_prior_),
	P_post_(obj.P_post_),
	Q_(obj.Q_),
	R_(obj.R_)

{
    std::cout << "Copy of KalmanFilter" << std::endl;
}*/

KalmanFilter::~KalmanFilter() {
	//std::cout << "A KalmanFilter was removed " <<  std::endl;
}

// Predict the Kalman Filter dt_ forward
void KalmanFilter::predict(double dt){


	A_dt = A_;
	A_dt(0,2) = dt;
	A_dt(1,3) = dt;

	//std::cout << "Doing prediction with " << std::endl << A_dt << std::endl;

	state_predicted_ = A_dt * state_estimated_;

	P_prior_ = A_dt * P_post_ * A_dt.transpose() + Q_;

	//std::cout << "state_"  << std::endl << state_ << std::endl;
	std::cout << "state_estimated " << state_estimated_.transpose() << "  -- prediction(" << dt <<  "s) -->  " << state_predicted_.transpose() << std::endl;
}

void KalmanFilter::update(Eigen::Matrix<double,2,1> z_k){
	std::cout << "Kalman Filter was updated with " << z_k.transpose() << std::endl;



	// Residual
	Eigen::Matrix<double,2,1> y_k;
	y_k = z_k - H_ * state_predicted_;

	// Residual covariance
	S_k_ = H_ * P_prior_ * H_.transpose() + R_;

	// Kalman Gain
	Eigen::Matrix<double,4,2> K_k;
	K_k = P_prior_ * H_.transpose() * S_k_.inverse();

	// State estimation
	state_estimated_ = state_predicted_ + K_k * y_k;

	// Update (a posterior) state covariance
	Eigen::Matrix<double,4,4> I = Eigen::Matrix<double,4,4>::Identity();
	P_post_ = (I - K_k * H_) + P_prior_;

	std::cout << state_predicted_.transpose() << "  -->Update   " << state_estimated_.transpose() << std::endl;
}

Eigen::Matrix<double,4,1> KalmanFilter::getPrediction(){
	return this->state_predicted_;
}

Eigen::Matrix<double,4,1> KalmanFilter::getEstimation(){
	return this->state_estimated_;
}

Eigen::Matrix<double,2,1> KalmanFilter::getMeasurementPrediction(){
	return this->state_predicted_.block(0,0,2,1);
}

