#ifndef IDAM_PLUGINS_LIVEDISPLAY_LIVEDISPLAY_H
#define IDAM_PLUGINS_LIVEDISPLAY_LIVEDISPLAY_H

#include <server/pluginStructs.h>
#include <structures/genStructs.h>


#ifdef __cplusplus
extern "C" {
#endif

#define THISPLUGIN_VERSION                  1
#define THISPLUGIN_MAX_INTERFACE_VERSION    1
#define THISPLUGIN_DEFAULT_METHOD        "get"

#define STRING char

typedef struct {
    STRING* identifier;
    STRING* name;
    int position_count;         // Number of Coordinates
    double* r;                  // Radial Position
    //double *r_error_upper;
    //double *r_error_lower;
    //double *r_error_index;
    double* z;                  // Z Position
    //double *z_error_upper;
    //double *z_error_lower;
    //double *z_error_index;
    double* phi;                // Angle
    //double *phi_error_upper;
    //double *phi_error_lower;
    //double *phi_error_index;
    int data_count;             // Number of data
    double* data;               // Measurement data
    //double *data_error_upper;
    //double *data_error_lower;
    //double *data_error_index;
    double* time;               // Measurement time
} FLUX_LOOP;

typedef struct {
    STRING* identifier;
    STRING* name;
    double r;                // Radial Position
    //double r_error_upper;
    //double r_error_lower;
    //double r_error_index;
    double z;                // Z Position
    //double z_error_upper;
    //double z_error_lower;
    //double z_error_index;
    double phi;                // Angle
    //double phi_error_upper;
    //double phi_error_lower;
    //double phi_error_index;
    //double poloidal_angle;
    //double poloidal_angle_error_upper;
    //double poloidal_angle_error_lower;
    //double poloidal_angle_error_index;
    //double toroidal_angle;
    //double toroidal_angle_error_upper;
    //double toroidal_angle_error_lower;
    //double toroidal_angle_error_index;
    //double area;
    //double area_error_upper;
    //double area_error_lower;
    //double area_error_index;
    //double length;
    //double length_error_upper;
    //double length_error_lower;
    //double length_error_index;
    //int turns;
    //int count;
    int data_count;            // Number of data Measurements
    double* data;                // Measurement data
    //double *data_error_upper;
    //double *data_error_lower;
    //double *data_error_index;
    double* time;                // Measurement time
} BPOL_PROBE;

typedef struct {
    STRING* identifier;
    STRING* name;
    int count;                // Number of data
    double* data;                // Measurement data
    double* time;                // Measurement time
    //double *data_error_upper;
    //double *data_error_lower;
    //double *data_error_index;
} METHOD_DATA;

typedef struct {
    STRING* name;
    METHOD_DATA* ip;
    METHOD_DATA* diamagnetic_flux;
} METHOD;

typedef struct {
    STRING* name;
    STRING* version;
    STRING* parameters;
    int output_flag_count;
    int* output_flag;
} CODE;

typedef struct {
    //STRING * comment;
    //int homogeneous_time;
    int flux_loop_count;
    FLUX_LOOP* flux_loop;        // Array of Flux Loops
    int bpol_probe_count;
    BPOL_PROBE* bpol_probe;        // Array of Magnetic Probes
    int method_count;
    METHOD* method;            // Array of measurement methods and values
    //CODE code;
    //METHOD_DATA ip;			// Plasma Current
    //METHOD_DATA diamagnetic_flux;	// diamagnetic_flux

} MAGNETICS_PROXY;            // Proxy for the Magnetics IDS

typedef struct {
    STRING* comment;
    int homogeneous_time;
    int flux_loop_count;
    int bpol_probe_count;
    int method_count;
    CODE code;
} MAGNETICS_TEST2;            // Proxy for the Magnetics IDS

typedef struct {
    STRING* comment;
    int homogeneous_time;
    int flux_loop_count;
    FLUX_LOOP* flux_loop;        // Array of Flux Loops
    int bpol_probe_count;
    int method_count;
    CODE code;
} MAGNETICS_TEST3;            // Proxy for the Magnetics IDS

typedef struct {
    int data_count;            // Number of data
    double* data;                // Measurement data
    double* time;                // Measurement time
} FLUX_LOOP_TEST1;

typedef struct {
    int data_count;            // Number of data Measurements
    double* data;                // Measurement data
    double* time;                // Measurement time
} BPOL_PROBE_TEST1;

typedef struct {
    STRING* identifier;
    STRING* name;
    double r;                // Radial Position
    double z;                // Z Position
    double phi;                // Angle
} BPOL_PROBE_TEST2;

typedef struct {
    int count;
    double* r;
    double* z;
} STATIC_LIMITER;

typedef struct {
    int data_count;            // Number of data Measurements
    double* r0;                // Major Radius of Measurement (m)
    double* b0;                // Vacuum Toroidal Magnetic Field at the Measurement Major Radius (T)
    double* rb0;                // Product r0 * b0 (Tm)
    double* time;                // Measurement Time
} TF_PROXY;                // Toroidal Magnetic Field IDS Proxy

extern int livedisplay(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

#ifdef __cplusplus
}
#endif

#endif // IDAM_PLUGINS_LIVEDISPLAY_LIVEDISPLAY_H
