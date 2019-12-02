package com.jorkoh.rubiksscanandsolve.rubikdetector.model;

import android.os.Parcel;
import android.os.Parcelable;

import androidx.annotation.NonNull;

public class CubeState implements Parcelable {
    public Face[] facelets;
    public int[] colors;

    public enum Face {
        UP, FRONT, RIGHT, DOWN, LEFT, BACK
    }

    public CubeState() {
        this(new Face[54], new int[6]);
    }

    public CubeState(Face[] facelets, int[] colors) {
        this.facelets = facelets;
        this.colors = colors;
    }

    public CubeState(@NonNull CubeState cubeState) {
        this.facelets = cubeState.facelets;
        this.colors = cubeState.colors;
    }

    protected CubeState(Parcel in) {
        int[] faceletsInt = new int[54];
        in.readIntArray(faceletsInt);
        Face[] values = Face.values();
        for (int j = 0; j < faceletsInt.length; j++) {
            facelets[j] = values[faceletsInt[j]];
        }
        in.readIntArray(colors);
    }

    public static final Creator<CubeState> CREATOR = new Creator<CubeState>() {
        @Override
        public CubeState createFromParcel(Parcel in) {
            return new CubeState(in);
        }

        @Override
        public CubeState[] newArray(int size) {
            return new CubeState[size];
        }
    };

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel parcel, int i) {
        int[] faceletsInt = new int[54];
        for (int j = 0; j < faceletsInt.length; j++) {
            faceletsInt[j] = facelets[j].ordinal();
        }
        parcel.writeIntArray(faceletsInt);
        parcel.writeIntArray(colors);
    }
}
