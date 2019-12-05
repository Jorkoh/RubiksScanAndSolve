package com.jorkoh.rubiksscanandsolve.model

import android.os.Parcelable
import android.util.Log
import com.jorkoh.rubiksscanandsolve.solve.rubiksolver.Search
import kotlinx.android.parcel.Parcelize

@Parcelize
data class CubeState(val facelets: List<Face>, val colors: List<Int>) : Parcelable {
    enum class Face {
        UP, FRONT, RIGHT, DOWN, LEFT, BACK
    }
}

fun CubeState.toSolverScramble(): String {

    val solverScrambleOrder = arrayOf(
        0, 1, 2, 3, 4, 5, 6, 7, 8,
        18, 19, 20, 21, 22, 23, 24, 25, 26,
        9, 10, 11, 12, 13, 14, 15, 16, 17,
        33, 30, 27, 34, 31, 28, 35, 32, 29,
        44, 43, 42, 41, 40, 39, 38, 37, 36,
        53, 52, 51, 50, 49, 48, 47, 46, 45
    )

    var result = ""
    for (order in solverScrambleOrder) {
        result += when (facelets[order]) {
            CubeState.Face.UP -> "U"
            CubeState.Face.FRONT -> "F"
            CubeState.Face.RIGHT -> "R"
            CubeState.Face.DOWN -> "D"
            CubeState.Face.LEFT -> "L"
            CubeState.Face.BACK -> "B"
        }
    }
    return result
}

fun CubeState.toVisualizerState(): String {
    val visualizerOrder = arrayOf(
        6, 7, 8, 3, 4, 5, 0, 1, 2,
        33, 34, 35, 30, 31, 32, 27, 28, 29,
        9, 12, 15, 10, 13, 16, 11, 14, 17,
        53, 50, 47, 52, 49, 46, 51, 48, 45,
        42, 43, 44, 39, 40, 41, 36, 37, 38,
        18, 21, 24, 19, 22, 25, 20, 23, 26
    )

    var result = ""
    for (order in visualizerOrder) {
        result += when (facelets[order]) {
            CubeState.Face.UP -> "0"
            CubeState.Face.FRONT -> "1"
            CubeState.Face.RIGHT -> "2"
            CubeState.Face.DOWN -> "3"
            CubeState.Face.LEFT -> "4"
            CubeState.Face.BACK -> "5"
        }
    }
    return result
}


// faces start at:
// U 0 W
// F 9 G
// R 18 R
// D 27 Y
// L 36 O
// B 45 B
fun CubeState.performMove(move: String): CubeState {
    val newFacelets = facelets.toMutableList()
    return when (move) {
        "U" -> {
            newFacelets[9] = facelets[44]
            newFacelets[10] = facelets[43]
            newFacelets[11] = facelets[42]

            newFacelets[18] = facelets[9]
            newFacelets[19] = facelets[10]
            newFacelets[20] = facelets[11]

            newFacelets[53] = facelets[18]
            newFacelets[52] = facelets[19]
            newFacelets[51] = facelets[20]

            newFacelets[44] = facelets[53]
            newFacelets[43] = facelets[52]
            newFacelets[42] = facelets[51]

            CubeState(newFacelets.toList(), colors)
        }
        "F" -> {
            newFacelets[6] = facelets[36]
            newFacelets[7] = facelets[39]
            newFacelets[8] = facelets[42]

            newFacelets[18] = facelets[6]
            newFacelets[21] = facelets[7]
            newFacelets[24] = facelets[8]

            newFacelets[27] = facelets[18]
            newFacelets[30] = facelets[21]
            newFacelets[33] = facelets[24]

            newFacelets[36] = facelets[27]
            newFacelets[39] = facelets[30]
            newFacelets[42] = facelets[33]

            CubeState(newFacelets.toList(), colors)
        }
        "R" -> {
            newFacelets[2] = facelets[11]
            newFacelets[5] = facelets[14]
            newFacelets[8] = facelets[17]

            newFacelets[47] = facelets[2]
            newFacelets[50] = facelets[5]
            newFacelets[53] = facelets[8]

            newFacelets[27] = facelets[47]
            newFacelets[28] = facelets[50]
            newFacelets[29] = facelets[53]

            newFacelets[11] = facelets[27]
            newFacelets[14] = facelets[28]
            newFacelets[17] = facelets[29]

            CubeState(newFacelets.toList(), colors)
        }
        "L" -> {
            newFacelets[0] = facelets[45]
            newFacelets[3] = facelets[48]
            newFacelets[4] = facelets[51]

            newFacelets[9] = facelets[0]
            newFacelets[12] = facelets[3]
            newFacelets[15] = facelets[4]

            newFacelets[33] = facelets[9]
            newFacelets[34] = facelets[12]
            newFacelets[35] = facelets[15]

            newFacelets[45] = facelets[33]
            newFacelets[48] = facelets[34]
            newFacelets[51] = facelets[35]

            CubeState(newFacelets.toList(), colors)
        }
        "B" -> {
            newFacelets[0] = facelets[20]
            newFacelets[1] = facelets[23]
            newFacelets[2] = facelets[26]

            newFacelets[38] = facelets[0]
            newFacelets[41] = facelets[1]
            newFacelets[44] = facelets[2]

            newFacelets[29] = facelets[38]
            newFacelets[32] = facelets[41]
            newFacelets[35] = facelets[44]

            newFacelets[20] = facelets[29]
            newFacelets[23] = facelets[32]
            newFacelets[26] = facelets[35]

            CubeState(newFacelets.toList(), colors)
        }
        "D" -> {
            newFacelets[15] = facelets[38]
            newFacelets[16] = facelets[37]
            newFacelets[17] = facelets[36]

            newFacelets[24] = facelets[15]
            newFacelets[25] = facelets[16]
            newFacelets[26] = facelets[17]

            newFacelets[47] = facelets[24]
            newFacelets[46] = facelets[25]
            newFacelets[45] = facelets[26]

            newFacelets[38] = facelets[47]
            newFacelets[37] = facelets[46]
            newFacelets[36] = facelets[45]

            CubeState(newFacelets.toList(), colors)
        }
        else -> {
            if (move[1] == '\'') {
                // Backwards move is the same as repeating the move three times
                performMove(move[0] + "2").performMove(move[0].toString())
            } else {
                // Double move is the same as repeating the move two times
                performMove(move[0].toString()).performMove(move[0].toString())
            }
        }
    }
}

fun CubeState.calculateSolution(): List<String> {
    return Search().solution(
        toSolverScramble(),
        21,
        100000000,
        0,
        0
    ).split(' ').filter { it.isNotBlank() }
}

fun CubeState.calculateStates(steps: List<String>): List<CubeState> {
    Log.d("TESTING", "Calculating states for solution ${steps.joinToString(" ")}")
    var previousState = this
    return steps.map { move ->
        Log.d("TESTING", "Executing move $move")
        previousState = previousState.performMove(move)
        previousState
    }
}