package com.jorkoh.rubiksscanandsolve.solve

import android.os.Bundle
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.Fragment
import androidx.navigation.fragment.navArgs
import com.jorkoh.rubiksscanandsolve.R
import com.jorkoh.rubiksscanandsolve.rubikdetector.model.toSolverScramble
import com.jorkoh.rubiksscanandsolve.rubikdetector.model.toVisualizerState
import com.jorkoh.rubiksscanandsolve.rubiksolver.Search
import kotlinx.android.synthetic.main.fragment_solve.*

class SolveFragment : Fragment() {

    companion object {
        const val ANIM_CUBE_SAVE_STATE_BUNDLE_ID = "animCube"
    }

    private val args: SolveFragmentArgs by navArgs()

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View? {
        return inflater.inflate(R.layout.fragment_solve, container, false)
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        button_previous.setOnClickListener {
            cube.animateMoveReversed()
        }

        button_pause.setOnClickListener {
            cube.stopAnimation()
        }

        button_play.setOnClickListener {
            cube.animateMoveSequence()
        }

        button_next.setOnClickListener {
            cube.animateMove()
        }
    }

    override fun onActivityCreated(savedInstanceState: Bundle?) {
        super.onActivityCreated(savedInstanceState)
        savedInstanceState?.getBundle(ANIM_CUBE_SAVE_STATE_BUNDLE_ID)?.let {
            cube.restoreState(it)
        }

        cube.setCubeModel(args.cubeState.toVisualizerState())
        cube.setCubeColors(args.cubeState.colors)

        //TODO Calculation of the search has to be done in a coroutine
        //TODO Probably should check that the state has a solution before this
        val solution = Search().solution(args.cubeState.toSolverScramble(), 21, 100000000, 0, 0)
        cube.setMoveSequence(solution)
        text_solution.text = solution
    }

    override fun onSaveInstanceState(outState: Bundle) {
        super.onSaveInstanceState(outState)
        outState.putBundle(ANIM_CUBE_SAVE_STATE_BUNDLE_ID, cube.saveState())
    }

    override fun onDestroyView() {
        super.onDestroyView()
        cube.cleanUpResources()
    }
}