/*********************************************************************
* Software License Agreement (BSD License)
*
*  Copyright (c) 2014, University of Toronto
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
*   * Neither the name of the University of Toronto nor the names of its
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

/* Authors: Jonathan Gammell */

// My definition:
#include "ompl/geometric/planners/bitstar/datastructures/Vertex.h"

// For std::move
#include <utility>
// For std::swap
#include <algorithm>

// For exceptions:
#include "ompl/util/Exception.h"

// BIT*:
// A collection of common helper functions
#include "ompl/geometric/planners/bitstar/datastructures/HelperFunctions.h"
// The ID generator class
#include "ompl/geometric/planners/bitstar/datastructures/IdGenerator.h"
// The cost-helper class:
#include "ompl/geometric/planners/bitstar/datastructures/CostHelper.h"

// Debug macros
#ifdef BITSTAR_DEBUG
    // Debug setting. The id number of a vertex to track. Requires BITSTAR_DEBUG to be defined in BITstar.h
    #define TRACK_VERTEX_ID 0

    /** \brief A helper function to print out every function called on vertex "TRACK_VERTEX_ID" that changes it */
    #define PRINT_VERTEX_CHANGE \
        if (vId_ == TRACK_VERTEX_ID) \
        { \
            std::cout << "vId " << vId_ << ": " << __func__ << "()" << std::endl; \
        }

    /** \brief A debug-only call to assert that the vertex is not pruned. */
    #define ASSERT_NOT_PRUNED this->assertNotPruned();
#else
    #define PRINT_VERTEX_CHANGE
    #define ASSERT_NOT_PRUNED
#endif

namespace ompl
{
    namespace geometric
    {
        /////////////////////////////////////////////////////////////////////////////////////////////
        // Public functions:
        BITstar::Vertex::Vertex(ompl::base::SpaceInformationPtr si, const CostHelperPtr &costHelpPtr,
                                bool root /*= false*/)
          : vId_(getIdGenerator().getNewId())
          , si_(std::move(si))
          , costHelpPtr_(std::move(costHelpPtr))
          , state_(si_->allocState())
          , isRoot_(root)
          , edgeCost_(costHelpPtr_->infiniteCost())
          , cost_(costHelpPtr_->infiniteCost())
        {
            PRINT_VERTEX_CHANGE

            if (this->isRoot())
            {
                cost_ = costHelpPtr_->identityCost();
            }
            // No else, infinite by default
        }

        BITstar::Vertex::~Vertex()
        {
            PRINT_VERTEX_CHANGE

            // Free the state on destruction
            si_->freeState(state_);
        }

        BITstar::VertexId BITstar::Vertex::getId() const
        {
            ASSERT_NOT_PRUNED

            return vId_;
        }

        ompl::base::State const *BITstar::Vertex::stateConst() const
        {
            ASSERT_NOT_PRUNED

            return state_;
        }

        ompl::base::State *BITstar::Vertex::state()
        {
            PRINT_VERTEX_CHANGE
            ASSERT_NOT_PRUNED

            return state_;
        }

        bool BITstar::Vertex::isRoot() const
        {
            ASSERT_NOT_PRUNED

            return isRoot_;
        }

        bool BITstar::Vertex::hasParent() const
        {
            ASSERT_NOT_PRUNED

            return static_cast<bool>(parentSPtr_);
        }

        bool BITstar::Vertex::isInTree() const
        {
            ASSERT_NOT_PRUNED

            return this->isRoot() || this->hasParent();
        }

        unsigned int BITstar::Vertex::getDepth() const
        {
            ASSERT_NOT_PRUNED

#ifdef BITSTAR_DEBUG
            if (this->isRoot() == false && this->hasParent() == false)
            {
                throw ompl::Exception("Attempting to get the depth of a vertex that does not have a parent yet is not "
                                      "root.");
            }
#endif  // BITSTAR_DEBUG

            return depth_;
        }

        BITstar::VertexConstPtr BITstar::Vertex::getParentConst() const
        {
            ASSERT_NOT_PRUNED

#ifdef BITSTAR_DEBUG
            if (this->hasParent() == false)
            {
                if (this->isRoot() == true)
                {
                    throw ompl::Exception("Attempting to access the parent of the root vertex.");
                }
                else
                {
                    throw ompl::Exception("Attempting to access the parent of a vertex that does not have one.");
                }
            }
#endif  // BITSTAR_DEBUG

            return parentSPtr_;
        }

        BITstar::VertexPtr BITstar::Vertex::getParent()
        {
            ASSERT_NOT_PRUNED

#ifdef BITSTAR_DEBUG
            if (this->hasParent() == false)
            {
                if (this->isRoot() == true)
                {
                    throw ompl::Exception("Attempting to access the parent of the root vertex.");
                }
                else
                {
                    throw ompl::Exception("Attempting to access the parent of a vertex that does not have one.");
                }
            }
#endif  // BITSTAR_DEBUG

            return parentSPtr_;
        }

        void BITstar::Vertex::addParent(const VertexPtr &newParent, const ompl::base::Cost &edgeInCost,
                                        bool updateChildCosts /*= true*/)
        {
            PRINT_VERTEX_CHANGE
            ASSERT_NOT_PRUNED

#ifdef BITSTAR_DEBUG
            if (this->hasParent() == true)
            {
                throw ompl::Exception("Attempting to add a parent to a vertex that already has one.");
            }
            if (this->isRoot() == true)
            {
                throw ompl::Exception("Attempting to add a parent to the root vertex, which cannot have a parent.");
            }
#endif  // BITSTAR_DEBUG

            // Store the parent
            parentSPtr_ = newParent;

            // Store the edge cost
            edgeCost_ = edgeInCost;

            // Update my cost
            this->updateCostAndDepth(updateChildCosts);
        }

        void BITstar::Vertex::removeParent(bool updateChildCosts /*= true*/)
        {
            PRINT_VERTEX_CHANGE
            ASSERT_NOT_PRUNED

#ifdef BITSTAR_DEBUG
            if (this->hasParent() == false)
            {
                throw ompl::Exception("Attempting to remove the parent of a vertex that does not have a parent.");
            }
            if (this->isRoot() == true)
            {
                throw ompl::Exception("Attempting to remove the parent of the root vertex, which cannot have a "
                                      "parent.");
            }
#endif  // BITSTAR_DEBUG

            // Clear my parent
            parentSPtr_.reset();

            // Update costs:
            this->updateCostAndDepth(updateChildCosts);
        }

        bool BITstar::Vertex::hasChildren() const
        {
            ASSERT_NOT_PRUNED

            return !childWPtrs_.empty();
        }

        void BITstar::Vertex::getChildrenConst(VertexConstPtrVector *children) const
        {
            ASSERT_NOT_PRUNED

            children->clear();

            for (const auto &childWPtr : childWPtrs_)
            {
#ifdef BITSTAR_DEBUG
                // Check that the weak pointer hasn't expired
                if (childWPtr.expired() == true)
                {
                    throw ompl::Exception("A (weak) pointer to a child was found to have expired while collecting the "
                                          "children of a vertex.");
                }
#endif  // BITSTAR_DEBUG

                // Lock and push back
                children->push_back(childWPtr.lock());
            }
        }

        void BITstar::Vertex::getChildren(VertexPtrVector *children)
        {
            ASSERT_NOT_PRUNED

            children->clear();

            for (const auto &childWPtr : childWPtrs_)
            {
#ifdef BITSTAR_DEBUG
                // Check that the weak pointer hasn't expired
                if (childWPtr.expired() == true)
                {
                    throw ompl::Exception("A (weak) pointer to a child was found to have expired while collecting the "
                                          "children of a vertex.");
                }
#endif  // BITSTAR_DEBUG

                // Lock and push back
                children->push_back(childWPtr.lock());
            }
        }

        void BITstar::Vertex::addChild(const VertexPtr &newChild, bool updateChildCosts /*= true*/)
        {
            PRINT_VERTEX_CHANGE
            ASSERT_NOT_PRUNED

            // Push back the shared_ptr into the vector of weak_ptrs, this makes a weak_ptr copy
            childWPtrs_.push_back(newChild);

            if (updateChildCosts)
            {
                newChild->updateCostAndDepth(true);
            }
            // No else, leave the costs out of date.
        }

        void BITstar::Vertex::removeChild(const VertexPtr &oldChild, bool updateChildCosts /*= true*/)
        {
            PRINT_VERTEX_CHANGE
            ASSERT_NOT_PRUNED

            // Variables
            // Whether the child has been found (and then deleted);
            bool foundChild;

            // Iterate over the vector of children pointers until the child is found. Iterators make erase easier
            foundChild = false;
            for (auto childIter = childWPtrs_.begin(); childIter != childWPtrs_.end() && !foundChild;
                 ++childIter)
            {
#ifdef BITSTAR_DEBUG
                // Check that the weak pointer hasn't expired
                if (childIter->expired() == true)
                {
                    throw ompl::Exception("A (weak) pointer to a child was found to have expired while removing a "
                                          "child from a vertex.");
                }
#endif  // BITSTAR_DEBUG

                // Check if this is the child we're looking for
                if (childIter->lock()->getId() == oldChild->getId())
                {
                    // It is, mark as found
                    foundChild = true;

                    // Remove the child from the vector
                    // Swap to the end
                    if (childIter != (childWPtrs_.end() - 1))
                    {
                        std::swap(*childIter, childWPtrs_.back());
                    }

                    // Pop it off the end
                    childWPtrs_.pop_back();
                }
                // No else, move on
            }

            // Update the child cost if appropriate
            if (updateChildCosts)
            {
                oldChild->updateCostAndDepth(true);
            }
// No else, leave the costs out of date.

#ifdef BITSTAR_DEBUG
            // Throw if we did not find the child
            if (foundChild == false)
            {
                throw ompl::Exception("Attempting to remove a child vertex not present in the vector of children "
                                      "stored in the (supposed) parent vertex.");
            }
#endif  // BITSTAR_DEBUG
        }

        ompl::base::Cost BITstar::Vertex::getCost() const
        {
            ASSERT_NOT_PRUNED

            return cost_;
        }

        ompl::base::Cost BITstar::Vertex::getEdgeInCost() const
        {
            ASSERT_NOT_PRUNED

#ifdef BITSTAR_DEBUG
            if (this->hasParent() == false)
            {
                throw ompl::Exception("Attempting to access the incoming-edge cost of a vertex without a parent.");
            }
#endif  // BITSTAR_DEBUG

            return edgeCost_;
        }

        bool BITstar::Vertex::isNew() const
        {
            ASSERT_NOT_PRUNED

            return isNew_;
        }

        void BITstar::Vertex::markNew()
        {
            PRINT_VERTEX_CHANGE
            ASSERT_NOT_PRUNED

            isNew_ = true;
        }

        void BITstar::Vertex::markOld()
        {
            PRINT_VERTEX_CHANGE
            ASSERT_NOT_PRUNED

            isNew_ = false;
        }

        bool BITstar::Vertex::hasBeenExpandedToSamples() const
        {
            ASSERT_NOT_PRUNED

            return hasBeenExpandedToSamples_;
        }

        void BITstar::Vertex::markExpandedToSamples()
        {
            PRINT_VERTEX_CHANGE
            ASSERT_NOT_PRUNED

            hasBeenExpandedToSamples_ = true;
        }

        void BITstar::Vertex::markUnexpandedToSamples()
        {
            PRINT_VERTEX_CHANGE
            ASSERT_NOT_PRUNED

            hasBeenExpandedToSamples_ = false;
        }

        bool BITstar::Vertex::hasBeenExpandedToVertices() const
        {
            ASSERT_NOT_PRUNED

            return hasBeenExpandedToVertices_;
        }

        void BITstar::Vertex::markExpandedToVertices()
        {
            PRINT_VERTEX_CHANGE
            ASSERT_NOT_PRUNED

            hasBeenExpandedToVertices_ = true;
        }

        void BITstar::Vertex::markUnexpandedToVertices()
        {
            PRINT_VERTEX_CHANGE
            ASSERT_NOT_PRUNED

            hasBeenExpandedToVertices_ = false;
        }

        bool BITstar::Vertex::isPruned() const
        {
            return isPruned_;
        }

        void BITstar::Vertex::markPruned()
        {
            PRINT_VERTEX_CHANGE
            ASSERT_NOT_PRUNED

            isPruned_ = true;
        }

        void BITstar::Vertex::markUnpruned()
        {
            PRINT_VERTEX_CHANGE

            isPruned_ = false;
        }
        /////////////////////////////////////////////////////////////////////////////////////////////

        /////////////////////////////////////////////////////////////////////////////////////////////
        // Protected functions:
        void BITstar::Vertex::updateCostAndDepth(bool cascadeUpdates /*= true*/)
        {
            PRINT_VERTEX_CHANGE
            ASSERT_NOT_PRUNED

            if (this->isRoot())
            {
                // Am I root? -- I don't really know how this would ever be called, but ok.
                cost_ = costHelpPtr_->identityCost();
                depth_ = 0u;
            }
            else if (!this->hasParent())
            {
                // Am I disconnected?
                cost_ = costHelpPtr_->infiniteCost();

                // Set the depth to 0u, getDepth will throw in this condition
                depth_ = 0u;

#ifdef BITSTAR_DEBUG
                // Assert that I have not been asked to cascade this bad data to my children:
                if (this->hasChildren() == true && cascadeUpdates == true)
                {
                    throw ompl::Exception("Attempting to update descendants' costs and depths of a vertex that does "
                                          "not have a parent and is not root. This information would therefore be "
                                          "gibberish.");
                }
#endif  // BITSTAR_DEBUG
            }
            else
            {
                // I have a parent, so my cost is my parent cost + my edge cost to the parent
                cost_ = costHelpPtr_->combineCosts(parentSPtr_->getCost(), edgeCost_);

                // I am one more than my parent's depth:
                depth_ = (parentSPtr_->getDepth() + 1u);
            }

            // Am I updating my children?
            if (cascadeUpdates)
            {
                // Now, iterate over my vector of children and tell each one to update its own damn cost:
                for (auto &childWPtr : childWPtrs_)
                {
#ifdef BITSTAR_DEBUG
                    // Check that it hasn't expired
                    if (childWPtr.expired() == true)
                    {
                        throw ompl::Exception("A (weak) pointer to a child has was found to have expired while "
                                              "updating the costs and depths of descendant vertices.");
                    }
#endif  // BITSTAR_DEBUG

                    // Get a lock and tell the child to update:
                    childWPtr.lock()->updateCostAndDepth(true);
                }
            }
            // No else, do not update the children. I hope the caller knows what they're doing.
        }
        /////////////////////////////////////////////////////////////////////////////////////////////

        /////////////////////////////////////////////////////////////////////////////////////////////
        // Private functions:
        void BITstar::Vertex::assertNotPruned() const
        {
            if (isPruned_ == true)
            {
                std::cout << std::endl
                          << "vId: " << vId_ << std::endl;
                throw ompl::Exception("Attempting to access a pruned vertex.");
            }
        }
        /////////////////////////////////////////////////////////////////////////////////////////////
    }  // geometric
}  // ompl
