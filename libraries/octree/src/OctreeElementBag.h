//
//  OctreeElementBag.h
//  libraries/octree/src
//
//  Created by Brad Hefta-Gaub on 4/25/2013.
//  Copyright 2013 High Fidelity, Inc.
//
//  This class is used by the VoxelTree:encodeTreeBitstream() functions to store extra nodes that need to be sent
//  it's a generic bag style storage mechanism. But It has the property that you can't put the same node into the bag
//  more than once (in other words, it de-dupes automatically), also, it supports collapsing it's several peer nodes
//  into a parent node in cases where you add enough peers that it makes more sense to just add the parent.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OctreeElementBag_h
#define hifi_OctreeElementBag_h

#include "OctreeElement.h"

class OctreeElementBag : public OctreeElementDeleteHook {

public:
    OctreeElementBag();
    ~OctreeElementBag();
    
    void insert(OctreeElement* element); // put a element into the bag
    OctreeElement* extract(); // pull a element out of the bag (could come in any order)
    bool contains(OctreeElement* element); // is this element in the bag?
    void remove(OctreeElement* element); // remove a specific element from the bag
    
    bool isEmpty() const { return _bagElements.isEmpty(); }
    int count() const { return _bagElements.size(); }

    void deleteAll();
    virtual void elementDeleted(OctreeElement* element);

private:
    QSet<OctreeElement*> _bagElements;
};

#endif // hifi_OctreeElementBag_h
