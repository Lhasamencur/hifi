//
//  HandData.cpp
//  libraries/avatars/src
//
//  Created by Stephen Birarda on 5/20/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QDataStream>

#include <GeometryUtil.h>
#include <SharedUtil.h>

#include "AvatarData.h" 
#include "HandData.h"


HandData::HandData(AvatarData* owningAvatar) :
    _owningAvatarData(owningAvatar)
{
    // Start with two palms
    addNewPalm();
    addNewPalm();
}

glm::vec3 HandData::worldVectorToLeapVector(const glm::vec3& worldVector) const {
    return glm::inverse(getBaseOrientation()) * worldVector / LEAP_UNIT_SCALE;
}

PalmData& HandData::addNewPalm()  {
    _palms.push_back(PalmData(this));
    return _palms.back();
}

const PalmData* HandData::getPalm(int sixSenseID) const {
    // the palms are not necessarily added in left-right order, 
    // so we have to search for the right SixSenseID
    for (unsigned int i = 0; i < _palms.size(); i++) {
        const PalmData* palm = &(_palms[i]);
        if (palm->getSixenseID() == sixSenseID) {
            return palm->isActive() ? palm : NULL;
        }
    }
    return NULL;
}

void HandData::getLeftRightPalmIndices(int& leftPalmIndex, int& rightPalmIndex) const {
    leftPalmIndex = -1;
    rightPalmIndex = -1;
    for (size_t i = 0; i < _palms.size(); i++) {
        const PalmData& palm = _palms[i];
        if (palm.isActive()) {
            if (palm.getSixenseID() == SIXENSE_CONTROLLER_ID_LEFT_HAND) {
                leftPalmIndex = i;
            }
            if (palm.getSixenseID() == SIXENSE_CONTROLLER_ID_RIGHT_HAND) {
                rightPalmIndex = i;
            }
        }
    }
}

PalmData::PalmData(HandData* owningHandData) :
_rawRotation(0.f, 0.f, 0.f, 1.f),
_rawPosition(0.f),
_rawNormal(0.f, 1.f, 0.f),
_rawVelocity(0.f),
_rotationalVelocity(0.f),
_totalPenetration(0.f),
_controllerButtons(0),
_isActive(false),
_leapID(LEAPID_INVALID),
_sixenseID(SIXENSEID_INVALID),
_numFramesWithoutData(0),
_owningHandData(owningHandData),
_isCollidingWithVoxel(false),
_isCollidingWithPalm(false),
_collisionlessPaddleExpiry(0)
{
    for (int i = 0; i < NUM_FINGERS_PER_HAND; ++i) {
        _fingers.push_back(FingerData(this, owningHandData));
    }
}

void PalmData::addToPosition(const glm::vec3& delta) {
    // convert to Leap coordinates, then add to palm and finger positions
    glm::vec3 leapDelta = _owningHandData->worldVectorToLeapVector(delta);
    _rawPosition += leapDelta;
    for (size_t i = 0; i < getNumFingers(); i++) {
        FingerData& finger = _fingers[i];
        if (finger.isActive()) {
            finger.setRawTipPosition(finger.getTipRawPosition() + leapDelta);
            finger.setRawRootPosition(finger.getRootRawPosition() + leapDelta);
        }
    }
}

FingerData::FingerData(PalmData* owningPalmData, HandData* owningHandData) :
_tipRawPosition(0, 0, 0),
_rootRawPosition(0, 0, 0),
_isActive(false),
_leapID(LEAPID_INVALID),
_numFramesWithoutData(0),
_owningPalmData(owningPalmData),
_owningHandData(owningHandData)
{
    const int standardTrailLength = 10;
    setTrailLength(standardTrailLength);
}

void HandData::setFingerTrailLength(unsigned int length) {
    for (size_t i = 0; i < getNumPalms(); ++i) {
        PalmData& palm = getPalms()[i];
        for (size_t f = 0; f < palm.getNumFingers(); ++f) {
            FingerData& finger = palm.getFingers()[f];
            finger.setTrailLength(length);
        }
    }
}

void HandData::updateFingerTrails() {
    for (size_t i = 0; i < getNumPalms(); ++i) {
        PalmData& palm = getPalms()[i];
        for (size_t f = 0; f < palm.getNumFingers(); ++f) {
            FingerData& finger = palm.getFingers()[f];
            finger.updateTrail();
        }
    }
}

bool HandData::findSpherePenetration(const glm::vec3& penetratorCenter, float penetratorRadius, glm::vec3& penetration, 
                                        const PalmData*& collidingPalm) const {
    
    for (size_t i = 0; i < _palms.size(); ++i) {
        const PalmData& palm = _palms[i];
        if (!palm.isActive()) {
            continue;
        }
        glm::vec3 palmPosition = palm.getPosition();
        const float PALM_RADIUS = 0.05f; // in world (not voxel) coordinates
        if (findSphereSpherePenetration(penetratorCenter, penetratorRadius, palmPosition, PALM_RADIUS, penetration)) {
            collidingPalm = &palm;
            return true;
        }
    }
    return false;
}

glm::quat HandData::getBaseOrientation() const {
    return _owningAvatarData->getOrientation();
}

glm::vec3 HandData::getBasePosition() const {
    return _owningAvatarData->getPosition();
}

void FingerData::setTrailLength(unsigned int length) {
    _tipTrailPositions.resize(length);
    _tipTrailCurrentStartIndex = 0;
    _tipTrailCurrentValidLength = 0;
}

void FingerData::updateTrail() {
    if (_tipTrailPositions.size() == 0)
        return;
    
    if (_isActive) {
        // Add the next point in the trail.
        _tipTrailCurrentStartIndex--;
        if (_tipTrailCurrentStartIndex < 0)
            _tipTrailCurrentStartIndex = _tipTrailPositions.size() - 1;
        
        _tipTrailPositions[_tipTrailCurrentStartIndex] = getTipPosition();
        
        if (_tipTrailCurrentValidLength < (int)_tipTrailPositions.size())
            _tipTrailCurrentValidLength++;
    }
    else {
        // It's not active, so just kill the trail.
        _tipTrailCurrentValidLength = 0;
    }
}

int FingerData::getTrailNumPositions() {
    return _tipTrailCurrentValidLength;
}

const glm::vec3& FingerData::getTrailPosition(int index) {
    if (index >= _tipTrailCurrentValidLength) {
        static glm::vec3 zero(0,0,0);
        return zero;
    }
    int posIndex = (index + _tipTrailCurrentStartIndex) % _tipTrailCurrentValidLength;
    return _tipTrailPositions[posIndex];
}

void PalmData::getBallHoldPosition(glm::vec3& position) const { 
    const float BALL_FORWARD_OFFSET = 0.08f;    // put the ball a bit forward of fingers
    position = BALL_FORWARD_OFFSET * getNormal(); 
    if (_fingers.size() > 0) {
        position += _fingers[0].getTipPosition();
    } else {
        position += getPosition();
    }
}



