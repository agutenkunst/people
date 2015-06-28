#ifndef PEOPLE_DUAL_PEOPLE_LEG_TRACKER_SRC_MHT_HYPOTHESIS_H_
#define PEOPLE_DUAL_PEOPLE_LEG_TRACKER_SRC_MHT_HYPOTHESIS_H_

#undef NDEBUG

// ROS includes
#include <ros/ros.h>

// Own includes
#include <dual_people_leg_tracker/detection/detection.h>
#include <dual_people_leg_tracker/jpda/murty.h>
#include <dual_people_leg_tracker/visualization/color_definitions.h>
#include <dual_people_leg_tracker/mht/Track.h>
#include <dual_people_leg_tracker/mht/NewTrackAssignment.h>
#include <dual_people_leg_tracker/mht/TrackAssignment.h>
//#include <dual_people_leg_tracker/mht/HypothesisTree.h>

// Boost includes
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

// Eigen includes
#include <eigen3/Eigen/Dense>

// System includes
#include <vector>

class Hypothesis; // Forward declaration
typedef boost::shared_ptr<Hypothesis> HypothesisPtr;

// Id counter
static int hypothesisIdCounter = 0;

class Hypothesis :  public boost::enable_shared_from_this<Hypothesis>{
private:

	// The id
	int id_;

	// CostValues
	int newTrackCostValue_;
	int falseAlarmCostValue_;
	int deletionCostValue_;
	int occlusionCostValue_;

	int invalidValue_;

	int cycle_; // The cycle to which this belongs
	int root_cycle_; // The current root cycle

    std::vector<Solution> solutions;

    std::vector<HypothesisPtr> children_;

    double probability_; // Probability of this hypothesis
    double parent_probability_; // Probability of the parent

    Eigen::Matrix<double,2,-1> detections_;

    // Time values
    double dt_;				// [s] Time difference to parent, needed for prediction
    ros::Time parent_time_;		// Time of the parent



    std::vector<TrackPtr> tracks_;

    ros::Time time_; // The time of this hypothesis

//	extern static enum LABEL{
//	    OCCLUDED,
//		DELETED,
//		DETECTED,
//		NEW_TRACK,
//		FALSE_ALARM
//	};

	// The current cost matrix
	Eigen::Matrix< int, Eigen::Dynamic, Eigen::Dynamic> costMatrix;

	// Numbers
	unsigned int numberOfMeasurements_;

public:
	Hypothesis(int cycle);
	virtual ~Hypothesis();

	/***
	 * Get the number of current Tracks hold by this hypothesis
	 */
	unsigned int getNumberOfTracks();

	long double getProbability(){
		return this->getProbability();
	}

	long double getParentProbabilility(){
		return this->parent_probability_;
	}

	// Assign the measurements (this is done in every cycle)
	bool assignMeasurements(int cycle, Eigen::Matrix<double,2,-1> detections, ros::Time time);

	// Create the cost matrix
	bool createCostMatrix();

	// Solve the cost matrix using murty
	bool solveCostMatrix();

	// Std Cout the Solutions from hypothesis of a given cycle
	bool stdCoutSolutions();

	// Create Children
	bool createChildren();

	// Set the parent
	void setParentProbability(double probability);

	// Add a track to this hypothesis
	void addTrack(TrackPtr track);

	// Get the cycle
	const int getCycle(){
		return this->cycle_;
	}

	// Set the time
	void setTime(ros::Time time){
		this->time_ = time;
	}

	// Set the Root Cycle
	void setRootCycle(int rootCycle);

	// Get the Root Cycle
	int getRootCycle() const{
		return this->root_cycle_;
	}

	// Print yourself
	void print();

	// Std Cout current solutions
	void coutCurrentSolutions(int cycle);

	// Time the parent received its data
	void setParentTime(ros::Time time);

	// Get the id
	int getId(){ return this->id_; };
};

#endif /* PEOPLE_DUAL_PEOPLE_LEG_TRACKER_SRC_MHT_HYPOTHESIS_H_ */
