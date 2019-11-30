package com.jorkoh.rubiksscanandsolve.rubikdetector.model

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
            null -> ""
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
            null -> ""
        }
    }
    return result
}