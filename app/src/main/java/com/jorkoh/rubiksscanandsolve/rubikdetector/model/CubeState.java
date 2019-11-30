package com.jorkoh.rubiksscanandsolve.rubikdetector.model;

import androidx.annotation.NonNull;

public class CubeState {
    public Face[] facelets;

    public CubeState() {
        this(new Face[54]);
    }

    public CubeState(Face[] facelets) {
        this.facelets = facelets;
    }

    public CubeState(@NonNull CubeState cubeState) {
        this.facelets = cubeState.facelets;
    }

    public enum Face {
        UP, FRONT, RIGHT, DOWN, LEFT, BACK
    }
}
