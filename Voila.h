/*********************************************************************************/
/*                                                                               */
/*                                    Voila.h                                    */
/*                                                                               */
/*********************************************************************************/

#ifndef VoilaH
#define VoilaH

#include <useful.h>
#include <Trackers.h>

#define CODAS 2
#define LED_SIZE 50.0
#define TRACKER_SCALE_FACTOR 1.0

#ifdef  __cplusplus
extern "C"

   {
#endif

/* Set of markers used to perform the alignment of the coda bars. */
extern int    o_marker, x_marker, y_marker, xy_marker;
extern int    y_marker_direction;

extern int    o_marker_color, x_marker_color, y_marker_color, xy_marker_color;
extern double nominal_o_position[3];
extern double nominal_x_position[3];
extern double nominal_y_position[3];
extern double nominal_xy_position[3];
extern int    first_alignment_marker;
extern int    last_alignment_marker;
extern double  alignment_tolerance;

/* What is the angle of the field-of-view for windows in the alignment program. */
extern double coda_viewpoint_fov;

/*
 * Where should the aligment markers appear in the fov of each CODA
 * if the alignment is good?
 */
extern double sweet_spot_center[3];
extern double standing_sweet_spot[3];
extern double rotating_sweet_spot[3];
extern double supine_sweet_spot[3];


/* How far from the center can the alignment markers be and still be considered good? */
extern double sweet_spot_radius;

/* Set of markers used to test the alignment. */
extern int    check_marker_first, check_marker_last;


/* Which sensor is which. */
extern int    hmd_sensor, wand_sensor, chest_sensor, back_sensor, platform_sensor, wall_sensor;

/* Make it easier to change the labels according to the opnom list. */
extern char  *coda_label[CODAS];

/* This is what will tipically be acquired in a single frame of coda data. */
extern Sample marker_sample;
extern DataSet marker_data_set;

extern int first_marker;
extern int last_marker;
extern double marker_frequency;

/* Put this texture on the wall of the module for realism. */
extern char *module_wall_texture;

/*
 * Nominal position of key objects where zero is the center of the module.
 */
extern double nominal_workspace_center[3];
extern double nominal_srs_position[3];
extern double nominal_waist_height;
extern double nominal_standing_position[3];
extern double nominal_supine_position[3];
extern double nominal_coda_position[CODAS][3];

/* 
* The following values are for the simulator only.
* The CODA locations are actually where the SRS appears to be in
* the intrinisic reference frame of each CODA when each CODA is in
* its nominal orientation.
* Nominal orientation is having the bar aligned vertically on the back
* wall and rotated inward to converge on the SRS.
* Values are in mm.
*/
extern double simulated_coda_location[CODAS][3];
extern double simulated_coda_eulers[CODAS][3];

#ifdef  __cplusplus
}
#endif

#define VoilaH
#endif