//
//  ShapeColliderTests.h
//  tests/physics/src
//
//  Created by Andrew Meadows on 02/21/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ShapeColliderTests_h
#define hifi_ShapeColliderTests_h

namespace ShapeColliderTests {

    void sphereMissesSphere();
    void sphereTouchesSphere();

    void sphereMissesCapsule();
    void sphereTouchesCapsule();

    void capsuleMissesCapsule();
    void capsuleTouchesCapsule();

    void runAllTests(); 
}

#endif // hifi_ShapeColliderTests_h
