package com.jorkoh.rubiksscanandsolve.solve

import android.os.Bundle
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.Fragment
import androidx.navigation.fragment.navArgs
import com.jorkoh.rubiksscanandsolve.R
import kotlinx.android.synthetic.main.fragment_solve.*

class SolveFragment : Fragment() {

    companion object {
        const val ANIM_CUBE_SAVE_STATE_BUNDLE_ID = "animCube"
    }

    val args: SolveFragmentArgs by navArgs()

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View? {
        return inflater.inflate(R.layout.fragment_solve, container, false)
    }

    override fun onActivityCreated(savedInstanceState: Bundle?) {
        super.onActivityCreated(savedInstanceState)
        savedInstanceState?.getBundle(ANIM_CUBE_SAVE_STATE_BUNDLE_ID)?.let {
            cube.restoreState(it)
        }

        Log.d("TESTING", "Initial state: ${args.initialState}")
        Log.d("TESTING", "Solution: ${args.solution}")
        cube.setCubeModel(args.initialState)
        cube.setMoveSequence(args.solution)
        cube.animateMoveSequence()
    }

    override fun onSaveInstanceState(outState: Bundle) {
        super.onSaveInstanceState(outState)
        outState.putBundle(ANIM_CUBE_SAVE_STATE_BUNDLE_ID, cube.saveState())
    }
}