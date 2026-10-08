CONTAINER Tmeshcheck
{
    NAME Tmeshcheck;
    INCLUDE Tbase;

    GROUP ID_TAGPROPERTIES
    {
        DEFAULT 1;

        BOOL MESHCHECK_ENABLED { }

        SEPARATOR { LINE; }

        REAL MESHCHECK_EDGE_WIDTH
        {
            MIN 1.0;
            MAX 10.0;
            MINSLIDER 1.0;
            MAXSLIDER 10.0;
            STEP 0.5;
            CUSTOMGUI REALSLIDER;
        }

        BOOL MESHCHECK_DEPTH_TEST { }

        SEPARATOR { LINE; }

        BOOL MESHCHECK_SHOW_ANGLE { }

        REAL MESHCHECK_ANGLE_THRESHOLD
        {
            UNIT DEGREE;
            MIN 0.0;
            MAX 180.0;
            MINSLIDER 0.0;
            MAXSLIDER 180.0;
            STEP 1.0;
            CUSTOMGUI REALSLIDER;
        }

        BOOL MESHCHECK_ANGLE_IGNORE_HARD { }

        BOOL MESHCHECK_USE_GRADIENT { }
        COLOR MESHCHECK_EDGE_COLOR { }
        COLOR MESHCHECK_COLOR_MIN { }
        COLOR MESHCHECK_COLOR_MAX { }

        SEPARATOR { LINE; }

        BOOL MESHCHECK_SHOW_HARD_EDGES { }
        COLOR MESHCHECK_HARD_EDGE_COLOR { }

        SEPARATOR { LINE; }

        BOOL MESHCHECK_SHOW_UV_SEAMS { }
        COLOR MESHCHECK_UV_SEAM_COLOR { }

        SEPARATOR { LINE; }

        BOOL MESHCHECK_SHOW_HARD_SEAM_DIFF { }
        COLOR MESHCHECK_HARD_SEAM_COLOR { }

        SEPARATOR { LINE; }

        BOOL MESHCHECK_SHOW_BOUNDARY { }
        COLOR MESHCHECK_BOUNDARY_COLOR { }

        SEPARATOR { LINE; }

        STRING MESHCHECK_INFO_COUNT { ANIM OFF; }
    }
}
