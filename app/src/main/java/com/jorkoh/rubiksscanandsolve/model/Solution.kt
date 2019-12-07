package com.jorkoh.rubiksscanandsolve.model

import android.os.Parcelable
import kotlinx.android.parcel.Parcelize

@Parcelize
class Solution(
    val initialState: CubeState,
    val solutionSteps: List<String> = initialState.calculateSolution(),
    val states: List<CubeState> = initialState.calculateStates(solutionSteps)
) : Parcelable