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

#ifndef OMPL_BASE_SPACE_ATLAS_STATE_SPACE_
#define OMPL_BASE_SPACE_ATLAS_STATE_SPACE_

#include "ompl/base/MotionValidator.h"
#include "ompl/base/StateSampler.h"
#include "ompl/base/spaces/AtlasChart.h"
#include "ompl/base/spaces/RealVectorStateSpace.h"
#include "ompl/datastructures/PDF.h"

#include <eigen3/Eigen/Core>

namespace ompl
{
    namespace base
    {
        // Forward declarations
        OMPL_CLASS_FORWARD(AtlasStateSpace);
        class AtlasChart;
        
        /** \brief StateSampler for use on an atlas. */
        class AtlasStateSampler : public StateSampler
        {
        public:
            
            /** \brief Constructor. */
            AtlasStateSampler (const AtlasStateSpace &atlas);
            
            /** \brief Sample a state uniformly from the known charted regions of the manifold. Return in \a state. */
            void sampleUniform (State *state);
            
            /** \brief Sample a state uniformly from the ball with center \a near and radius \a distance. Return in \a state. */
            void sampleUniformNear (State *state, const State *near, const double distance);
            
            /** \brief Not implemented. */
            void sampleGaussian (State *, const State *, const double);
            
        private:
            
            /** \brief Atlas on which to sample. */
            const AtlasStateSpace &atlas_;
        };
        
        /** \brief Atlas-specific implementation of checkMotion(). */
        class AtlasMotionValidator : public MotionValidator
        {
        public:
            
            /** \brief Constructor. */
            AtlasMotionValidator (SpaceInformation *si);
            
            /** \brief Constructor. */
            AtlasMotionValidator (const SpaceInformationPtr &si);
            
            /** \brief Return whether we can step from \a s1 to \a s2 without collision. */
            bool checkMotion (const State *s1, const State *s2) const;
            
            /** \brief Return whether we can step from \a s1 to \a s2 without collision.
             * If not, return the last valid state and its interpolation parameter in \a lastValid. 
             * If traversing the manifold terminates because it exits the ball of radius d(s1,s2), or
             * accumulates a distance traveled longer than lambda*d(d1,s2), the interpolation parameter is
             * computed as though \a s2 were the final state visited before this termination. */
            bool checkMotion (const State *s1, const State *s2, std::pair<State *, double> &lastValid) const;
            
        private:
            
            /** \brief Atlas on which we check motion. */
            const AtlasStateSpace &atlas_;
            
            /** \brief Check that the space is, in fact, an AtlasStateSpace. */
            void checkSpace (void);
        };
        
        /** \brief State space encapsulating the atlas algorithm to assist planning on a constraint manifold.
         * \warning Does not comply with OMPL thread-safety assumptions. TODO */
        class AtlasStateSpace : public RealVectorStateSpace
        {
        public:
            
            /** \brief A state in an atlas represented as a real vector in ambient space and a chart pointer. */
            class StateType : public RealVectorStateSpace::StateType
            {
            public:
                
                /** \brief Constructor. Real vector will be of size \a dimension. */
                StateType (const unsigned int dimension);
                
                /** \brief Destructor. */
                ~StateType(void);
                
                /** \brief Set the real vector to the values in \a x and the chart to \a c.
                 * Assumes \a x is of the same dimensionality as the state. */
                void setRealState (const Eigen::VectorXd &x, const AtlasChart &c);
                
                /** \brief Convert this state to an Eigen::VectorXd. */
                Eigen::VectorXd toVector (void) const;
                
                /** \brief Get the chart for the state. */
                const AtlasChart &getChart (void) const;
                
                /** \brief Set the chart for the state. */
                void setChart (const AtlasChart &c);
                
            private:
                
                /** \brief Chart owning the real vector. */
                const AtlasChart *chart_;
                
                /** \brief Dimension of the real vector. */
                const unsigned int dimension_;
            };
            
            /** \brief Constraint function type; input vector size is the ambient dimension;
             * output vector size is the number of constraints. */
            typedef boost::function<Eigen::VectorXd (const Eigen::VectorXd &)> Constraints;
            
            /** \brief Jacobian function type; input vector size is the ambient dimension;
             * output matrix is (number of constraints) by (ambient dimension). */
            typedef boost::function<Eigen::MatrixXd (const Eigen::VectorXd &)> Jacobian;
            
            /** \brief Constructor. The ambient space has dimension \a dimension. The manifold is implicitly defined
             * as { x in R^(\a dimension) : \a constraintFn(x) = Zero }. The Jacobian of \a constraintFn at x
             * is given by \a jacobianFn(x). */
            AtlasStateSpace (const unsigned int dimension, const Constraints constraintFn, const Jacobian jacobianFn);
            
            /** \brief Destructor. */
            ~AtlasStateSpace (void);
            
            /** \brief Final setup for the space. */
            void setup (void);
            
            /** \brief Associate \a si with this space. Requires that \a si was constructed from this AtlasStateSpace. */
            void setSpaceInformation (const SpaceInformationPtr &si);
            
            /** \brief Set \a delta, the step size for traversing the manifold and collision checking. Default is 0.02. */
            void setDelta (const double delta);
            
            /** \brief Set \a epsilon, the maximum permissible distance between a point in the validity region
             * of a chart and its projection onto the manifold. Default is 0.1. */
            void setEpsilon (const double epsilon);
            
            /** \brief Set \a rho, the maximum radius for which a chart is valid. Default is 0.1. If this value
             * is too large, it will be decreased during operation of the atlas. Marked 'const' because operation
             * of the atlas is performed by other const methods. */
            void setRho (const double rho) const;
            
            /** \brief Set \a alpha, the maximum permissible angle between the chart and the manifold inside
             * the validity region of the chart. Default is pi/16. Must be within the range (0,pi/2). */
            void setAlpha (const double alpha);
            
            /** \brief Set the \a exploration parameter, which tunes the balance of refinement (sampling within
             * known regions) and exploration (sampling on the frontier). Valid values are in the range [0,1),
             * where 0 is all refinement, and 1 is all exploration. Default is 0.5. */
            void setExploration (const double exploration);
            
            /** \brief Set \a lambda, where lambda * d(x,y) is the maximum distance that can be accumulated while
             * traversing the manifold from x to y before the algorithm gives up. Default is 2. Must be > 1. */
            void setLambda (const double lambda);
            
            /** \brief Projection from a chart to the manifold will stop if the norm of the error is less than
             * \a tolerance. Default is 1e-8. */
            void setProjectionTolerance (const double tolerance);
            
            /** \brief Projection from a chart to the manifold will stop after at most \a iterations iterations.
             * Default is 200. */
            void setProjectionMaxIterations (unsigned int iterations);
            
            /** \brief The number of samples used in the Monte Carlo integration of a chart's measure is proportional to
             * \a thoroughness ^ (manifold dimension). Default is 3.5. Has a massive performance impact in higher dimensions. */
            void setMonteCarloThoroughness (const double thoroughness);
            
            /** \brief Get delta. */
            double getDelta (void) const;
            
            /** \brief Get epsilon. */
            double getEpsilon (void) const;
            
            /** \brief Get rho. */
            double getRho (void) const;
            
            /** \brief Get alpha. */
            double getAlpha (void) const;
            
            /** \brief Get the exploration parameter. */
            double getExploration (void) const;
            
            /** \brief Get lambda. */
            double getLambda (void) const;
            
            /** \brief Get the sampling radius. */
            double getRho_s (void) const;
            
            /** \brief Get the projection tolerance. */
            double getProjectionTolerance (void) const;
            
            /** \brief Get the maximum number of projection iterations. */
            unsigned int getProjectionMaxIterations (void) const;
            
            /** \brief Get the Monte Carlo integration thoroughness parameter. */
            double getMonteCarloThoroughness (void) const;
            
            /** \brief Get the dimension of the ambient space. */
            unsigned int getAmbientDimension (void) const;
            
            /** \brief Get the dimension of the constraint manifold. */
            unsigned int getManifoldDimension (void) const;
            
            /** \brief Constraint function. Accepts a vector \a x in ambient space. Returns a vector of the amount of violation of
             * each constraint. Returns the zero vector when \a x is on the manifold. */
            const Constraints bigF;
            
            /** \brief Jacobian of the constraint function, F. Accepts a vector \a x in ambient space. Returns the Jacobian of F
             * at \a x. */
            const Jacobian bigJ;
            
            /** \brief Pick a chart at random with probability proportional the chart measure / atlas measure. */
            AtlasChart &sampleChart (void) const;
            
            /** \brief Find the chart to which \a x belongs. Use \a neighbor to hint that the chart
             * may be its neighbor, if that information is available. Returns NULL if no chart found. */
            AtlasChart *owningChart (const Eigen::VectorXd &x, const AtlasChart *const neighbor = NULL) const;
            
            /** \brief Create a new chart for the atlas, centered at \a xorigin, which should be on
             * the manifold. */
            AtlasChart &newChart (const Eigen::VectorXd &xorigin) const;
            
            /** \brief Search for the border of chart \a c between \a xinside, which is assumed to be inside the
             * polytope of \a c, and \a xoutside. The returned point lies inside the border at a distance no farther
             * than half the distance of \a xinside to the border. */
            Eigen::VectorXd dichotomicSearch (const AtlasChart &c, const Eigen::VectorXd &xinside, Eigen::VectorXd xoutside) const;
            
            /** \brief Update the recorded measure of a chart. */
            void updateMeasure (const AtlasChart &c) const;
            
            /** \brief Return the measure of a manifold-dimensional ball of radius sqrt(2)*rho. */
            double measureSqrt2RhoKBall (void) const;
            
            /** \brief Return the number of samples to use in chart measure estimation. */
            unsigned int monteCarloSamples (void) const;
            
            /** \brief Return the number of charts currently in the atlas. */
            std::size_t getChartCount (void) const;
            
            /** \brief Traverse the manifold from \a from toward \a to. Returns true if we reached \a to, and false if
             * we stopped early for any reason, such as a collision or traveling too far. No collision checking is performed
             * if \a interpolate is true. If \a stateList is not NULL, the sequence of intermediates is saved to it, including
             * a copy of \a from, as well as the final state. Caller is responsible for freeing states returned in \a stateList. */
            bool followManifold (const StateType *from, const StateType *to, const bool interpolate = false,
                                 std::vector<StateType *> *const stateList = NULL) const;
            
            /** \brief Find the state between \a from and \a to at time \a t, where \a t = 0 is \a from, and \a t = 1 is the final
             * state reached by followManifold(\a from, \a to, true, ...), which may not be \a to. State returned in \a state. */
            void interpolate (const State *from, const State *to, const double t, State *state) const;
            
            /** \brief Like interpolate(...), but uses the information about intermediate states already supplied in \a stateList from
             * a previous call to followManifold(..., true, \a stateList). The 'from' and 'to' states are the first and last
             * elements \a stateList. Assumes \a stateList contains at least two elements. */
            void fastInterpolate (const std::vector<StateType *> &stateList, const double t, State *state) const;
            
            /** \brief Whether interpolation is symmetric. (No.) */
            bool hasSymmetricInterpolate (void) const;
            
            /** \brief Duplicate \a source in \a destination. The memory for these two states should not overlap. */
            void copyState (State *destination, const State *source) const;
            
            /** \brief Return an instance of the AtlasStateSampler. */
            StateSamplerPtr allocDefaultStateSampler (void) const;
            
            /** \brief Allocate a new state in this space. */
            State *allocState (void) const;
            
            /** \brief Free \a state. Assumes \a state is an AtlasStateSpace state. */
            void freeState (State *state) const;
            
        private:
            
            /** \brief Ambient space dimension. */
            const unsigned int n_;
            
            /** \brief Manifold dimension. */
            unsigned int k_;
            
            /** \brief SpaceInformation associated with this space. */
            SpaceInformationPtr si_;
            
            /** \brief Random number generator. */
            mutable RNG rng_;
            
            /** \brief List of charts, sampleable by weight. */
            mutable PDF<AtlasChart *> charts_;
            
            /** \brief Step size when traversing the manifold and collision checking. */
            double delta_;
            
            /** \brief Maximum distance between a chart and the manifold inside its validity region. */
            double epsilon_;
            
            /** \brief Maximum radius of chart validity region. */
            mutable double rho_;
            
            /** \brief Cosine of the maximum angle between a chart and teh manifold inside its validity region. */
            double cos_alpha_;
            
            /** \brief Balance between explorationa and refinement. */
            double exploration_;
            
            /** \brief Manifold traversal from x to y is stopped if accumulated distance is greater than d(x,y) times this. */
            double lambda_;
            
            /** \brief Sampling radius within a chart. Inferred from rho and exploration parameters. */
            mutable double rho_s_;
            
            /** \brief Tolerance for Newton method used in projection onto manifold. */
            double projectionTolerance_;
            
            /** \brief Maximum number of iterations for Newton method used in projection onto manifold. */
            unsigned int projectionMaxIterations_;
            
            /** \brief Parameter which tunes the number of samples used by Monte Carlo integration. */
            double monteCarloThoroughness_;
            
            /** \brief Measure of a k-ball of radius sqrt(2)*rho. */
            mutable double ballMeasure_;
        };
    }
}

#endif
