<h1 align="center"> Rubik’s Scan & Solve </h1>

<p align="center">
  Easily solve your cube a couple of minutes. Built with Kotlin and ❤️
</p>

## Technologies

Some technologies used to develop Rubik’s Scan & Solve:

* [Kotlin](https://kotlinlang.org/)\*, Google’s preferred language for Android app development.
* [NDK](https://developer.android.com/ndk), toolset to bring C and C++ native code to Android.
* [OpenCV](https://opencv.org/), open source computer vision (and more) library.
* [CameraX](https://developer.android.com/training/camerax), Google's new camera API with consistent and easy-to-use surface.
* [LiveData](https://developer.android.com/topic/libraries/architecture/livedata), lifecycle-aware observables.
* [LeakCanary](https://square.github.io/leakcanary/), memory leak detection.
* [ShapeShifter](https://github.com/alexjlockwood/ShapeShifter), Animated Vector Drawable animations.

\* <sub>The app itself is 100% Kotlin code. Some of the modified open source libraries included as local modules are written in Java.</sub>

## Features

* Detect 3 faces at the same time making the scanning process faster and less error-prone.
* Supports any\* color configuration by grouping the facelets through kMeans instead of hardcoded color ranges.
* Continuous high-performance scanning through CameraX image analysis, directly allocated memory buffers and native C++ code.

\* <sub>Results may vary depending on lightning conditions and the condition of the stickers.</sub>

## Video demo

  https://user-images.githubusercontent.com/27740534/133829552-95af6cc4-d60d-4177-bdea-20d8c729e210.mp4
 

## Acknowledgments

Stuff that made this project possible:

* [RubikDetector](https://github.com/cjurjiu/RubikDetector-Android). Original facelet and color detection algorithm which was deeply modified to support scanning of multiple faces at once, improved color detection through kMeans and CIELAB color space as well as integrated into [CameraX image analysis](https://developer.android.com/training/camerax/analyze) system. Created by the awesome [Catalin Jurjiu](https://github.com/cjurjiu).
* [AnimCube](https://github.com/cjurjiu/AnimCubeAndroid). Rendering of a 3d cube used to display the solution. Port by the awesome [Catalin Jurjiu](https://github.com/cjurjiu) of [Josef Jelinek applet](http://software.rubikscube.info/AnimCube/).
* [min2phase](https://github.com/cs0x7f/min2phase). Optimized implementation of Kociemba's two-phase solver algorithm.
