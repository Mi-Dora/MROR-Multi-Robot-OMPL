/*********************************************************************
* Software License Agreement (BSD License)
*
*  Copyright (c) 2010, Rice University
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of the Rice University nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*********************************************************************/

/* Author: Caleb Voss */

#include <ompl/base/ScopedState.h>
#include <ompl/base/spaces/AtlasStateSpace.h>
#include <ompl/geometric/PathGeometric.h>
#include <ompl/geometric/planners/est/EST.h>
#include <ompl/geometric/planners/prm/PRM.h>
#include <ompl/geometric/planners/rrt/RRT.h>
#include <ompl/geometric/planners/rrt/RRTConnect.h>
#include <ompl/geometric/planners/rrt/RRTstar.h>

/** Simple manifold example: the unit sphere. */
Eigen::VectorXd Fsphere (const Eigen::VectorXd &x)
{
    Eigen::VectorXd f(1);
    f[0] = x.norm() - 1;
    return f;
}

/** Jacobian of Fsphere(x). */
Eigen::MatrixXd Jsphere (const Eigen::VectorXd &x)
{
    Eigen::MatrixXd j = x.transpose();
    j /= x.norm();
    return j;
}

/** More complicated manifold example: Consider three points in 3D space: p1, p2, and p3. Put p1 exactly
 * 3 units above p2, and have p3 orbit p1 at a distance of 2 in a plane perpendicular to p1. That's 9
 * dimensions, with 5 constraints, to create a 4D manifold. */
Eigen::VectorXd Fcomplicated (const Eigen::VectorXd &x)
{
    Eigen::VectorXd f(5);
    Eigen::VectorXd p1, p2, p3;
    
    // Separate out the three points
    p1 = x.segment(0, 3);
    p2 = x.segment(3, 3);
    p3 = x.segment(6, 3);
    
    f[0] = p1[0] - p2[0];           // p1, p2 have same x coordinate
    f[1] = p1[1] - p2[1];           // p1, p2 have same y coordinate
    f[2] = p1[2] - p2[2] - 3;       // p1 is 3 units above p2
    f[3] = (p1 - p3).norm() - 2;    // p3 is 2 units away from p1
    f[4] = (p3 - p1).dot(p1);       // p3 lies in the plane perpendicular to p1
    return f;
}

/** Jacobian of Fcomplicated(x). */
Eigen::MatrixXd Jcomplicated (const Eigen::VectorXd &x)
{
    Eigen::VectorXd e1, e2, p;
    e1 = x.segment(0, 3);
    e2 = x.segment(3, 3);
    p = x.segment(6, 3);
    
    Eigen::MatrixXd j = Eigen::MatrixXd::Zero(5, 9);
    j(0,0) = 1; j(0,3) = -1;
    j(1,1) = 1; j(1,4) = -1;
    j(2,2) = 1; j(2,5) = -1;
    j.row(3).head(3) = (e1 - p).transpose()/(e1 - p).norm(); j.row(3).tail(3) = -j.row(3).head(3);
    j.row(4).head(3) = (p - 2*e1).transpose(); j.row(4).tail(3) = e1.transpose();
    return j;
}

/** Every state has a 1% chance to be invalid. On very rare occasions, the start or goal is declared
 * invalid and the planning fails. */
bool isValid (const ompl::base::State *)
{
    return ((double) std::rand())/RAND_MAX < 0.99;
}

/** Print the state and its chart ID. */
void printState (const ompl::base::AtlasStateSpace::StateType *state)
{
    std::cout << "[" << state->toVector().transpose() << "]  " << state->getChart().getID() << "\n";
}

int main (int, char *[])
{
    // Atlas initialization
    const unsigned int dim = 3; /* 9; */
    ompl::base::AtlasStateSpacePtr atlas(new ompl::base::AtlasStateSpace(dim, Fsphere, Jsphere)); /* Fcomplicated, Jcomplicated); */
    ompl::base::StateSpacePtr space(boost::dynamic_pointer_cast<ompl::base::StateSpace>(atlas));
    
    // Seed some intial charts around the start and goal points
    Eigen::VectorXd x(dim); x << 0, 0, 1;
    Eigen::VectorXd y(dim); y << 0, 1, 0;
    /*
    Eigen::VectorXd x(dim); x << 0, 0, 3, 0, 0, 0, 2, 0, 3;
    Eigen::VectorXd y(dim); y << -4, -4, 0, -4, -4, -3, -4, -4, 2;
    */
    const ompl::base::AtlasChart &startChart = atlas->newChart(x);
    const ompl::base::AtlasChart &goalChart = atlas->newChart(y);
    ompl::base::ScopedState<> start(space), goal(space);
    start->as<ompl::base::AtlasStateSpace::StateType>()->setRealState(x, startChart);
    goal->as<ompl::base::AtlasStateSpace::StateType>()->setRealState(y, goalChart);
    
    // More setup for the space and problem definition
    ompl::base::SpaceInformationPtr si(new ompl::base::SpaceInformation(space));
    atlas->setSpaceInformation(si);
    si->setStateValidityChecker(&isValid);
    ompl::base::RealVectorBounds bounds(dim);
    bounds.setLow(-10);
    bounds.setHigh(10);
    atlas->setBounds(bounds);
    ompl::base::ProblemDefinitionPtr pdef(new ompl::base::ProblemDefinition(si));
    pdef->setStartAndGoalStates(start, goal);
    si->setup();
    
    // Choose the planner. Try others, like RRT, RRTstar, EST, PRM, ...
    ompl::base::PlannerPtr planner(new ompl::geometric::RRTConnect(si));
    planner->setProblemDefinition(pdef);
    planner->setup();
    
    // Plan for at most 60 seconds
    std::clock_t tstart = std::clock();
    ompl::base::PlannerStatus stat;
    if ((stat = planner->solve(60)) == ompl::base::PlannerStatus::EXACT_SOLUTION)
    {
        double time = ((double)(std::clock()-tstart))/CLOCKS_PER_SEC;
        std::cout << "Solution found!\n";
        
        // Extract the solution path by re-interpolating between the saved states
        const std::vector<ompl::base::State *> &waypoints =
            boost::dynamic_pointer_cast<ompl::geometric::PathGeometric>(pdef->getSolutionPath())->getStates();
        double length = 0;
        for (std::size_t i = 0; i < waypoints.size()-1; i++)
        {
            // Denote that we are switching to the next saved state
            std::cout << "-----\n";
            ompl::base::AtlasStateSpace::StateType *from, *to;
            from = waypoints[i]->as<ompl::base::AtlasStateSpace::StateType>();
            to = waypoints[i+1]->as<ompl::base::AtlasStateSpace::StateType>();
            
            // Traverse the manifold
            std::vector<ompl::base::AtlasStateSpace::StateType *> stateList;
            atlas->followManifold(from, to, true, &stateList);
            if (!atlas->equalStates(stateList.front(), stateList.back()))
            {
                // Print the intermediate states
                for (std::size_t i = 1; i < stateList.size(); i++)
                {
                    printState(stateList[i]);
                    length += atlas->distance(stateList[i-1], stateList[i]);
                }
            }
            
            // Delete the intermediate states
            for (std::size_t i = 0; i < stateList.size(); i++)
                atlas->freeState(stateList[i]);
        }
        std::cout << "-----\n";
        std::cout << "Length: " << length << "\n";
        std::cout << "Took " << time << " seconds.\n";
    }
    else if (stat == ompl::base::PlannerStatus::APPROXIMATE_SOLUTION)
        std::cout << "Not enough time!\n";
    else
        std::cout << "No solution found.\n";
    
    std::cout << "Atlas created " << atlas->getChartCount() << " charts.\n";
    
    return 0;
}
