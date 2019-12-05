package com.jorkoh.rubiksscanandsolve.scan.rubikdetector;

import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Path;
import android.media.Image;

import androidx.annotation.ColorInt;
import androidx.annotation.NonNull;

import com.jorkoh.rubiksscanandsolve.scan.rubikdetector.model.Point2d;
import com.jorkoh.rubiksscanandsolve.scan.rubikdetector.model.RubikFacelet;

import java.nio.ByteBuffer;

@SuppressWarnings("unused")
public class RubikDetectorUtils {

    /**
     * Rescales the values & position of the detected facelets to match the new resolution.
     *
     * @param result         3x3 {@link RubikFacelet} array of previously detected facelets
     * @param originalWidth  original image width, for which the facelets were detected
     * @param originalHeight original image height, for which the facelets were detected
     * @param newWidth       new image width
     * @param newHeight      new image height
     * @return a 3x3 array of scaled {@link RubikFacelet} objects that match the new desired image resolution.
     */
    @NonNull
    public static RubikFacelet[][] rescaleResults(@NonNull RubikFacelet[][] result, int originalWidth, int originalHeight, int newWidth, int newHeight) {

        if (originalWidth > originalHeight != newWidth > newHeight) {
            throw new IllegalArgumentException("Largest side cannot differ between original frame size, and new frame size. In original: " +
                    "frameWidth > frameHeight = " + (originalWidth > originalHeight) +
                    ", whereas in requested size: " +
                    "width > height = " + (newWidth > newHeight));
        }

        if (newWidth <= 0 || newHeight <= 0) {
            throw new IllegalArgumentException("New dimensions need to be positive numbers.");
        }

        RubikFacelet[][] rescaledResults = new RubikFacelet[3][3];
        int largestDimension = newWidth > newHeight ? newWidth : newHeight;
        int largestFrameDimension = originalWidth > originalHeight ? originalWidth : originalHeight;
        float ratio = (float) largestDimension / largestFrameDimension;

        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                RubikFacelet facelet = new RubikFacelet(result[i][j]);

                facelet.center.x *= ratio;
                facelet.center.y *= ratio;
                facelet.width *= ratio;
                facelet.height *= ratio;

                rescaledResults[i][j] = facelet;
            }
        }
        return rescaledResults;
    }

    /**
     * Draws the facelets as empty rectangles, on the received canvas, with the specified paint.
     * <p>
     * For each facelet, the paint color is overwritten with the facelet's color.
     * <p>
     * This internally allocates a path.
     *
     * @param facelets a 3x3 array of {@link RubikFacelet}s which need to be drawn
     * @param canvas   the {@link Canvas} in which the drawing will be performed
     * @param paint    the {@link Paint} used to draw the facelets. Its color will be overwritten by each facelet,
     *                 s.t. each facelet is drawn with its correct color.
     */
    public static void drawFaceletsAsRectangles(@NonNull RubikFacelet[][] facelets, @NonNull Canvas canvas, @NonNull Paint paint) {
        Path path = new Path();
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                paint.setColor(getAndroidColor(facelets[i][j]));
                Point2d[] points = facelets[i][j].corners();
                path.reset();

                path.moveTo(points[0].x, points[0].y);
                path.lineTo(points[1].x, points[1].y);

                path.moveTo(points[1].x, points[1].y);
                path.lineTo(points[2].x, points[2].y);

                path.moveTo(points[2].x, points[2].y);
                path.lineTo(points[3].x, points[3].y);

                path.moveTo(points[3].x, points[3].y);
                path.lineTo(points[0].x, points[0].y);

                canvas.drawPath(path, paint);
            }
        }
    }

    /**
     * Draws the facelets as empty or filled circles, on the received canvas, with the specified paint. To draw the facelets
     * as filled circles, specify {@link Paint.Style#FILL_AND_STROKE} or {@link Paint.Style#FILL} as the paint's style.
     * <p>
     * For each facelet, the paint color is overwritten with the facelet's color.
     * <p>
     *
     * @param facelets a 3x3 array of {@link RubikFacelet}s which need to be drawn
     * @param canvas   the {@link Canvas} in which the drawing will be performed
     * @param paint    the {@link Paint} used to draw the facelets. Its color will be overwritten by each facelet,
     *                 s.t. each facelet is drawn with its correct color.
     */
    public static void drawFaceletsAsCircles(@NonNull RubikFacelet[][] facelets, @NonNull Canvas canvas, @NonNull Paint paint) {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                RubikFacelet facelet = facelets[i][j];
                paint.setColor(getAndroidColor(facelet));
                canvas.drawCircle(facelet.center.x, facelet.center.y, Math.min(facelet.width, facelet.height) / 2, paint);
            }
        }
    }

    /**
     * Translates the color of a {@link RubikFacelet} to an Android {@link android.graphics.Color}.
     *
     * @param rubikFacelet the {@link RubikFacelet} whose color needs to be translated.
     * @return a {@link android.graphics.Color} equivalent of the {@link RubikFacelet} color.
     */
    @ColorInt
    public static int getAndroidColor(@NonNull RubikFacelet rubikFacelet) {
        switch (rubikFacelet.color) {
            case RubikFacelet.Color.WHITE:
                return android.graphics.Color.WHITE;
            case RubikFacelet.Color.RED:
                return android.graphics.Color.RED;
            case RubikFacelet.Color.GREEN:
                return android.graphics.Color.GREEN;
            case RubikFacelet.Color.BLUE:
                return android.graphics.Color.BLUE;
            case RubikFacelet.Color.YELLOW:
                return android.graphics.Color.YELLOW;
            case RubikFacelet.Color.ORANGE:
                return android.graphics.Color.argb(255, 255, 127, 0);
            default:
                return android.graphics.Color.WHITE;
        }
    }

    /**
     * Utility method to print the colors of the detected facelets as a formatted string.
     * <p>
     * Useful for printing.
     *
     * @param result 3x3 array of {@link RubikFacelet} objects
     * @return a formatted {@link String} describing the colors of the found facelets.
     */
    @NonNull
    public static String getResultColorsAsString(@NonNull RubikFacelet[][] result) {
        StringBuilder stringBuilder = new StringBuilder("Colors: {");
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                if (j == 0) {
                    stringBuilder.append(" {");
                }
                switch (result[i][j].color) {
                    case RubikFacelet.Color.WHITE:
                        stringBuilder.append("WHITE ");
                        break;
                    case RubikFacelet.Color.YELLOW:
                        stringBuilder.append("YELLOW ");
                        break;
                    case RubikFacelet.Color.RED:
                        stringBuilder.append("RED ");
                        break;
                    case RubikFacelet.Color.BLUE:
                        stringBuilder.append("BLUE ");
                        break;
                    case RubikFacelet.Color.GREEN:
                        stringBuilder.append("GREEN ");
                        break;
                    case RubikFacelet.Color.ORANGE:
                        stringBuilder.append("ORANGE ");
                        break;

                }
                stringBuilder.append(", ");
                stringBuilder.append(result[i][j].angle);
                if (j == 2) {
                    stringBuilder.append("} ");
                }
            }
        }
        stringBuilder.append("}");
        return stringBuilder.toString();
    }

    // https://stackoverflow.com/a/52740776 @Alex-Cohn has many other good answers related
    public static byte[] YUV_420_888toNV21(Image image) {

        int width = image.getWidth();
        int height = image.getHeight();
        int ySize = width*height;
        int uvSize = width*height/4;

        byte[] nv21 = new byte[ySize + uvSize*2];

        ByteBuffer yBuffer = image.getPlanes()[0].getBuffer(); // Y
        ByteBuffer uBuffer = image.getPlanes()[1].getBuffer(); // U
        ByteBuffer vBuffer = image.getPlanes()[2].getBuffer(); // V

        int rowStride = image.getPlanes()[0].getRowStride();
        if ((image.getPlanes()[0].getPixelStride() != 1)) throw new AssertionError();

        int pos = 0;

        if (rowStride == width) { // likely
            yBuffer.get(nv21, 0, ySize);
            pos += ySize;
        }
        else {
            long yBufferPos = width - rowStride; // not an actual position
            for (; pos<ySize; pos+=width) {
                yBufferPos += rowStride - width;
                yBuffer.position((int) yBufferPos);
                yBuffer.get(nv21, pos, width);
            }
        }

        rowStride = image.getPlanes()[2].getRowStride();
        int pixelStride = image.getPlanes()[2].getPixelStride();

        if ((rowStride != image.getPlanes()[1].getRowStride())) throw new AssertionError();
        if ((pixelStride != image.getPlanes()[1].getPixelStride())) throw new AssertionError();

        if (pixelStride == 2 && rowStride == width && uBuffer.get(0) == vBuffer.get(1)) {
            // maybe V an U planes overlap as per NV21, which means vBuffer[1] is alias of uBuffer[0]
            byte savePixel = vBuffer.get(1);
            vBuffer.put(1, (byte)0);
            if (uBuffer.get(0) == 0) {
                vBuffer.put(1, (byte)255);
                if (uBuffer.get(0) == 255) {
                    vBuffer.put(1, savePixel);
                    vBuffer.get(nv21, ySize, uvSize);

                    return nv21; // shortcut
                }
            }

            // unfortunately, the check failed. We must save U and V pixel by pixel
            vBuffer.put(1, savePixel);
        }

        // other optimizations could check if (pixelStride == 1) or (pixelStride == 2),
        // but performance gain would be less significant

        for (int row=0; row<height/2; row++) {
            for (int col=0; col<width/2; col++) {
                int vuPos = col*pixelStride + row*rowStride;
                nv21[pos++] = vBuffer.get(vuPos);
                nv21[pos++] = uBuffer.get(vuPos);
            }
        }

        return nv21;
    }
}
