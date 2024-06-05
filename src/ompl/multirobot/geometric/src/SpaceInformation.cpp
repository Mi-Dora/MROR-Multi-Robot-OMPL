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

/* Author: Steve Du */

#include "ompl/multirobot/geometric/SpaceInformation.h"


ompl::multirobot::geometric::SpaceInformation::SpaceInformation(): ompl::multirobot::base::SpaceInformation()
{
}

// unsigned int ompl::multirobot::control::SpaceInformation::getIndividualCount() const
// {
//     return individualCount_;
// }

const ompl::base::SpaceInformationPtr &ompl::multirobot::geometric::SpaceInformation::getIndividual(const unsigned int index) const
{
    if (individualCount_ > index)
        return individuals_[index];
    else
        throw Exception("Subspace index does not exist");
}

void ompl::multirobot::geometric::SpaceInformation::addIndividual(const ompl::base::SpaceInformationPtr &individual)
{
    if (locked_)
        throw Exception("SpaceInformation is locked and unable to add another individual");
    individuals_.push_back(individual);
    individualCount_ = individuals_.size();
}

void ompl::multirobot::geometric::SpaceInformation::setup()
{
    for (unsigned int i = 0; i < individualCount_; ++i)
        individuals_[i]->setup();
    setup_ = true;
}
