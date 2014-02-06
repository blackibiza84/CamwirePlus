#ifndef CAMERA_H
#define CAMERA_H
/******************************************************************************

    Copyright (c) Industrial Research Limited 2004-2011

    This file is part of Camwire, a generic camera interface.

    Camwire is free software; you can redistribute it and/or modify it
    under the terms of the GNU Lesser General Public License as
    published by the Free Software Foundation; either version 2.1 of the
    License, or (at your option) any later version.

    Camwire is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Camwire; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
    USA


    Title: Header for Camera.cpp

    Description:
    This module is about using a single named camera through its
    handle. The handle should be all a user need know about for complete
    access to all camera functions.  Finding cameras and assigning
    handles to them is done in the Camwire bus module.

CamwirePlus: Michele Adduci <adducimi@informatik.hu-berlin.de>
******************************************************************************/

#include <CameraData.h>

namespace cw
{
    class Camera
    {

        public:
            Camera();
            ~Camera();

            init(const CameraHandle c_handle);


        private:
            CameraID camID;
            CameraUserData camData;
            Camera(const Camera &cam);
            Camera& operator=(const Camera &cam);

    };

}



int camwire_create(const Camwire_handle c_handle);
/* Sets the camera to default initialization settings and connects it to
   the bus.  This function is equivalent to camwire_get_state()
   followed by camwire_create_from_struct().  The handle c_handle is
   obtained from camwire_bus_create().  Returns CAMERA_SUCCESS on
   success or CAMERA_FAILURE on failure or if the camera had previously
   been created.  The function camwire_destroy() must be called when
   done to free the allocated memory.  */

int camwire_create_from_struct(const Camwire_handle c_handle,
			       const Camwire_state *set);
/* Sets the camera to the given initialization settings and connects it
   to the bus.  The Camwire_state structure is returned unchanged.  The
   handle c_handle is obtained from camwire_bus_create().  Returns
   CAMERA_SUCCESS on success or CAMERA_FAILURE on failure or if the
   camera had previously been created.  The function camwire_destroy()
   must be called when done to free the allocated memory.  */

void camwire_destroy(const Camwire_handle c_handle);
/* Disconnects the camera from the bus and frees memory allocated in
   camwire_create() or camwire_create_from_struct().  All camera
   settings are lost.  Calling this function with a null handle has no
   effect. */

int camwire_get_state(const Camwire_handle c_handle, Camwire_state *set);
/* Gets the camera's current settings (running/stopped, trigger source,
   frame rate, frame size, etc).  If the camera has not been created,
   the camera is physically reset to factory default settings and those
   are probed and returned (without creating the camera).  If the camera
   has been created and the state shadow flag is set, the shadow state
   settings are returned, else the camera is queried for current
   settings.  If the camera does not support some settings their
   returned values are undefined.  Returns CAMERA_SUCCESS on success or
   CAMERA_FAILURE on failure.*/

int camwire_set_state(const Camwire_handle c_handle, const Camwire_state *set);
/* Sets the camera to the given settings (running/stopped, trigger
   source, frame rate, frame size, etc).  The camera must already have
   been created.  If the camera does not support some settings, they are
   ignored.  Returns CAMERA_SUCCESS on success or CAMERA_FAILURE on
   failure.*/

int camwire_read_state_from_file(FILE *infile, Camwire_state *set);
/* Reads camera initialization settings from the given infile, changing
   only the members of set that are present in the file.  The remaining
   members are left as they were, and are assumed to contain previous
   settings initialized before this function call.  The format of the
   infile is one line per setting, where each line contains a tag and
   value(s) separated by whitespace.  The tags are chosen from the names
   of the members of Camwire_state and may be abbreviated as long as no
   ambiguity results.  The value(s) is (are) the setting as understood
   by the corresponding camwire_get/set_...() functions.  Most settings
   have a single value, and some (like white balance) have two or more.
   Empty lines and lines starting with the '#' character in the file are
   ignored, as are unrecognized tags.  The function
   camwire_write_state_to_file() can provide an example of the file
   format.  If there are no errors, infile is at end-of-file on return.
   Physical cameras are unchanged (there is no camera handle in the
   argument list).  This function can be called at any time.  Returns
   CAMERA_SUCCESS on success or CAMERA_FAILURE on failure. */

int camwire_write_state_to_file(FILE *outfile, const Camwire_state *set);
/* Writes camera initialization settings to the given outfile, in the
   format understood by camwire_read_state_from_file().  The file can
   subsequently be edited to change settings, or to remove lines of
   settings that one wants to leave unchanged with
   camwire_read_state_from_file().  outfile is at end-of-file on return.
   This function can be called at any time.  Returns CAMERA_SUCCESS on
   success or CAMERA_FAILURE on failure.*/
    
int camwire_get_config(const Camwire_handle c_handle, Camwire_conf *cfg);
/* Gets the camera and its bus's static configuration settings for
   initialization from a configuration file.  They are bus-specific
   hardware parameters that the casual user need not know or care about.
   The structure Camwire_conf is defined above.  If a configuration file
   does not exist, an error message is printed which includes a
   best-guess default configuration.  This can be copied into a new
   configuration file (and edited as needed).  The filename must be
   identical to one of the camera's ID strings (as may be obtained from
   camwire_get_identifier()) appended by an extension of ".conf".  The
   function checks for existing filenames containing the unique chip
   string, or the model name string, or the vendor name string, in this
   order.  It first looks for the three filenames in the current working
   directory and after that in a directory given by the CAMERA_CONF
   environment variable.  The configuration file is cached the first
   time it is read by the current camera with handle c_handle.  There
   should be no need to call this function from a user program for
   normal camera operations.  It is provided here in case it is needed
   for debugging.  Returns CAMERA_SUCCESS on success or CAMERA_FAILURE
   on failure.*/

int camwire_write_config_to_file(FILE *outfile, const Camwire_conf *cfg);
/* Writes the static configuration settings as obtained from
   camwire_get_config() to the given file.  The print format is the same
   as that expected by camwire_get_config() when it reads configuration
   files.  Returns CAMERA_SUCCESS on success or CAMERA_FAILURE on
   failure.*/

int camwire_get_identifier(const Camwire_handle c_handle, Camwire_id *identifier);
/* Fills in the given camwire identifier structure (type defined above).
   The identifier is uniquely and permanently associated with the camera
   hardware, such as might be obtained from configuration ROM data.
   Returns CAMERA_SUCCESS on success or CAMERA_FAILURE on failure. */

int camwire_get_stateshadow(const Camwire_handle c_handle, int *shadow);
/* Gets the state shadow flag: 1 to get camera settings from an internal
   shadow structure or 0 to read them directly from the camera hardware.
   Returns CAMERA_SUCCESS on success or CAMERA_FAILURE on failure.*/

int camwire_set_stateshadow(const Camwire_handle c_handle, const int shadow);
/* Sets the state shadow flag: 1 to get camera settings from an internal
   shadow structure or 0 to read them directly from the camera hardware.
   Setting state shadowing may result in faster camera responses and may
   increase robustness because the camera firmware is not interrupted by
   unnecessary status requests.  State shadowing may however be slightly
   risky because bugs in Camwire or in the camera firmware could cause
   the shadow state to differ from the actual state.  The state shadow
   flag of a created camera can be modified at any time because the
   shadow state is internally maintained irrespective of the state
   shadow flag status.  Returns CAMERA_SUCCESS on success or
   CAMERA_FAILURE on failure.*/

int camwire_get_num_framebuffers(const Camwire_handle c_handle,
				 int *num_frame_buffers);
/* Gets the number of frame reception buffers allocated for the camera
   bus in the host computer.  Returns CAMERA_SUCCESS on success or
   CAMERA_FAILURE on failure. */

int camwire_set_num_framebuffers(const Camwire_handle c_handle,
				 const int num_frame_buffers);
/* Sets the number of frame reception buffers allocated for the camera
   bus in the host computer.  Returns CAMERA_SUCCESS on success or
   CAMERA_FAILURE on failure. */

int camwire_get_framebuffer_lag(const Camwire_handle c_handle, int *buffer_lag);
/* Warning: The use of this function is deprecated because it creates
   the impression that it returns a currently fresh measure of the
   buffer lag.  Rather use the buffer_lag returned by calls to
   camwire_copy_next_frame(), camwire_point_next_frame(),
   camwire_point_next_frame_poll(), and calls to
   camwire_flush_framebuffers() with argument num_to_flush >= 1.
   Ideally it should get the number of bus frame buffers which have been
   filled by the camera but not yet accessed or, in other words, the
   number of frames by which we are behind.  In the current
   implementation this number is only updated by the calls listed above
   and should otherwise be considered stale.  Returns CAMERA_SUCCESS on
   success or CAMERA_FAILURE on failure. */

int camwire_flush_framebuffers(const Camwire_handle c_handle, const int num_to_flush,
			       int *num_flushed, int *buffer_lag);
/* Tries to flush num_to_flush buffered frames.  If num_flushed is not
   the null pointer, the actual number of frames flushed is returned in
   *num_flashed.  *num_flushed will be less than num_to_flush if there
   were fewer buffered frames available to flush than requested.
   *num_flushed will never be more than num_to_flush.  It is safe to
   make num_to_flush a larger number than the total number of buffered
   frames.  If buffer_lag is not the null pointer, the number of bus
   frame buffers by which we are behind after flushing is returned in
   *buffer_lag.  This is the same number as can be obtained from
   camwire_get_framebuffer_lag().  *buffer_lag is only valid if
   num_to_flush was 1 or more.  The historically older frames are
   flushed first.  Frames currently being transmitted by the camera are
   not affected.  Any frames accessed by camwire_point_next_frame() or
   camwire_point_next_frame_poll() should first be released with
   camwire_unpoint_frame() before flushing.  If you want to be sure of
   completely emptying all bus frame buffers, you should stop the
   camera, wait for more than one frame transmission period and then
   flush.  Returns CAMERA_SUCCESS on success or CAMERA_FAILURE on
   failure. */

int camwire_get_frame_offset(const Camwire_handle c_handle, int *left, int *top);
/* Gets the frame offsets (left, top) in units of pixels.  Returns
   CAMERA_SUCCESS on success or CAMERA_FAILURE on failure. */

int camwire_set_frame_offset(const Camwire_handle c_handle, const int left,
			     const int top);
/* Sets the frame offsets (left, top) in units of pixels to the nearest
   values valid for the camera.  The resulting offsets are constrained
   by the maximum available frame dimensions and the current frame
   dimensions.  The offsets can be checked afterwards with
   camwire_get_frame_offset().  Returns CAMERA_SUCCESS on success or
   CAMERA_FAILURE on failure. */

int camwire_get_frame_size(const Camwire_handle c_handle, int *width, int *height);
/* Gets the frame size (width, height) in units of pixels.  Returns
   CAMERA_SUCCESS on success or CAMERA_FAILURE on failure. */

int camwire_set_frame_size(const Camwire_handle c_handle, const int width,
			   const int height);
/* Sets the frame size (width, height) in units of pixels to the nearest
   values valid for the camera.  The sizes available may be constrained
   by the maximum available frame size and the current frame offsets.
   The actual size set can be checked afterwards with
   camwire_get_frame_size().  For some camera buses like IEEE 1394, the
   frame rate may also change, especially with small frame sizes.  Check
   afterwards with camwire_get_framerate().  Returns CAMERA_SUCCESS on
   success or CAMERA_FAILURE on failure. */

int camwire_get_pixel_coding(const Camwire_handle c_handle, Camwire_pixel *coding);
/* Gets the pixel coding, as one of the Camwire_pixel enumeration
   members above.  Returns CAMERA_SUCCESS on success or CAMERA_FAILURE
   on failure. */

int camwire_set_pixel_coding(const Camwire_handle c_handle,
			     const Camwire_pixel coding);
/* Sets the pixel coding, given one of the Camwire_pixel enumeration
   members above.  For some camera buses like IEEE 1394, the frame rate
   may also change, especially with small frame sizes.  Check afterwards
   with camwire_get_framerate().  Returns CAMERA_SUCCESS on success or
   CAMERA_FAILURE on failure or if gamma is switched on and the new
   coding does not support gamma correction. */

int camwire_get_pixel_tiling(const Camwire_handle c_handle, Camwire_tiling *tiling);
/* Gets the pixel tiling, as one of the Camwire_tiling enumeration
   members above.  The tiling is not a typical camera setting because it
   is not settable and hence there is no camwire_set_pixel_tiling()
   function.  Returns CAMERA_SUCCESS on success or CAMERA_FAILURE on
   failure. */

int camwire_pixel_depth(const Camwire_pixel coding, int *depth);
/* Translates the given Camwire pixel colour coding into the
   corresponding pixel depth in bits per pixel.  Returns CAMERA_SUCCESS
   on success or CAMERA_FAILURE on failure. */

int camwire_get_framerate(const Camwire_handle c_handle, double *framerate);
/* Gets the video frame rate in frames per second.  See the description
   of the meaning of 'frame rate' under camwire_set_framerate().
   Returns CAMERA_SUCCESS on success or CAMERA_FAILURE on failure. */

int camwire_set_framerate(const Camwire_handle c_handle, const double framerate);
/* Sets the video frame rate in frames per second to the nearest value
   valid for the camera.  The frame rates available may be constrained
   by the frame dimensions.  The actual frame rate set can be checked
   afterwards with camwire_get_framerate().  The meaning of 'frame rate'
   changes depending on the single-shot and trigger source settings.
   The frame rate determines how long frame transmission from camera to
   computer takes and how much bus bandwidth is used.  With internal
   trigger, non-single-shot (continuous), it is indeed the number of
   frames per second.  With external trigger, non-single-shot, it should
   be set faster than the external trigger frequency to avoid dropped
   frames.  With single-shot, it should be set faster than the frequency
   of single-shot requests.  Returns CAMERA_SUCCESS on success or
   CAMERA_FAILURE on failure. */

int camwire_get_shutter(const Camwire_handle c_handle, double *shutter);
/* Gets the camera's shutter speed (exposure time in seconds).  Returns
   CAMERA_SUCCESS on success or CAMERA_FAILURE on failure. */

int camwire_set_shutter(const Camwire_handle c_handle, const double shutter);
/* Sets the given shutter speed (exposure time in seconds) to the
   nearest valid value for the camera.  The actual value can be checked
   afterwards with camwire_get_shutter().  Returns CAMERA_SUCCESS on
   success or CAMERA_FAILURE on failure. */

int camwire_get_trigger_source(const Camwire_handle c_handle, int *external);
/* Gets the camera's trigger source setting: 1 for external or 0 for
   internal.  Returns CAMERA_SUCCESS on success or CAMERA_FAILURE on
   failure. */

int camwire_set_trigger_source(const Camwire_handle c_handle, const int external);
/* Sets the camera's trigger source: 1 for external or 0 for internal.
   Fails if the camera does not have an external trigger.  Returns
   CAMERA_SUCCESS on success or CAMERA_FAILURE on failure. */

int camwire_get_trigger_polarity(const Camwire_handle c_handle, int *rising);
/* Gets the camera's trigger polarity setting: 1 for rising edge (active
   high) or 0 for falling edge (active low).  Returns CAMERA_SUCCESS on
   success or CAMERA_FAILURE on failure. */

int camwire_set_trigger_polarity(const Camwire_handle c_handle, const int rising);
/* Sets the camera's trigger polarity: 1 for rising edge (active high)
   or 0 for falling edge (active low).  Fails if the trigger polarity is
   not settable.  Returns CAMERA_SUCCESS on success or CAMERA_FAILURE
   on failure. */

int camwire_get_gain(const Camwire_handle c_handle, double *gain);
/* Gets the relative gain factor (response slope) of the camera, as a
   number between 0.0 (minimum) and 1.0 (maximum).  The user has to
   interpret the meaning of this relative gain in terms of the
   individual camera's specification of its minimum and maximum settable
   slope.  For example, if the camera specifies the minimum gain as a
   slope of 1.0 (0dB) and the maximum as 4.0 (12dB), then the actual
   gain is (4.0 - 1.0)*gain + 1.0.  Returns CAMERA_SUCCESS on success
   or CAMERA_FAILURE on failure. */

int camwire_set_gain(const Camwire_handle c_handle, const double gain);
/* Sets the gain factor (response slope) to the nearest value valid for
   the camera.  The gain can be checked afterwards with
   camwire_get_gain().  Gain values can range between the relative
   values of 0.0 (minimum) and 1.0 (maximum).  The user has to normalize
   the actual wanted gain to this range before calling
   camwire_set_gain().  For example, if the camera specifies the minimum
   gain as a slope of 1.0 (0dB) and the maximum as 4.0 (12dB), then the
   relative gain to set is (actual_gain - 1.0)/(4.0 - 1.0).  Fails if
   the gain is not settable.  Returns CAMERA_SUCCESS on success or
   CAMERA_FAILURE on failure. */

int camwire_get_brightness(const Camwire_handle c_handle, double *brightness);
/* Gets the relative brightness (black level) of the camera, as a number
   between -1.0 (minimum) and +1.0 (maximum).  The user has to interpret
   the meaning of this relative brightness in terms of the individual
   camera's specification of its minimum and maximum settable
   brightness.  For example, if the camera specifies the minimum
   brightness level as 0 and the maximum as 1023, then the actual
   brightness level is (1023 - 0)*(brightness + 1.0)/2.0.  Returns
   CAMERA_SUCCESS on success or CAMERA_FAILURE on failure. */

int camwire_set_brightness(const Camwire_handle c_handle,
			   const double brightness);
/* Sets the brightness (black level) to the nearest value valid for the
   camera.  The brightness can be checked afterwards with
   camwire_get_brightness().  Brightness values can range between the
   relative values of -1.0 (minimum) and +1.0 (maximum).  The user has
   to normalize the actual wanted brightness to this range before
   calling camwire_set_brightness().  For example, if the camera
   specifies the minimum brightness level as 0 and the maximum as 1023,
   then the relative brightness to set is 2.0*actual_brightness/(1023 -
   0) - 1.0.  Fails if the brightness is not settable.  Returns
   CAMERA_SUCCESS on success or CAMERA_FAILURE on failure. */

int camwire_get_white_balance(const Camwire_handle c_handle, double bal[2]);
/* Gets the white balance levelss (blue or U, red or V) for colour
   cameras.  bal contains relative numbers between 0.0 and 1.0,
   corresponding respectively to the minimum and maximum levels
   implemented by the camera.  Returns CAMERA_SUCCESS on success or
   CAMERA_FAILURE on failure. */

int camwire_set_white_balance(const Camwire_handle c_handle, const double bal[2]);
/* Sets the white balance levels (blue or U, red or V) for colour
   cameras to the nearest values valid for the camera.  bal containts
   numbers between 0.0 (minimum level) and 1.0 (maximum level).  The
   levels can be checked afterwards with camwire_get_white_balance().
   Fails if the white balance is not settable.  Returns CAMERA_SUCCESS
   on success or CAMERA_FAILURE on failure. */

int camwire_get_gamma(const Camwire_handle c_handle, int *gamma_on);
/* Gets the camera's gamma setting in gamma_on: 1 for gamma-corrected or
   0 for linear pixel values or if the camera is not capable of gamma
   correction.  So far Camwire supports gamma in AVT cameras only.
   Returns CAMERA_SUCCESS on success or CAMERA_FAILURE on failure. */

int camwire_set_gamma(const Camwire_handle c_handle, const int gamma_on);
/* Sets the camera's gamma setting in gamma_on: 1 for gamma-corrected or
   0 for linear pixel values.  Camwire assumes that gamma correction
   non-linearly compresses the camera's 10 or 12-bit internal data path
   into 8 bits per pixel component, according to the Rec.709
   specification.  To use gamma, the current pixel coding should
   therefore have 8 bits per component (such as CAMERA_PIXEL_MONO8,
   CAMERA_PIXEL_YUV422 or CAMERA_PIXEL_RGB8, but not
   CAMERA_PIXEL_MONO16 etc.).  So far Camwire supports gamma in AVT
   cameras only.  Returns CAMERA_SUCCESS on success or CAMERA_FAILURE
   on failure or if the camera is not capable of gamma correction or if
   gamma correction is not supported for the current pixel coding. */

int camwire_inv_gamma(const Camwire_handle c_handle, const void *cam_buf,
		      void *lin_buf, const unsigned long max_val);
/* Transforms cam_buf into lin_buf by inverse gamma correction assuming
   the Rec.709 specification.  This should only be used to linearize the
   response of images that were captured while gamma correction was
   enabled with camwire_set_gamma().  It should restore the image
   sensor's 10-bit (or so) linear dynamic range at the expense of some
   loss of resolution.  The size and pixel coding of the image pointed
   to by cam_buf must match the current setings of image size and pixel
   coding.  The current pixel coding should have 8 bits per component
   (see description of camwire_set_gamma()).  The resultant image
   pointed to by lin_buf must have the same image size, and an assumed
   pixel coding with 16 bits per component such as CAMERA_PIXEL_MONO16
   or CAMERA_PIXEL_RGB16.  The image in lin_buf therefore requires
   twice as much allocated memory as the one in cam_buf.  All 16-bit
   camwire images are in network (big-endian) byte order.  The max_val
   argument scales the output to give the wanted saturation value when
   the input value is 255.  Typical values for max_val are 1023, 4095 or
   65535.  Returns CAMERA_SUCCESS on success or CAMERA_FAILURE on
   failure. */

int camwire_get_colour_correction(const Camwire_handle c_handle, int *corr_on);
/* Gets the camera's colour correction setting corr_on, 1 for
   colour-corrected or 0 for no correction or if the camera is not
   capable of colour correction.  So far Camwire supports colour
   correction in AVT cameras only.  Returns CAMERA_SUCCESS on success
   or CAMERA_FAILURE on failure. */

int camwire_set_colour_correction(const Camwire_handle c_handle, const int corr_on);
/* Sets the camera's colour correction setting corr_on, 1 for
   colour-corrected or 0 for no correction.  Colour correction only
   takes effect if the current pixel coding is colour (such as
   CAMERA_PIXEL_YUV422 or CAMERA_PIXEL_RGB8, but not
   CAMERA_PIXEL_MONO8 etc.).  So far Camwire supports colour correction
   in AVT cameras only.  Returns CAMERA_SUCCESS on success or
   CAMERA_FAILURE on failure or if the camera is not capable of colour
   correction. */

int camwire_get_colour_coefficients(const Camwire_handle c_handle, double coef[9]);
/* Gets the camera's colour correction coefficients.  coef is an array
   of 9 colour correction coefficients, see the description of the
   colour_coef member of Camwire_state above.  If the camera is not
   capable of colour correction, coef forms the 3x3 identity matrix.
   Returns CAMERA_SUCCESS on success or CAMERA_FAILURE on failure. */

int camwire_set_colour_coefficients(const Camwire_handle c_handle,
				    const double coef[9]);
/* Sets the camera's colour correction coefficients.  coef is an array
   of 9 colour correction coefficients, see the description of the
   colour_coef member of Camwire_state above.  Some cameras support
   colour correction with fixed coefficients in which case
   camwire_set_colour_correction() above can enable colour correction
   but this function would fail because it cannot change the
   coefficients.  Returns CAMERA_SUCCESS on success or CAMERA_FAILURE
   on failure or if the camera is not capable of setting colour
   coefficients. */

int camwire_get_single_shot(const Camwire_handle c_handle, int *single_shot_on);
/* Gets the camera's acquisition type setting in single_shot_on: 1 for
   single-shot or 0 for continuous or if the camera is not capable of
   single-shot.  Returns CAMERA_SUCCESS on success or CAMERA_FAILURE
   on failure. */

int camwire_set_single_shot(const Camwire_handle c_handle,
			    const int single_shot_on);
/* Sets the camera's acquisition type in single_shot_on: 1 for
   single-shot or 0 for continuous.  To capture a single frame, make
   sure that the camera is stopped, set the acquisition type to
   single-shot, and set the camera running (with camwire_set_run_stop())
   at the moment the frame is needed.  If the camera is already running
   while changing acquisition type from continous to single-shot, then
   the single frame is acquired immediately (pending an external trigger
   if used).  Changing acquisition type from single-shot to continuous
   while the camera is running is an unreliable thing to do, because you
   can't be sure if the single frame acquisition has started and hence
   you don't know whether the camera is stopped or running.  Returns
   CAMERA_SUCCESS on success or CAMERA_FAILURE on failure. */

int camwire_get_run_stop(const Camwire_handle c_handle, int *runsts);
/* Gets the camera run status in runsts: 1 for running or 0 for stopped.
   Returns CAMERA_SUCCESS on success or CAMERA_FAILURE on failure. */

int camwire_set_run_stop(const Camwire_handle c_handle, const int runsts);
/* Sets the camera run status in runsts: 1 for running or 0 for stopped.
   If a stopped camera is set to running while the acquisition type (as
   set by camwire_set_single_shot()) is single-shot, then only one frame
   is transmitted (pending an external trigger if used) after which the
   camera reverts to the stopped state.  Otherwise, frames are
   transmitted from camera to bus frame buffers at the programmed frame
   rate until the camera is stopped.  Returns CAMERA_SUCCESS on success
   or CAMERA_FAILURE on failure. */

int camwire_copy_next_frame(const Camwire_handle c_handle, void *buffer,
			    int *buffer_lag);
/* Waits until a frame has been received and copies it into the given
   buffer.  All 16-bit camwire images are in network (big-endian) byte
   order.  Note that this function may never return to the calling
   program if a frame does not become available, for example if the
   camera is not running or an external trigger does not arrive.  If
   more than one frame are buffered when this function is called then
   the earliest frame is copied.  If buffer_lag is not the null pointer,
   the number of bus frame buffers by which we are behind is returned in
   *buffer_lag.  This is the same number as can be obtained from
   camwire_get_framebuffer_lag().  The frame's number and time stamp are
   available afterwards from camwire_get_framenumber() and
   camwire_get_timestamp().  If speed is important then
   camwire_point_next_frame() or camwire_point_next_frame_poll() might
   be faster.  Returns CAMERA_SUCCESS on success or CAMERA_FAILURE on
   failure. */

int camwire_point_next_frame(const Camwire_handle c_handle, void **buf_ptr,
			     int *buffer_lag);
/* Sets the given buffer pointer buf_ptr to the next received frame
   buffer.  If a frame is ready it returns immediately, otherwise it
   waits until a frame has been received.  All 16-bit camwire images are
   in network (big-endian) byte order.  Note that this function may
   never return to the calling program if a frame does not become
   available, for example if the camera is not running or an external
   trigger does not arrive.  The frame buffer is locked, which prevents
   it from being overwritten by subsequent frames.  The calling program
   may freely read and write the buffer.  If more than one frames are
   buffered when this function is called then buf_ptr points to the
   earliest frame.  If buffer_lag is not the null pointer, the number of
   bus frame buffers by which we are behind is returned in *buffer_lag.
   This is the same number as can be obtained from
   camwire_get_framebuffer_lag().  The frame's number and time stamp are
   available afterwards from camwire_get_framenumber() and
   camwire_get_timestamp().  This function may be faster than
   camwire_copy_next_frame() because no copying is involved.  The
   downside is that the function camwire_unpoint_frame() must be called
   each time when done to release the frame buffer again.  Returns
   CAMERA_SUCCESS on success or CAMERA_FAILURE on failure. */

int camwire_point_next_frame_poll(const Camwire_handle c_handle, void **buf_ptr,
				  int *buffer_lag);
/* Sets the given buffer pointer buf_ptr to the next received frame
   buffer and returns immediately.  If no frame is ready it sets buf_ptr
   to the null pointer and returns 0 in *buffer_lag.  All 16-bit camwire
   images are in network (big-endian) byte order.  Although it may not
   always return a frame pointer, this function always returns to the
   calling program even if a frame does not become available.  Otherwise
   its behaviour is similar to camwire_point_next_frame().  Note that
   the values returned from camwire_get_framenumber() and
   camwire_get_timestamp() are not valid if no frame was returned.  Like
   camwire_point_next_frame(), the function camwire_unpoint_frame() must
   be called each time a frame is obtained to release the frame buffer
   again.  If no frame was obtained then camwire_unpoint_frame() should
   not be called.  Returns CAMERA_SUCCESS on success or CAMERA_FAILURE
   on failure. */

int camwire_unpoint_frame(const Camwire_handle c_handle);
/* Releases the bus frame buffer that was pointed to with the pointer
   access functions camwire_point_next_frame() or
   camwire_point_next_frame_poll(), so that it can be used again for
   receiving new image data.  Calls to pointer access functions and
   camwire_unpoint_frame() should be strictly interleaved, otherwise the
   next call to a pointer access function will fail.
   camwire_unpoint_frame() itself is however safe to call if no frame is
   locked in which case it has no effect.  Returns CAMERA_SUCCESS on
   success or CAMERA_FAILURE on failure. */

int camwire_get_framenumber(const Camwire_handle c_handle, long *framenumber);
/* Gets the serial number of the last frame captured with
   camwire_copy_next_frame(), camwire_point_next_frame() or
   camwire_point_next_frame_poll().  The number is initialized to zero
   by camwire_create() or camwire_create_from_struct(), and the first
   frame is number 1.  All frames are numbered (as far as possible -
   buffer overflows may cause problems) even if they are not accessed by
   camwire_copy_next_frame(), camwire_point_next_frame() or
   camwire_point_next_frame_poll().  If the camera or its bus are
   configured to drop old frames, the number of missed frames may be
   calculated as one less than the difference between consecutive frame
   numbers.  Returns CAMERA_SUCCESS on success or CAMERA_FAILURE on
   failure. */

int camwire_get_timestamp(const Camwire_handle c_handle, struct timespec *timestamp);
/* Gets the time stamp as a timespec structure (seconds and nanoeconds)
   of the last frame captured with camwire_copy_next_frame(),
   camwire_point_next_frame() or camwire_point_next_frame_poll().  The
   time stamp is the estimated time of the shutter trigger relative to
   the start of the Unix clock (0 January 1970) as reported by the local
   host processor.  Returns CAMERA_SUCCESS on success or
   CAMERA_FAILURE on failure. */

int camwire_debug_print_status(const Camwire_handle c_handle);
/* Debugging tool for developers.  Prints the contents of
   c_handle->userdata to stderr no matter what the state of the camera
   is.  Always returns CAMERA_SUCCESS. */

int camwire_version(char const **version_str);
/* Sets the given pointer to Camwire's version as a const string such as
   "1.2.3", or NULL if an internal error caused the string to overflow
   (should never happen).  This function can be called at any time.
   Returns CAMERA_SUCCESS on success or CAMERA_FAILURE on internal
   error. */

/* Set up for C function definitions, even when using C++ */

#endif
