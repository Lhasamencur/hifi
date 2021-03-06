//
//  toyball.js
//  examples
//
//  Created by Brad Hefta-Gaub on 1/20/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  This is an example script that turns the hydra controllers into a toy ball catch and throw game.
//  It reads the controller, watches for button presses and trigger pulls, and launches particles.
//
//  The particles it creates have a script that when they collide with Voxels, the
//  particle will change it's color to match the voxel it hits.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// maybe we should make these constants...
var LEFT_PALM = 0;
var LEFT_TIP = 1;
var LEFT_BUTTON_FWD = 5;
var LEFT_BUTTON_3 = 3;

var RIGHT_PALM = 2;
var RIGHT_TIP = 3;
var RIGHT_BUTTON_FWD = 11;
var RIGHT_BUTTON_3 = 9;

var leftBallAlreadyInHand = false;
var rightBallAlreadyInHand = false;
var leftHandParticle;
var rightHandParticle;

var throwSound = new Sound("https://dl.dropboxusercontent.com/u/1864924/hifi-sounds/throw.raw");
var catchSound = new Sound("https://dl.dropboxusercontent.com/u/1864924/hifi-sounds/catch.raw");
var targetRadius = 0.25;


var wantDebugging = false;
function debugPrint(message) {
    if (wantDebugging) {
        print(message);
    }
}

function getBallHoldPosition(whichSide) { 
    var normal;
    var tipPosition;
    if (whichSide == LEFT_PALM) {
        normal = Controller.getSpatialControlNormal(LEFT_PALM);
        tipPosition = Controller.getSpatialControlPosition(LEFT_TIP);
    } else {
        normal = Controller.getSpatialControlNormal(RIGHT_PALM);
        tipPosition = Controller.getSpatialControlPosition(RIGHT_TIP);
    }
    
    var BALL_FORWARD_OFFSET = 0.08; // put the ball a bit forward of fingers
    position = { x: BALL_FORWARD_OFFSET * normal.x,
                 y: BALL_FORWARD_OFFSET * normal.y, 
                 z: BALL_FORWARD_OFFSET * normal.z }; 

    position.x += tipPosition.x;
    position.y += tipPosition.y;
    position.z += tipPosition.z;
    
    return position;
}

function checkControllerSide(whichSide) {
    var BUTTON_FWD;
    var BUTTON_3;
    var palmPosition;
    var ballAlreadyInHand;
    var handMessage;
    
    if (whichSide == LEFT_PALM) {
        BUTTON_FWD = LEFT_BUTTON_FWD;
        BUTTON_3 = LEFT_BUTTON_3;
        palmPosition = Controller.getSpatialControlPosition(LEFT_PALM);
        ballAlreadyInHand = leftBallAlreadyInHand;
        handMessage = "LEFT";
    } else {
        BUTTON_FWD = RIGHT_BUTTON_FWD;
        BUTTON_3 = RIGHT_BUTTON_3;
        palmPosition = Controller.getSpatialControlPosition(RIGHT_PALM);
        ballAlreadyInHand = rightBallAlreadyInHand;
        handMessage = "RIGHT";
    }
    
    var grabButtonPressed = (Controller.isButtonPressed(BUTTON_FWD) || Controller.isButtonPressed(BUTTON_3));

    // If I don't currently have a ball in my hand, then try to catch closest one
    if (!ballAlreadyInHand && grabButtonPressed) {
        var closestParticle = Particles.findClosestParticle(palmPosition, targetRadius);

        if (closestParticle.isKnownID) {

            debugPrint(handMessage + " HAND- CAUGHT SOMETHING!!");

            if (whichSide == LEFT_PALM) {
                leftBallAlreadyInHand = true;
                leftHandParticle = closestParticle;
            } else {
                rightBallAlreadyInHand = true;
                rightHandParticle = closestParticle;
            }
            var ballPosition = getBallHoldPosition(whichSide);
            var properties = { position: { x: ballPosition.x, 
                                           y: ballPosition.y, 
                                           z: ballPosition.z }, 
                                velocity : { x: 0, y: 0, z: 0}, inHand: true };
            Particles.editParticle(closestParticle, properties);
            
    		var options = new AudioInjectionOptions(); 
			options.position = ballPosition;
			options.volume = 1.0;
			Audio.playSound(catchSound, options);
            
            return; // exit early
        }
    }

    // change ball color logic...
    //
    //if (wasButtonJustPressed()) {
    //    rotateColor();
    //}

    //  If '3' is pressed, and not holding a ball, make a new one
    if (Controller.isButtonPressed(BUTTON_3) && !ballAlreadyInHand) {
        var ballPosition = getBallHoldPosition(whichSide);
        var properties = { position: { x: ballPosition.x, 
                                       y: ballPosition.y, 
                                       z: ballPosition.z }, 
                velocity: { x: 0, y: 0, z: 0}, 
                gravity: { x: 0, y: 0, z: 0}, 
                inHand: true,
                radius: 0.05,
                damping: 0.999,
                color: { red: 255, green: 0, blue: 0 },

                lifetime: 10 // 10 seconds - same as default, not needed but here as an example
            };

        newParticle = Particles.addParticle(properties);
        if (whichSide == LEFT_PALM) {
            leftBallAlreadyInHand = true;
            leftHandParticle = newParticle;
        } else {
            rightBallAlreadyInHand = true;
            rightHandParticle = newParticle;
        }

        // Play a new ball sound
        var options = new AudioInjectionOptions(); 
        options.position = ballPosition;
        options.volume = 1.0;
        Audio.playSound(catchSound, options);
        
        return; // exit early
    }

    if (ballAlreadyInHand) {
        if (whichSide == LEFT_PALM) {
            handParticle = leftHandParticle;
            whichTip = LEFT_TIP;
        } else {
            handParticle = rightHandParticle;
            whichTip = RIGHT_TIP;
        }

        //  If holding the ball keep it in the palm
        if (grabButtonPressed) {
            debugPrint(">>>>> " + handMessage + "-BALL IN HAND, grabbing, hold and move");
            var ballPosition = getBallHoldPosition(whichSide);
            var properties = { position: { x: ballPosition.x, 
                                           y: ballPosition.y, 
                                           z: ballPosition.z }, 
                };
            Particles.editParticle(handParticle, properties);
        } else {
            debugPrint(">>>>> " + handMessage + "-BALL IN HAND, not grabbing, THROW!!!");
            //  If toy ball just released, add velocity to it!
            var tipVelocity = Controller.getSpatialControlVelocity(whichTip);
            var THROWN_VELOCITY_SCALING = 1.5;
            var properties = { 
                    velocity: { x: tipVelocity.x * THROWN_VELOCITY_SCALING, 
                                y: tipVelocity.y * THROWN_VELOCITY_SCALING, 
                                z: tipVelocity.z * THROWN_VELOCITY_SCALING } ,
                    inHand: false,
                    gravity: { x: 0, y: -2, z: 0}, 
                };

            Particles.editParticle(handParticle, properties);

            if (whichSide == LEFT_PALM) {
                leftBallAlreadyInHand = false;
                leftHandParticle = false;
            } else {
                rightBallAlreadyInHand = false;
                rightHandParticle = false;
            }

    		var options = new AudioInjectionOptions(); 
			options.position = ballPosition;
			options.volume = 1.0;
			Audio.playSound(throwSound, options);
        }
    }
}


function checkController(deltaTime) {
    var numberOfButtons = Controller.getNumberOfButtons();
    var numberOfTriggers = Controller.getNumberOfTriggers();
    var numberOfSpatialControls = Controller.getNumberOfSpatialControls();
    var controllersPerTrigger = numberOfSpatialControls / numberOfTriggers;

    // this is expected for hydras
    if (!(numberOfButtons==12 && numberOfTriggers == 2 && controllersPerTrigger == 2)) {
        debugPrint("no hydra connected?");
        return; // bail if no hydra
    }

    checkControllerSide(LEFT_PALM);
    checkControllerSide(RIGHT_PALM);
}


// register the call back so it fires before each data send
Script.update.connect(checkController);
