package com.jorkoh.rubiksscanandsolve.rubikdetector.model;


import androidx.annotation.IntDef;
import androidx.annotation.NonNull;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

public class RubikFacelet {

    @RubikFacelet.Color
    public int color;

    public Point2d center;

    public float width;

    public float height;

    public float angle;

    public RubikFacelet() {
        this(RubikFacelet.Color.WHITE, new Point2d(0, 0), 0, 0, 0);
    }

    public RubikFacelet(@RubikFacelet.Color int color, @NonNull Point2d center, float width, float height, float angle) {
        this.color = color;
        this.center = center;
        this.width = width;
        this.height = height;
        this.angle = angle;
    }

    public RubikFacelet(@NonNull RubikFacelet rubikFacelet) {
        this.color = rubikFacelet.color;
        this.center = rubikFacelet.center;
        this.width = rubikFacelet.width;
        this.height = rubikFacelet.height;
        this.angle = rubikFacelet.angle;
    }

    @NonNull
    public Point2d[] corners() {
        Point2d[] result = new Point2d[4];
        double angleSinHalf = Math.sin(angle) * 0.5f;
        double angleCosHalf = Math.cos(angle) * 0.5f;

        result[0] = new Point2d(Math.round((float) (center.x - angleSinHalf * height - angleCosHalf * width)),
                Math.round((float) (center.y + angleCosHalf * height - angleSinHalf * width)));

        result[1] = new Point2d(Math.round((float) ((center.x + angleSinHalf * height - angleCosHalf * width))),
                Math.round((float) (center.y - angleCosHalf * height - angleSinHalf * width)));

        result[2] = new Point2d(2 * center.x - result[0].x, 2 * center.y - result[0].y);

        result[3] = new Point2d(2 * center.x - result[1].x, 2 * center.y - result[1].y);
        return result;
    }

    float innerCircleRadius() {
        return Math.min(width, height) / 2;
    }

    @IntDef
    @Retention(RetentionPolicy.SOURCE)
    public @interface Color {
        int RED = 0,
                ORANGE = 1,
                YELLOW = 2,
                GREEN = 3,
                BLUE = 4,
                WHITE = 5;
    }
}
