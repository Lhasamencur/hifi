//
//  VoxelTreeCommands.cpp
//  libraries/voxels/src
//
//  Created by Clement on 4/4/14.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "VoxelTree.h"

#include "VoxelTreeCommands.h"

AddVoxelCommand::AddVoxelCommand(VoxelTree* tree, VoxelDetail& voxel, VoxelEditPacketSender* packetSender, QUndoCommand* parent) :
    QUndoCommand("Add Voxel", parent),
    _tree(tree),
    _packetSender(packetSender),
    _voxel(voxel)
{
}

void AddVoxelCommand::redo() {
    if (_tree) {
        _tree->createVoxel(_voxel.x, _voxel.y, _voxel.z, _voxel.s, _voxel.red, _voxel.green, _voxel.blue);
    }
    if (_packetSender) {
        _packetSender->queueVoxelEditMessages(PacketTypeVoxelSet, 1, &_voxel);
    }
}

void AddVoxelCommand::undo() {
    if (_tree) {
        _tree->deleteVoxelAt(_voxel.x, _voxel.y, _voxel.z, _voxel.s);
    }
    if (_packetSender) {
        _packetSender->queueVoxelEditMessages(PacketTypeVoxelErase, 1, &_voxel);
    }
}

DeleteVoxelCommand::DeleteVoxelCommand(VoxelTree* tree, VoxelDetail& voxel, VoxelEditPacketSender* packetSender, QUndoCommand* parent) :
    QUndoCommand("Delete Voxel", parent),
    _tree(tree),
    _packetSender(packetSender),
    _voxel(voxel)
{
}

void DeleteVoxelCommand::redo() {
    if (_tree) {
        _tree->deleteVoxelAt(_voxel.x, _voxel.y, _voxel.z, _voxel.s);
    }
    if (_packetSender) {
        _packetSender->queueVoxelEditMessages(PacketTypeVoxelErase, 1, &_voxel);
    }
}

void DeleteVoxelCommand::undo() {
    if (_tree) {
        _tree->createVoxel(_voxel.x, _voxel.y, _voxel.z, _voxel.s, _voxel.red, _voxel.green, _voxel.blue);
    }
    if (_packetSender) {
        _packetSender->queueVoxelEditMessages(PacketTypeVoxelSet, 1, &_voxel);
    }
}
