package com.jorkoh.rubiksscanandsolve.solve

import android.graphics.Paint
import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.Fragment
import androidx.lifecycle.Lifecycle
import androidx.navigation.fragment.navArgs
import com.google.android.material.chip.Chip
import com.google.android.material.chip.ChipGroup
import com.jorkoh.rubiksscanandsolve.R
import com.jorkoh.rubiksscanandsolve.model.toVisualizerState
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

        button_reset.setOnClickListener {
            cube.resetToInitialState()
        }

        button_previous.setOnClickListener {
            cube.applyMoveReversed()
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

        cube.setOnCubeModelUpdatedListener { _, movePosition ->
            if(!lifecycle.currentState.isAtLeast(Lifecycle.State.CREATED)){
                return@setOnCubeModelUpdatedListener
            }

            val stepsCount = args.solution.solutionSteps.size

            for (i in 0 until stepsCount) {
                val solutionStep = layout_solution_steps.getChildAt(i) as Chip

                solutionStep.isEnabled = i <= movePosition
                solutionStep.paintFlags = if (i == movePosition) {
                    solutionStep.paintFlags or Paint.UNDERLINE_TEXT_FLAG
                } else {
                    solutionStep.paintFlags and Paint.UNDERLINE_TEXT_FLAG.inv()
                }
            }

            when {
                movePosition == 0 -> {
                    button_reset.isEnabled = false
                    button_previous.isEnabled = false
                    button_play.isEnabled = true
                    button_next.isEnabled = true
                }
                movePosition >= stepsCount - 1 -> {
                    button_reset.isEnabled = true
                    button_previous.isEnabled = true
                    button_play.isEnabled = false
                    button_next.isEnabled = false
                }
                else -> {
                    button_reset.isEnabled = true
                    button_previous.isEnabled = true
                    button_play.isEnabled = true
                    button_next.isEnabled = true
                }
            }
        }
    }

    override fun onActivityCreated(savedInstanceState: Bundle?) {
        super.onActivityCreated(savedInstanceState)
        savedInstanceState?.getBundle(ANIM_CUBE_SAVE_STATE_BUNDLE_ID)?.let {
            cube.restoreState(it)
        }

        cube.setCubeModel(args.solution.initialState.toVisualizerState())
        cube.setCubeColors(args.solution.initialState.colors.toIntArray())
        cube.setMoveSequence(args.solution.solutionSteps.joinToString(" "))
        inflateSolutionSteps(layout_solution_steps, args.solution.solutionSteps)
    }

    private fun inflateSolutionSteps(container: ChipGroup, solutionSteps: List<String>) {
        container.removeAllViews()
        solutionSteps.forEachIndexed { index, step ->
            LayoutInflater.from(context).inflate(R.layout.solution_step, container)

            val solutionStep = container.getChildAt(index) as Chip
            solutionStep.text = step
        }
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