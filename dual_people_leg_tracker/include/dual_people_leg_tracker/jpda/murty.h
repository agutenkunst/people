#ifndef PEOPLE_DUAL_PEOPLE_LEG_TRACKER_INCLUDE_DUAL_PEOPLE_LEG_TRACKER_JPDA_MURTY_H_
#define PEOPLE_DUAL_PEOPLE_LEG_TRACKER_INCLUDE_DUAL_PEOPLE_LEG_TRACKER_JPDA_MURTY_H_

// System includes
#include <cstdio>
#include <vector>

// Eigen includes
#include <Eigen/Dense>

// Hungarian Solver
#include <libs/libhungarian/hungarian.h>
#include <libs/dlib/optimization/max_cost_assignment.h>

//#define PSEUDO_INF
#define INF 100000


// Murty Algorithm, basic header only implementation
void color_print_solution(Eigen::Matrix<int, -1, -1> costMatrix, Eigen::Matrix<int, -1, -1> solution) {
  int i,j;
  fprintf(stderr , "\n");
  for(i=0; i<costMatrix.rows(); i++) {
    fprintf(stderr, " [");
    for(j=0; j<costMatrix.cols(); j++) {
      if(solution(i,j) == 1)
        fprintf(stderr,"\033[1m\033[33m");
      fprintf(stderr, "%5d ",costMatrix(i,j));
      fprintf(stderr,"\033[0m");

    }
    fprintf(stderr, "]\n");
  }
}

int** eigenmatrix_to_cmatrix(Eigen::Matrix<int, -1, -1> m, int rows, int cols) {
  int i,j;
  int** r;
  r = (int**)calloc(rows,sizeof(int*));
  for(i=0;i<rows;i++)
  {
    r[i] = (int*)calloc(cols,sizeof(int));
    for(j=0;j<cols;j++)
      r[i][j] = m(i,j);
  }
  return r;
}

Eigen::Matrix<int, -1, -1> cmatrix_to_eigenmatrix(int** C, int rows, int cols) {

  Eigen::Matrix<int, -1, -1> result;
  result = Eigen::Matrix<int, -1, -1>::Zero(rows,cols);

  int i,j;
  for(i=0; i<rows; i++) {
    for(j=0; j<cols; j++) {
      result(i,j) = C[i][j];
    }
  }

  return result;
}


enum STATE {PARTITIONED, UNPARTITIONED};

class Solution {
  public:
    Eigen::Matrix<int, -1, -1> assignmentMatrix;
    int cost_total; // Cost after solving
};

class SolutionProblemPair {
  public:
    STATE state;
    Eigen::Matrix<int, -1, -1> problem;
    Solution solution;
};

Solution solvehungarian(Eigen::Matrix<int, -1, -1> problem){

    Solution solution;

  // Turn eigenmatrix into a c matrix
    int** m = eigenmatrix_to_cmatrix(problem,problem.rows(),problem.cols());


    unsigned int nRows = problem.rows();
    unsigned int nCols = problem.cols();

    // Solve Hungarian
    hungarian_problem_t p;

    /* initialize the gungarian_problem using the cost matrix*/
    int matrix_size = hungarian_init(&p, m , nRows, nCols, HUNGARIAN_MODE_MINIMIZE_COST) ;

    /* solve the assignement problem */
    hungarian_solve(&p);

    solution.cost_total = p.cost_total;
    solution.assignmentMatrix = cmatrix_to_eigenmatrix(p.assignment,nRows,nCols);

    /* free used memory */
    hungarian_free(&p);
    free(m);

    return solution;
}

/*
Solution solvehungarianDlib(Eigen::Matrix<int, -1, -1> problem){

    Solution solution;



    // This prints optimal assignments:  [2, 0, 1] which indicates that we should assign
    // the person from the first row of the cost matrix to job 2, the middle row person to
    // job 0, and the bottom row person to job 1.
    for (unsigned int i = 0; i < assignment.size(); i++)
        cout << assignment[i] << std::endl;

    // This prints optimal cost:  16.0
    // which is correct since our optimal assignment is 6+5+5.
    cout << "optimal cost: " << assignment_cost(cost, assignment) << endl;


  // Turn eigenmatrix into a c matrix
    int** m = eigenmatrix_to_cmatrix(problem,problem.rows(),problem.cols());



    // To find out the best assignment of people to jobs we just need to call this function.
    std::vector<long> assignment = dlib::max_cost_assignment(m);

    unsigned int nRows = problem.rows();
    unsigned int nCols = problem.cols();

    // Solve Hungarian
    hungarian_problem_t p;

     initialize the gungarian_problem using the cost matrix
    int matrix_size = hungarian_init(&p, m , nRows, nCols, HUNGARIAN_MODE_MINIMIZE_COST) ;

     solve the assignement problem
    hungarian_solve(&p);

    solution.cost_total = p.cost_total;
    solution.assignmentMatrix = cmatrix_to_eigenmatrix(p.assignment,nRows,nCols);

     free used memory
    hungarian_free(&p);
    free(m);

    return solution;
}
*/

bool sortPairs (SolutionProblemPair i, SolutionProblemPair j) {
  // PARTITIONED ALWAYS BEFORE UNPARTITIONED
  if(i.state == PARTITIONED && j.state == UNPARTITIONED)
    return true;

  // Second criteria are the costs
  return (i.solution.cost_total < j.solution.cost_total);
}



//// Printer the Answer List
void printAnswerList(std::vector<SolutionProblemPair> AnswerList){

  std::cout << "AnswerList is now:" << std::endl;
  for(std::vector<SolutionProblemPair>::iterator sppIt = AnswerList.begin();
      sppIt != AnswerList.end();
      sppIt++){
    if(sppIt->state == PARTITIONED)
      std::cout << "  PARTITIONED ";
    else
      std::cout << "UNPARTITIONED ";

    std::cout << "Costs: " << sppIt->solution.cost_total << std::endl;
  }
}

std::vector<Solution> murty(Eigen::Matrix<int, -1, -1> costMat, int nBest){

  // Initialize needed lists
  std::vector<SolutionProblemPair> AnswerList;

  // Initialize the first solutions
  SolutionProblemPair firstPair;
  firstPair.state = UNPARTITIONED;
  firstPair.problem = costMat;
  firstPair.solution = solvehungarian(costMat);

  AnswerList.push_back(firstPair);

  // Pointer to the UnpartitionedPair with the lowest costs
  SolutionProblemPair* lowestCostPair;


  bool containesUnpartitioned = false;

  // The main loop
  do{

    // Intermediate list
    std::vector<SolutionProblemPair> NewList;

    // Find unpartitioned pair with the lowest costs
    for(std::vector<SolutionProblemPair>::iterator sppIt = AnswerList.begin(); sppIt != AnswerList.end(); sppIt++){

      // We can use the first unpartioned pair because the list is sorted
      if(sppIt->state == UNPARTITIONED){
        lowestCostPair = &(*sppIt);
        break;
      }
    }

    // Iterate each assignment inside the assigment matrix
    for(int i=0; i<lowestCostPair->problem.rows(); i++)
      for(int j=0; j<lowestCostPair->problem.cols(); j++)
      {

        // Check the assignment matrix
        if(lowestCostPair->solution.assignmentMatrix(i,j) == 1){

          Eigen::Matrix<int, -1, -1> problem_copy = lowestCostPair->problem;
          problem_copy(i,j) = INF;

          // Solve the modified problem
          Solution solution;
          solution = solvehungarian(problem_copy);

          if(solution.cost_total < INF &&

            (AnswerList.size() < nBest || AnswerList.back().solution.cost_total >= solution.cost_total)){
            SolutionProblemPair solutionProblemPair;
            solutionProblemPair.state = UNPARTITIONED;
            solutionProblemPair.solution = solution;
            solutionProblemPair.problem = problem_copy;

            // Add to new list
            NewList.push_back(solutionProblemPair);
          }

          // Remove the column
          for(int m=0; m<costMat.rows(); m++) {
            if(m != i)
              lowestCostPair->problem(m,j) = INF; // Column must be fixed
          }

          // Remove the row
          for(int n=0; n<costMat.cols(); n++) {
            if(n != j)
              lowestCostPair->problem(i,n) = INF; // Row must be fixed
          }
        }

      }

      // Mark Pair as PARTITIONED
      lowestCostPair->state = PARTITIONED;

      // Append NewList to AnswerList
      AnswerList.reserve(AnswerList.size()+NewList.size()); // preallocation
      AnswerList.insert(AnswerList.end(),NewList.begin(), NewList.end());

      // Sort the AnswerList
      std::sort(AnswerList.begin(),AnswerList.end(), sortPairs);

      // Truncate
      if(AnswerList.size() > nBest){
        AnswerList.resize(nBest);
      }

    containesUnpartitioned = false;
    // Check if the list contains unpartitioned
    for(std::vector<SolutionProblemPair>::iterator sppIt = AnswerList.begin();
        sppIt != AnswerList.end();
        sppIt++){

      if(sppIt->state == UNPARTITIONED){
        containesUnpartitioned = true;
        break;
      }
    }

  }
  while(containesUnpartitioned);

  std::vector<Solution> solutions;

  for(std::vector<SolutionProblemPair>::iterator sppIt = AnswerList.begin();
      sppIt != AnswerList.end();
      sppIt++){

    solutions.push_back(sppIt->solution);
  }

  return solutions;

}

#endif /* PEOPLE_DUAL_PEOPLE_LEG_TRACKER_INCLUDE_DUAL_PEOPLE_LEG_TRACKER_JPDA_MURTY_H_ */
