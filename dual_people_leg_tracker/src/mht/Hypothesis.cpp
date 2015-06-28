/*
 * Hypothesis.cpp
 *
 *  Created on: Jun 27, 2015
 *      Author: alex
 */

#include <dual_people_leg_tracker/mht/Hypothesis.h>
#include <dual_people_leg_tracker/mht/HypothesisTree.h>

// Parameters (from Paper)
const long double prob_detected_free = 0.3;
const long double prob_occluded_free = 0.63;
const long double prob_deleted_free = 0.07;
const long double prob_detected_approved = 0.2;
const long double prob_occluded_approved = 0.79;
const long double prob_deleted_approved = 0.01;

const long double prob_new = 0.001;
const long double prob_fal = 0.003;

Hypothesis::Hypothesis(int cycle):
	newTrackCostValue_(20),
	falseAlarmCostValue_(10),
	deletionCostValue_(50),
	occlusionCostValue_(120),
	invalidValue_(-1000),
	probability_(1.0),
	cycle_(cycle),
	root_cycle_(0),
	dt_(0.0),
	id_(hypothesisIdCounter++)
{
	std::cout << "New Hypothesis create for cycle " << cycle << std::endl;
}

Hypothesis::~Hypothesis() {
	// TODO Auto-generated destructor stub

}

unsigned int Hypothesis::getNumberOfTracks(){
	return this->tracks_.size();
}

bool Hypothesis::assignMeasurements(int cycle, Eigen::Matrix<double,2,-1> detectionsMat, ros::Time time){

	// Recursive call if this is not the right generation
	if(cycle > this->cycle_){
		//std::cout << "Measurements are passed to the " << this->children_.size() << " children" << std::endl;
		for(std::vector<HypothesisPtr>::iterator hypoIt = this->children_.begin(); hypoIt != this->children_.end(); hypoIt++){
			//std::cout << "Pass" << std::endl;
			(*hypoIt)->assignMeasurements(cycle, detectionsMat, time);
		}

		// Done
		return true;
	}
	std::cout << "To a hypothesis in cycle " << BOLDRED << cycle_ << RESET <<" new measurements are assigned" << std::endl;

	// Set the time
	this->time_ = time;

	// Calculate the time difference to the step, needed for prediction
	this->dt_ = (this->time_ - this->parent_time_).toSec();

	// Predict the tracks
	for(std::vector<TrackPtr>::iterator trackIt = this->tracks_.begin(); trackIt != this->tracks_.end(); trackIt++){
		std::cout << "Doing prediction of track " << (*trackIt)->getId() << std::endl;
		(*trackIt)->predict(this->time_);
	}

	// Assign the detections
	this->detections_ = detectionsMat;

	// Calculate most important values
	numberOfMeasurements_ = detectionsMat.cols();

	// Create the cost matrix
	this->createCostMatrix();

	// Solve the cost matrix
	this->solveCostMatrix();

	// Print the solutions
	// this->stdCoutSolutions();
	std::cout << "Starting to create children" << std::endl;
	this->createChildren();


	//rootHypothesis->createChildren();

	return true;
}

bool Hypothesis::createCostMatrix(){

    // Generate Matrix
    //std::cout << "There are currently " << nObjects << " objects and " << nMeasurements << " measurements(occlusion included)" << std::endl;

	unsigned int numberOfCols = getNumberOfTracks() 	  + 2 * numberOfMeasurements_;
	unsigned int numberOfRows = numberOfMeasurements_ + 2 * getNumberOfTracks();

	unsigned int numberOfTracks = getNumberOfTracks();

	if(getNumberOfTracks() > numberOfMeasurements_){
		// Initialize
		unsigned int size = numberOfMeasurements_ + 2 * numberOfTracks;
	    costMatrix = Eigen::Matrix< int, Eigen::Dynamic, Eigen::Dynamic>::Constant(size,size,this->invalidValue_);
	}else{
		// Initialize
		unsigned int size = numberOfTracks 	  + 2 * numberOfMeasurements_;
	    costMatrix = Eigen::Matrix< int, Eigen::Dynamic, Eigen::Dynamic>::Constant(size,size,this->invalidValue_);
	}

    // Print the cost matrix
    //std::cout << "The cost matrix is now " << std::endl << costMatrix << std::endl;




    // Fill the new tracks
    costMatrix.block(getNumberOfTracks(),0,numberOfMeasurements_,numberOfMeasurements_).diagonal() = Eigen::Matrix< int, -1, 1>::Constant(numberOfMeasurements_,1,this->newTrackCostValue_);

    //std::cout << "The cost matrix is now " << std::endl << costMatrix << std::endl;

    // Fill the false alarms
    costMatrix.block(getNumberOfTracks()+numberOfMeasurements_,0,numberOfMeasurements_,numberOfMeasurements_).diagonal() = Eigen::Matrix< int, -1, 1>::Constant(numberOfMeasurements_,1,this->falseAlarmCostValue_);

    // Fill the Deletions
    costMatrix.block(0,numberOfMeasurements_,numberOfTracks,getNumberOfTracks()).diagonal() = Eigen::Matrix< int, -1, 1>::Constant(numberOfTracks,1,this->deletionCostValue_);

    // Fill the Occlusions
    costMatrix.block(0,numberOfMeasurements_+getNumberOfTracks(),getNumberOfTracks(),getNumberOfTracks()).diagonal() = Eigen::Matrix< int, -1, 1>::Constant(getNumberOfTracks(),1,this->occlusionCostValue_);

    costMatrix.block(getNumberOfTracks(),numberOfMeasurements_,2*numberOfMeasurements_,2*getNumberOfTracks()) = Eigen::Matrix< int, -1, -1>::Constant(2*numberOfMeasurements_,2*numberOfTracks,-100);
    if(getNumberOfTracks() > numberOfMeasurements_){
    	costMatrix.block(getNumberOfTracks(),numberOfMeasurements_,2*numberOfMeasurements_,2*numberOfMeasurements_).diagonal() = Eigen::Matrix< int, -1, 1>::Constant(2*numberOfMeasurements_,1,0);
    }
    else{

    	costMatrix.block(numberOfTracks,numberOfMeasurements_,numberOfMeasurements_,numberOfMeasurements_).diagonal() = Eigen::Matrix< int, -1, 1>::Constant(numberOfMeasurements_,1,-100);
    	costMatrix.block(numberOfTracks+numberOfMeasurements_,numberOfMeasurements_,numberOfMeasurements_,numberOfMeasurements_).diagonal() = Eigen::Matrix< int, -1, 1>::Constant(numberOfMeasurements_,1,-100);

    	costMatrix.block(numberOfTracks,numberOfMeasurements_,2*numberOfTracks,2*numberOfTracks).diagonal() = Eigen::Matrix< int, -1, 1>::Constant(2*numberOfTracks,1,0);
    }

    // Fill the validated measurements (using mahalanobis distance)
    for(unsigned int t=0; t < numberOfTracks; t++){
        for(unsigned int m=0; m < numberOfMeasurements_; m++){

        	Eigen::Vector2d measPrediction;
        	measPrediction = this->tracks_[t]->getMeasurementPrediction();

        	// Use the distance between measurement and measurement prediction
        	double dist = (measPrediction - this->detections_.col(m)).norm();

        	costMatrix(t, m) = ((int) (50 / dist) * 100) / 100;
        }
    }

    // Print the cost matrix
    if(numberOfTracks > 0)
    std::cout << "The cost matrix is now " << std::endl << costMatrix << std::endl;

    // StdCout the implications of this





	return true;
}

bool Hypothesis::solveCostMatrix(){
    //std::cout << std::endl << "Cost Matrix:" << std::endl  << costMatrix << std::endl;

    // TODO depend this on the number of measurements
    int k = 1;
    solutions = murty(-costMatrix,k);

    // TODO Filter the solution regarding several parameters using the leg tracker information
    // TODO Calculate the crossing value of the solutions in order to reject obvious unrealistic solutions

    //std::cout << "Solutions are:" << std::endl;
    //for(std::vector<Solution>::iterator solIt = solutions.begin(); solIt != solutions.end(); solIt++){
      //color_print_solution(costMatrix,solIt->assignmentMatrix);
      //std::cout << "Costs "<< "\033[1m\033[31m" << solIt->cost_total << "\033[0m" << std::endl;
    //}



    return true;
}

bool Hypothesis::stdCoutSolutions(){

	unsigned int numberOfTracks = getNumberOfTracks();

	Eigen::Matrix<double, -1, 1> childProbs = Eigen::Matrix<double, -1, 1>::Zero(solutions.size(),1);
	//double childProbs[solutions.size];

	size_t solCounter = 0;
    for(std::vector<Solution>::iterator solIt = solutions.begin(); solIt != solutions.end(); solIt++){
      std::cout << "Starting printing the solutions for HYP[" << getId() << "]" << " cycle " << this->cycle_ << std::endl;
      color_print_solution(costMatrix,solIt->assignmentMatrix);
      std::cout << "Costs "<< "\033[1m\033[31m" << solIt->cost_total << "\033[0m" << std::endl;

      // Get the assignment matrix
      Eigen::Matrix<int, -1, -1> assignmentMatrix = solIt->assignmentMatrix;

      // Iterate the matrix
      size_t nCols = assignmentMatrix.cols();
      size_t nRows = assignmentMatrix.rows();

      //Parameters
      size_t N_det_F = 0;	// Number of detected free tracks
      size_t N_occ_F = 0;	// Number of occluded free tracks
      size_t N_del_F = 0;	// Number of deleted  free tracks

      size_t N_det_A = 0;	// Number of detected approved tracks
      size_t N_occ_A = 0;	// Number of occluded approved tracks
      size_t N_del_A = 0;	// Number of deleted  approved tracks

      size_t N_new = 0;		// New tracks
      size_t N_false = 0;   // Deleted tracks

      // Iterate through the top left rectangle
      for(size_t r=0; r < numberOfTracks; r++){
          for(size_t c=0; c < numberOfMeasurements_; c++){

        	  // Skip if there is no assignment
        	  if(assignmentMatrix(r,c) == 0) continue;

        	  // Check if this value is on one of the four diagonals
        	  std::cout << "TRACK " << r << " --> DETECTED (MEAS " << c  << ")" << std::endl;
        	  N_det_F++;
          }
      }

      // Iterate the new Tracks
      for(size_t i=0; i < numberOfMeasurements_; i++){

    	  // Skip if there is no assignment
    	  if(assignmentMatrix(i+numberOfTracks,i) == 0) continue;

    	  // Check if this value is on one of the four diagonals
    	  std::cout << "MEAS  " << i << " --> NEW_TRACK" << std::endl;
    	  N_new++;
      }

      // Iterate false alarms
      for(size_t i=0; i < numberOfMeasurements_; i++){

    	  // Skip if there is no assignment
    	  if(assignmentMatrix(i+numberOfMeasurements_+numberOfTracks,i) == 0) continue;

    	  // Check if this value is on one of the four diagonals
    	  std::cout << "MEAS  " << i << " --> FALSE_ALARM" << std::endl;
    	  N_false++;
      }

      // Iterate occlusion
      for(size_t i=0; i < numberOfTracks; i++){

    	  // Skip if there is no assignment
    	  if(assignmentMatrix(i,i+numberOfMeasurements_) == 0) continue;

    	  // Check if this value is on one of the four diagonals
    	  std::cout << "TRACK " << i << " --> OCCLUDED" << std::endl;
    	  N_occ_F++;
      }

      // Iterate deletion
      for(size_t i=0; i < numberOfTracks; i++){

    	  // Skip if there is no assignment
    	  if(assignmentMatrix(i,i+numberOfMeasurements_+numberOfTracks) == 0) continue;
    	  N_del_F++;

    	  // Check if this value is on one of the four diagonals
    	  std::cout << "TRACK " << i << " --> DELETED" << std::endl;
      }

      double childProb = pow(prob_detected_free,    N_det_F) *
    		  	  	  	 pow(prob_occluded_free,    N_occ_F) *
						 pow(prob_deleted_free,     N_del_F) *
						 pow(prob_detected_approved,N_det_A) *
						 pow(prob_occluded_approved,N_occ_A) *
						 pow(prob_deleted_approved, N_del_A) *
						 pow(prob_new, N_new) *
						 pow(prob_new, N_false);

      std::cout << RED << childProb << RESET << std::endl;
      childProbs[solCounter] = childProb;

      solCounter++;

    }

    childProbs.normalize();

    std::cout << "childProbs" << std::endl << childProbs << std::endl;

    return true;
}

bool Hypothesis::createChildren(){

	ROS_ASSERT(solutions.size() > 0);

	Eigen::Matrix<double, -1, 1> childProbs = Eigen::Matrix<double, -1, 1>::Zero(solutions.size(),1);
		//double childProbs[solutions.size];

	// Holds the assignments
    std::vector<std::vector<TrackAssignment> > all_assignments;


	// Iterate the solutions
	size_t solCounter = 0;
	for(std::vector<Solution>::iterator solIt = solutions.begin(); solIt != solutions.end(); solIt++){

	  // Get the assignment matrix
	  Eigen::Matrix<int, -1, -1> assignmentMatrix = solIt->assignmentMatrix;

	  // Storage for track Assignments of this solution
	  std::vector<TrackAssignment> assignments;

	  // Iterate the matrix
	  size_t nCols = assignmentMatrix.cols();
	  size_t nRows = assignmentMatrix.rows();

	  //Parameters
	  size_t N_det_F = 0;	// Number of detected free tracks
	  size_t N_occ_F = 0;	// Number of occluded free tracks
	  size_t N_del_F = 0;	// Number of deleted  free tracks

	  size_t N_det_A = 0;	// Number of detected approved tracks
	  size_t N_occ_A = 0;	// Number of occluded approved tracks
	  size_t N_del_A = 0;	// Number of deleted  approved tracks

	  size_t N_new = 0;		// New tracks
	  size_t N_false = 0;   // Deleted tracks

	  // Detected tracks
	  for(size_t r=0; r < getNumberOfTracks(); r++){
		  for(size_t c=0; c < numberOfMeasurements_; c++){

			  // Skip if there is no assignment
			  if(assignmentMatrix(r,c) == 0) continue;

			  TrackAssignment assignment(this->tracks_[r], c, TrackAssignment::DETECTED);
			  assignments.push_back(assignment);
			  N_det_F++;
		  }
	  }

	  // Iterate the new Tracks
	  for(size_t i=0; i < numberOfMeasurements_; i++){

		  // Skip if there is no assignment
		  if(assignmentMatrix(i+getNumberOfTracks(),i) == 0) continue;

		  N_new++;
		  TrackAssignment assignment(i, TrackAssignment::NEW);
		  assignments.push_back(assignment);
		  std::cout << "New Track!!!" << std::endl;
	  }

	  // Iterate false alarms
	  for(size_t i=0; i < numberOfMeasurements_; i++){

		  // Skip if there is no assignment
		  if(assignmentMatrix(i+numberOfMeasurements_+getNumberOfTracks(),i) == 0) continue;

		  N_false++;

		  TrackAssignment assignment(i, TrackAssignment::FALSEALARM);
		  assignments.push_back(assignment);
	  }

	  // Iterate occlusion
	  for(size_t i=0; i < getNumberOfTracks(); i++){

		  // Skip if there is no assignment
		  if(assignmentMatrix(i,i+numberOfMeasurements_) == 0) continue;

		  N_occ_F++;
		  TrackAssignment assignment(this->tracks_[i], TrackAssignment::OCCLUDED);
		  assignments.push_back(assignment);
	  }

	  // Iterate deletion
	  for(size_t i=0; i < getNumberOfTracks(); i++){

		  // Skip if there is no assignment
		  if(assignmentMatrix(i,i+numberOfMeasurements_+getNumberOfTracks()) == 0) continue;
		  N_del_F++;
		  TrackAssignment assignment(this->tracks_[i], TrackAssignment::DELETED);
		  assignments.push_back(assignment);
	  }



	  double childProb = pow(prob_detected_free,    N_det_F) *
						 pow(prob_occluded_free,    N_occ_F) *
						 pow(prob_deleted_free,     N_del_F) *
						 pow(prob_detected_approved,N_det_A) *
						 pow(prob_occluded_approved,N_occ_A) *
						 pow(prob_deleted_approved, N_del_A) *
						 pow(prob_new, N_new) *
						 pow(prob_new, N_false) *
						 this->getParentProbabilility();

	  std::cout << RED << childProb << RESET << std::endl;
	  childProbs[solCounter] = childProb;

	  all_assignments.push_back(assignments);

	  solCounter++;

	}

	double probSum = childProbs.sum();
	// Normalize the children probabilities
	for(size_t i = 0; i < childProbs.rows(); i++)
		childProbs[i] = childProbs[i]/probSum;

	std::cout << childProbs << std::endl;

	double maxProb = childProbs.maxCoeff();

	// Create the children, one for each solution
	for(size_t i = 0; i < solutions.size(); i++){

		// Ratio pruning
		if(childProbs[i]/maxProb > 0.1){

			// Print the assignments
			std::cout << std::endl << "Assignments:" << std::endl;
			for(std::vector<TrackAssignment>::iterator assIt = all_assignments[i].begin(); assIt != all_assignments[i].end(); assIt++){
				std::cout << (*assIt) << std::endl;
			}
			std::cout << std::endl;

			// Create child
			HypothesisPtr childHypothesis(new Hypothesis(cycle_+1));
			childHypothesis->setParentProbability(this->probability_);
			childHypothesis->setRootCycle(this->getRootCycle());
			childHypothesis->setParentTime(this->time_);

			// Iterate the assignments
			std::vector<TrackAssignment> assignments = all_assignments[i];
			for(std::vector<TrackAssignment>::iterator assIt = assignments.begin(); assIt != assignments.end(); assIt++){
				switch(assIt->type_){

					// New track
					case TrackAssignment::NEW:
					{
						TrackPtr newTrack(new Track(this->detections_.col(assIt->meas_), this->time_));
						childHypothesis->addTrack(newTrack);
						std::cout << "    A New track for child " << i << " at " << std::endl << this->detections_.col(assIt->meas_) << std::endl;
					}
					break;

					// Detection
					case TrackAssignment::DETECTED:
					{
						TrackPtr copyTrack(new Track( *(assIt->track_) ));
						childHypothesis->addTrack(copyTrack);
						std::cout << assIt->track_->getId() << " =>copy  " << copyTrack->getId() << std::endl;

						std::cout << "Update of track " << copyTrack->getId() << std::endl;
						std::cout << assIt->track_->getMeasurementPrediction() << std::endl;
						std::cout << copyTrack->getMeasurementPrediction() << std::endl;
						copyTrack->update(this->detections_.col(assIt->meas_));
						// TODO, apply measurement !!! After!! making the copy!
					}
					break;

					// OCCLUDED
					case TrackAssignment::OCCLUDED:
					{
						// TODO
					}
					break;

					// FALSEALARM
					case TrackAssignment::FALSEALARM:
					{
						// TODO
					}
					break;

					// DELETED
					case TrackAssignment::DELETED:
					{
						// TODO
					}
					break;

				}

			}

			this->children_.push_back(childHypothesis);
			//this->globalHypothesisTree_->addHypothesis(childHypothesis);

		}

	}

	//std::cout << this->children_.size() << " children were created" << std::endl;


	//std::cout << "childProbs" << std::endl << childProbs << std::endl;



	return true;
}

void Hypothesis::setParentProbability(double probability){

	this->parent_probability_ = probability;

	//std::cout << "I am Hypothesis for cycle " << this->cycle_ << " my parent was in cycle " << this->parent_->cycle_ << std::endl;
}

void Hypothesis::addTrack(TrackPtr track){
	this->tracks_.push_back(track);
	std::cout << "HPY[" << getId() << "] adding track" << track->getId() << "now has " << getNumberOfTracks() << " tracks" << std::endl;
}

void Hypothesis::setRootCycle(int rootCycle){
	for(std::vector<HypothesisPtr>::iterator hypoIt = this->children_.begin(); hypoIt != this->children_.end(); hypoIt++){
		(*hypoIt)->setRootCycle(rootCycle);
	}
}

// Print yourself
void Hypothesis::print(){
	int nTabs = this->cycle_ - getRootCycle();
	for(int i=0; i < nTabs; i++){
		std::cout << "  ";
	}
	std::cout << "->HYP[ID:" << getId() << " cyc: " << getCycle() << " nCh: " << this->children_.size() << " nT: " << getNumberOfTracks() << " dt:" << this->dt_ << "]" << std::endl;
	for(std::vector<HypothesisPtr>::iterator hypoIt = this->children_.begin(); hypoIt != this->children_.end(); hypoIt++){
		(*hypoIt)->print();
	}
}

void Hypothesis::coutCurrentSolutions(int cycle){
	if(this->cycle_ ==  cycle)
		this->stdCoutSolutions();
	else
		for(std::vector<HypothesisPtr>::iterator hypoIt = this->children_.begin(); hypoIt != this->children_.end(); hypoIt++){
			(*hypoIt)->coutCurrentSolutions(cycle);
		}
}

void Hypothesis::setParentTime(ros::Time time){
	this->parent_time_ = time;
}
