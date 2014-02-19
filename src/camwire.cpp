#include <dc1394/vendor/avt.h>

#include <camwire_config.hpp>
#include <camwire.hpp>
#include <cstring>
#include <unistd.h>         //sleep function
#include <cmath>            //log function
#include <cfloat>           //definition of DBL_MAX

camwire::camwire::camwire()
{

}

camwire::camwire::~camwire()
{

}

camwire::camwire::camwire(const camwire &cam)
{

}

camwire::camwire &camwire::camwire::operator=(const camwire &cam)
{
    return *this;
}

int camwire::camwire::create(const Camwire_bus_handle_ptr &c_handle, const Camwire_state_ptr &set)
{
    try
    {
        /* Allocate zero-filled space for internal status, and register a
                   pointer to it in the camera handle: */
        User_handle internal_status(new Camwire_user_data);
        Camwire_conf_ptr config;

        /* Camwire_user_data is defined above.*/

        ERROR_IF_NULL(internal_status); 	/* Allocation failure.*/
        if (!c_handle->handle_set_userdata(internal_status))
        {   /* Already in use.*/
            DPRINTF("camwire_bus_set_userdata() failed.");
            return CAMWIRE_FAILURE;
        }

        /* Allocate zero-filled space for the extra features: */
        internal_status->extras = Extra_features_ptr(new Extra_features);
        if (!internal_status->extras)
        { 	/* Allocation failure.*/
            DPRINTF("calloc(Extra_features) failed.");
            return CAMWIRE_FAILURE;
        }

        /* Allocate zero-filled space for the config cache: */
        internal_status->config_cache = Camwire_conf_ptr(new Camwire_conf);
        if (!internal_status->config_cache)
        { 	/* Allocation failure.*/
            DPRINTF("calloc(Camwire_conf) failed.");
            free_internals(c_handle);
            return CAMWIRE_FAILURE;
        }

        /* Allocate space and make a copy of the initial settings: */
        internal_status->current_set = Camwire_state_ptr(new Camwire_state);
        if (!internal_status->current_set)
        { 	/* Allocation failure.*/
            DPRINTF("malloc(Camwire_state) failed.");
            free_internals(c_handle);
            return CAMWIRE_FAILURE;
        }

        internal_status->current_set = set;

        /* Get 1394-specific hardware configuration: */
        if (get_config(c_handle, config) != CAMWIRE_SUCCESS)
        {
            DPRINTF("camwire_get_config() failed.");
            free_internals(c_handle);
            return CAMWIRE_FAILURE;
        }

        /* Make sure the video1394 device is not already listening on this
           channel (due to a provious process being killed without resetting
           the camera).  Hopefully this won't hurt anything: */
        dc1394_capture_stop(c_handle->camera.get());
        dc1394_iso_release_all(c_handle->camera.get());

        /* Connect the camera to the bus and initialize it with our
           settings: */
        if (connect_cam(c_handle, &config, set) != CAMWIRE_SUCCESS)
        {
            DPRINTF("connect_cam() failed.");
            free_internals(c_handle);
            return CAMWIRE_FAILURE;
        }

        return CAMWIRE_SUCCESS;
    }
    catch(std::runtime_error &re)
    {
        DPRINTF("Failed to init camera");
        return CAMWIRE_FAILURE;
    }
}

int camwire::camwire::generate_default_config(const Camwire_bus_handle_ptr &c_handle, Camwire_conf_ptr &conf)
{
    return CAMWIRE_SUCCESS;
}

int camwire::camwire::generate_default_settings(const Camwire_bus_handle_ptr &c_handle, Camwire_state_ptr &set)
{
    try
    {
        /* Initialize the camera to factory settings: */
        ERROR_IF_NULL(c_handle);
        /* dc1394_camera_reset() does not work on all cameras, so we are
           lenient on the test: */
        int dc1394_return = dc1394_camera_reset(c_handle->camera.get());
        if (dc1394_return != DC1394_SUCCESS)
        {
            DPRINTF("Warning: dc1394_camera_reset() failed.  Continuing configuration, "
            "but camera may not be properly initialized.");
            sleep(1);  /* Increase chances that camera may recover.*/
        }

        dc1394video_mode_t video_mode = get_1394_video_mode(c_handle.get());
        dc1394color_coding_t color_id;
        dc1394framerates_t supported_fr;
        dc1394feature_info_t capability;
        dc1394bool_t on_off;
        uint32_t num_packets = 0;
        /* Merging the functions, reducing the number of checks on variables */
        /* Format and mode-specific frame dimensions and pixel coding: */
        if(fixed_image_size(video_mode))    /* Formats 0-2.*/
        {
            set->left = 0;  /* pixels */
            set->top = 0;   /* pixels */
            ERROR_IF_DC1394_FAIL(
                        dc1394_get_image_size_from_video_mode(c_handle->camera.get(),
                                                             video_mode,
                                                             (uint32_t *)&set->width,
                                                             (uint32_t *)&set->height));
            set->coding = convert_videomode2pixelcoding(video_mode);

            /* Determine the maximum supported framerate in this mode and
               format: */
            ERROR_IF_DC1394_FAIL(
                    dc1394_video_get_supported_framerates(c_handle->camera.get(),
                                      video_mode,
                                      &supported_fr));
            if(supported_fr.num == 0)
            {
                DPRINTF("dc1394_video_get_supported_framerates() returned no "
                            "framerates.");
                return CAMWIRE_FAILURE;
            }

            set->frame_rate = 0;
            double fr = 0.0f;
            for (int f=0; f < supported_fr.num; ++f)
            {
                fr = convert_index2framerate(supported_fr.framerates[f]);
                if (fr < 0.0f)
                {
                    DPRINTF("convert_index2framerate() failed.");
                    return CAMWIRE_FAILURE; 	/* Invalid index.*/
                }
                if (fr > set->frame_rate)  set->frame_rate = fr;
            }
        }
        else if(variable_image_size(video)) /* Format 7.*/
        {
            ERROR_IF_DC1394_FAIL(
               dc1394_format7_get_image_position(c_handle->camera.get(),
                                video_mode,
                                (uint32_t *)&set->left,
                                (uint32_t *)&set->top));
            ERROR_IF_DC1394_FAIL(
               dc1394_format7_get_image_size(c_handle->camera.get(),
                                video_mode,
                                (uint32_t *)&set->width,
                                (uint32_t *)&set->height));
            ERROR_IF_DC1394_FAIL(
               dc1394_format7_get_color_coding(
                c_handle->camera.get(),
                video_mode,
                &color_id));
            set->coding = convert_colorid2pixelcoding(color_id);

            /* Determine the maximum supported framerate in this mode and
               format: */

            /* PACKET_PER_FRAME_INQ depends on BYTE_PER_PACKET which in turn
               depends on IMAGE_SIZE and COLOR_CODING_ID.  Since we are not
               changing these registers, it is safe to use the value
               returned by get_num_packets(): */
            ERROR_IF_CAMWIRE_FAIL(get_numpackets(c_handle, num_packets));
            set->frame_rate = convert_numpackets2framerate(c_handle, num_packets);
        }
        else
        {
            DPRINTF("Camera's format is not supported");
            return CAMWIRE_FAILURE;
        }

        /* Get the shutter speed and try to fit it into one frame period: */
        if(set->frame_rate != 0)
            set->shutter = 0.5/set->frame_rate; /* Seconds, default.*/

        capability.id = DC1394_FEATURE_SHUTTER;
        ERROR_IF_DC1394_FAIL(
            dc1394_feature_get(c_handle->camera.get(),
                       &capability));

        double max_shutter, min_shutter;
        Camwire_conf_ptr config(new Camwire_conf);
        if(feature_is_usable(capability))
        {
            ERROR_IF_CAMWIRE_FAIL(get_config(c_handle, config));
            set->shutter = config->exposure_offset + capability.value * config->exposure_quantum;
            max_shutter = config->exposure_quantum * ((unsigned long int)(1.0 / (set->frame_rate * config->exposure_quantum)));

            if(set->shutter > max_shutter)
                set->shutter = max_shutter;

            min_shutter = config->exposure_offset + capability.min * config->exposure_quantum;

            if(set->shutter < min_shutter)
                set->shutter = min_shutter;
        }
        else
            DPRINTF("Camera reported no usable shutter");

        /* Format and mode-independent settings: */
        set->external_trigger = 0;  /* Flag */
        set->trigger_polarity = 1;  /* Flag, default */
        capability.id = DC1394_FEATURE_TRIGGER;
        ERROR_IF_DC1394_FAIL(dc1394_feature_get(c_handle->camera.get(), &capability));
        if (feature_is_usable(capability))
        {
            if (capability.trigger_polarity == DC1394_TRIGGER_ACTIVE_LOW)
                set->trigger_polarity = 0;
            else
                set->trigger_polarity = 1;
        }

        /* Get the factory gain and set our normalized gain accordingly: */
        set->gain = 0.0;			/* Units, default.*/
        capability.id = DC1394_FEATURE_GAIN;
        ERROR_IF_DC1394_FAIL(dc1394_feature_get(c_handle->camera.get(), &capability));
        if (feature_is_usable(capability))
        {
            if (capability.max != capability.min)
                set->gain = (double)(capability.value - capability.min) /
                (capability.max - capability.min);
        }
        else
            DPRINTF("Camera reported no usable gain.");

        /* Get the factory brightness and set our normalized brightness
           accordingly: */
        set->brightness = 0.0;		/* Units, default.*/
        capability.id = DC1394_FEATURE_BRIGHTNESS;
        ERROR_IF_DC1394_FAIL(dc1394_feature_get(c_handle->camera.get(), &capability));
        if (feature_is_usable(capability))
        {
            set->brightness = 2.0*(double)(capability.value - capability.min) /
                (capability.max - capability.min) - 1.0;
        }
        else
            DPRINTF("Camera reported no usable brightness.");

        /* Get the factory white balance and set our normalized levels
           accordingly: */
        set->white_balance[0] = set->white_balance[1] = 0.0; 	/* Units, default.*/
        capability.id = DC1394_FEATURE_WHITE_BALANCE;
        ERROR_IF_DC1394_FAIL(dc1394_feature_get(c_handle->camera.get(), &capability));
        if (feature_is_usable(capability))
        {
            if (capability.max != capability.min)
            {
                set->white_balance[0] = (double)(capability.BU_value - capability.min) /
                (capability.max - capability.min);
                set->white_balance[1] = (double)(capability.RV_value - capability.min) /
                (capability.max - capability.min);
            }
        }
        else
            DPRINTF("Camera reported no usable white balance.");

        /* Enable colour correction by default if the camera supports it,
           and get the factory colour correction coefficients: */
        int32_t coef_reg[9];
        set->colour_corr = probe_camera_colour_correction(c_handle);
        if (set->colour_corr) 		/* Flag, on by default.*/
        {
        ERROR_IF_DC1394_FAIL(dc1394_avt_get_color_corr(c_handle->camera.get(),
                          &on_off,
                          &coef_reg[0], &coef_reg[1], &coef_reg[2],
                          &coef_reg[3], &coef_reg[4], &coef_reg[5],
                          &coef_reg[6], &coef_reg[7], &coef_reg[8]));
            convert_avtvalues2colourcoefs(coef_reg, set->colour_coef);
        }
        else
            DPRINTF("Camera reported no usable colour correction.");

        /* Enable gamma if the camera supports it: */
        set->gamma = probe_camera_gamma(c_handle);  /* Flag.*/
        if (!set->gamma)
            DPRINTF("Camera reported no usable gamma correction.");


        /* Other defaults: */
        set->tiling = probe_camera_tiling(c_handle);  /* Pattern.*/
        set->num_frame_buffers = 10;        /* Frames.*/
        set->single_shot = 0;               /* Flag.*/
        set->running = 0;                   /* Flag.*/
        set->shadow = 1;                    /* Flag.*/

        return CAMWIRE_SUCCESS;
    }
    catch(std::runtime_error &re)
    {
        DPRINTF("Failed to generate default settings");
        return CAMWIRE_FAILURE;
    }
}

int camwire::camwire::get_current_settings(const Camwire_bus_handle_ptr &c_handle, Camwire_state_ptr &set)
{
    try
    {
        Camwire_state_ptr shadow_state(new Camwire_state);
        dc1394bool_t one_shot_set;

        //*shadow_state = get_shadow_state(c_handle);
        ERROR_IF_NULL(shadow_state);
        if(shadow_state->shadow)
        {
            set = shadow_state;
            /* One_Shot register self-clears after transmission, hence we
               don't know if camera is still runnning: */
            if (shadow_state->running && shadow_state->single_shot)
            {
                ERROR_IF_DC1394_FAIL(dc1394_video_get_one_shot(c_handle->camera.get(), &one_shot_set));
                if (one_shot_set == DC1394_FALSE)
                    set->running = shadow_state->running = 0;
            }
        }
        else
        {
          /*
            ERROR_IF_CAMWIRE_FAIL(
                camwire_get_num_framebuffers(c_handle, &set->num_frame_buffers));
            ERROR_IF_CAMWIRE_FAIL(
                camwire_get_gain(c_handle, &set->gain));
            ERROR_IF_CAMWIRE_FAIL(
                camwire_get_brightness(c_handle, &set->brightness));
            ERROR_IF_CAMWIRE_FAIL(
                camwire_get_white_balance(c_handle, set->white_balance));
            ERROR_IF_CAMWIRE_FAIL(
                camwire_get_gamma(c_handle, &set->gamma));
            ERROR_IF_CAMWIRE_FAIL(
                camwire_get_colour_correction(c_handle, &set->colour_corr));
            ERROR_IF_CAMWIRE_FAIL(
                camwire_get_colour_coefficients(c_handle, set->colour_coef));
            ERROR_IF_CAMWIRE_FAIL(
                camwire_get_frame_offset(c_handle, &set->left, &set->top));
            ERROR_IF_CAMWIRE_FAIL(
                camwire_get_frame_size(c_handle, &set->width, &set->height));
            ERROR_IF_CAMWIRE_FAIL(
                camwire_get_pixel_coding(c_handle, &set->coding));
            ERROR_IF_CAMWIRE_FAIL(
                camwire_get_pixel_tiling(c_handle, &set->tiling));
            ERROR_IF_CAMWIRE_FAIL(
                camwire_get_framerate(c_handle, &set->frame_rate));
            ERROR_IF_CAMWIRE_FAIL(
                camwire_get_shutter(c_handle, &set->shutter));
            ERROR_IF_CAMWIRE_FAIL(
                camwire_get_trigger_source(c_handle, &set->external_trigger));
            ERROR_IF_CAMWIRE_FAIL(
                camwire_get_trigger_polarity(c_handle, &set->trigger_polarity));
            ERROR_IF_CAMWIRE_FAIL(
                camwire_get_single_shot(c_handle, &set->single_shot));
            ERROR_IF_CAMWIRE_FAIL(
                camwire_get_run_stop(c_handle, &set->running));
            ERROR_IF_CAMWIRE_FAIL(
                camwire_get_stateshadow(c_handle, &set->shadow));*/
        }

        return CAMWIRE_SUCCESS;

    }
    catch(std::runtime_error &re)
    {
        DPRINTF("Failed to retrieve current settings");
        return CAMWIRE_FAILURE;
    }
}

int camwire::camwire::sleep_frametime(const Camwire_bus_handle_ptr &c_handle, const double multiple)
{
    double frame_rate = 0.0f;
    double sleep_period = 0.0f;
    struct timespec nap, left;

    try
    {
        //ERROR_IF_CAMWIRE_FAIL(camwire_get_framerate(c_handle, &frame_rate));
        if(frame_rate != 0.0f)                  /* Avoiding division by 0 */
            sleep_period = multiple/frame_rate;
        nap.tv_sec = (time_t) sleep_period; 	/* Trunc. to integer.*/
        nap.tv_nsec = (long)((sleep_period - nap.tv_sec)*1e9);
        if (nanosleep(&nap, &left) != 0)
        {
            DPRINTF("nanosleep() failed.");
            return CAMWIRE_FAILURE;
        }
        return CAMWIRE_SUCCESS;
    }
    catch(std::runtime_error &re)
    {
        DPRINTF("Failed to set sleep time");
        return CAMWIRE_FAILURE;
    }
}

void camwire::camwire::disconnect_cam(const Camwire_bus_handle_ptr &c_handle)
{    
    try
    {
        User_handle internal_status = c_handle->userdata;
        if (internal_status)
        {
            if (internal_status->camera_connected)
            {
                if (internal_status->frame_lock)
                {
                    dc1394_capture_enqueue(c_handle->camera.get(), internal_status->frame.get());
                    internal_status->frame = 0;
                    internal_status->frame_lock = 0;
                }
                dc1394_capture_stop(c_handle->camera.get());
            }
            internal_status->camera_connected = 0;
        }
    }
    catch(std::runtime_error &re)
    {
        DPRINTF("Failed to disconnect camera");
    }
}

void camwire::camwire::free_internals(const Camwire_bus_handle_ptr &c_handle)
{
    try
    {
        User_handle internal_status = c_handle->userdata;
        if (internal_status)
        {
            if (internal_status->frame_lock)
            {
                dc1394_capture_enqueue(c_handle->camera.get(),
                           internal_status->frame.get());
                internal_status->frame = 0;
                internal_status->frame_lock = 0;
            }
        }
    }
    catch(std::runtime_error &re)
    {
        DPRINTF("Failed to free internal values");
    }
}

int camwire::camwire::config_cache_exists(const User_handle &internal_status)
{
    #ifdef CAMWIRE_DEBUG
    if (!internal_status)
    DPRINTF("Internal error: User_handle internal_status pointer is 0.");
    #endif
    if(internal_status->config_cache)
        return CAMWIRE_SUCCESS;
    else
        return CAMWIRE_FAILURE;
}

int camwire::camwire::find_conf_file(const Camwire_id &id, std::shared_ptr<FILE> &conffile)
{
    try
    {
        std::string env_directory;

        if(open_named_conf_file(0, id.chip, conffile) == CAMWIRE_SUCCESS)
            return CAMWIRE_SUCCESS;

        if(open_named_conf_file(0, id.model, conffile) == CAMWIRE_SUCCESS)
            return CAMWIRE_SUCCESS;

        if(open_named_conf_file(0, id.vendor, conffile) == CAMWIRE_SUCCESS)
            return CAMWIRE_SUCCESS;

        env_directory = getenv(ENVIRONMENT_VAR_CONF);
        if(env_directory.length() > 0)
        {
            if(open_named_conf_file(env_directory, id.chip, conffile) == CAMWIRE_SUCCESS)
                return CAMWIRE_SUCCESS;

            if(open_named_conf_file(env_directory, id.model, conffile) == CAMWIRE_SUCCESS)
                return CAMWIRE_SUCCESS;

            if(open_named_conf_file(env_directory, id.vendor, conffile) == CAMWIRE_SUCCESS)
                return CAMWIRE_SUCCESS;
        }

        DPRINTF("No configuration file found");
        return CAMWIRE_FAILURE;
    }
    catch(std::runtime_error &re)
    {
        DPRINTF("Failed to find configuration file");
        return CAMWIRE_FAILURE;
    }
}

int camwire::camwire::open_named_conf_file(const std::string &path, const std::string &filename, std::shared_ptr<FILE> &conffile)
{
    try
    {
        std::string conffilename("");
        if(path.length() > 0)
        {
            conffilename = path;
            if(conffilename[conffilename.length() - 1] != '/')
                conffilename += "/";
        }

        conffilename += filename + CONFFILE_EXTENSION;
        /* Check if previously pointing to other data and release it */
        if(conffile)
            conffile.reset();

        conffile = std::shared_ptr<FILE>(fopen(conffilename.c_str(), "r"));
        if(conffile)
            return CAMWIRE_SUCCESS;
        else
            return CAMWIRE_FAILURE;
    }
    catch(std::runtime_error &re)
    {
        DPRINTF("Failed to open configuration file");
        return CAMWIRE_FAILURE;
    }
}

/* Keeping C-style I/O operations just for compatibility with Camwire original code */
/* This will be in future converted into C++ style, using fstream: it's cleaner */
int camwire::camwire::read_conf_file(const std::shared_ptr<FILE> &conffile, Camwire_conf_ptr &cfg)
{
    int scan_result = 0, speed = 0, num_bits_set = 0;
    try
    {
        scan_result =
        fscanf(conffile.get(),
               "Camwire IEEE 1394 IIDC DCAM hardware configuration:\n"
               "  bus_speed:           %d\n"
               "  format:              %d\n"
               "  mode:                %d\n"
               "  max_packets:         %d\n"
               "  min_pixels:          %d\n"
               "  trig_setup_time:     %lf\n"
               "  exposure_quantum:    %lf\n"
               "  exposure_offset:     %lf\n"
               "  line_transfer_time:  %lf\n"
               "  transmit_setup_time: %lf\n"
               "  transmit_overlap:    %d\n"
               "  drop_frames:         %d\n"
               "  dma_device_name:     %[^\n]s",
               /* FIXME: bus_speed will soon disappear from config: */
               &cfg->bus_speed,
               &cfg->format,
               &cfg->mode,
               &cfg->max_packets,
               &cfg->min_pixels,
               &cfg->trig_setup_time,
               &cfg->exposure_quantum,
               &cfg->exposure_offset,
               &cfg->line_transfer_time,
               &cfg->transmit_setup_time,
               &cfg->transmit_overlap,
               &cfg->drop_frames,
               &cfg->dma_device_name);

        if (scan_result == EOF || scan_result < 12)
        {
            DPRINTF("fscanf() failed reading configuration file.");
            return CAMWIRE_FAILURE;
        }

        /* FIXME: bus_speed will soon disappear from config; no need to check: */
        /* Ensure that bus_speed is one of 100, 200, 400...: */
        num_bits_set = 0;
        speed = cfg->bus_speed/100;
        while (speed != 0)
        {
            if ((speed & 1) != 0)  ++num_bits_set;
            speed >>= 1;
        }

        if (cfg->bus_speed%100 != 0 || num_bits_set != 1)
        {
            DPRINTF("Invalid bus_speed in configuration file read.");
            return CAMWIRE_FAILURE;
        }
        return CAMWIRE_SUCCESS;
    }
    catch(std::runtime_error &re)
    {
        DPRINTF("Failed to read configuration file");
        return CAMWIRE_FAILURE;
    }
}

/* Keeping C-style I/O operations just for compatibility with Camwire original code */
/* This will be in future converted into C++ style, using fstream: it's cleaner */
int camwire::camwire::write_config_to_file(const std::shared_ptr<FILE> &outfile, const Camwire_conf_ptr &cfg)
{
    int print_result = 0;
    try
    {
        print_result = fprintf(outfile.get(),
                   "Camwire IEEE 1394 IIDC DCAM hardware configuration:\n"
                   "  bus_speed:           %d\n"
                   "  format:              %d\n"
                   "  mode:                %d\n"
                   "  max_packets:         %d\n"
                   "  min_pixels:          %d\n"
                   "  trig_setup_time:     %g\n"
                   "  exposure_quantum:    %g\n"
                   "  exposure_offset:     %g\n"
                   "  line_transfer_time:  %g\n"
                   "  transmit_setup_time: %g\n"
                   "  transmit_overlap:    %d\n"
                   "  drop_frames:         %d\n"
                   "  dma_device_name:     %s\n",
                   cfg->bus_speed,
                   cfg->format,
                   cfg->mode,
                   cfg->max_packets,
                   cfg->min_pixels,
                   cfg->trig_setup_time,
                   cfg->exposure_quantum,
                   cfg->exposure_offset,
                   cfg->line_transfer_time,
                   cfg->transmit_setup_time,
                   cfg->transmit_overlap,
                   cfg->drop_frames,
                   cfg->dma_device_name.c_str());

        if (print_result < 1)
        {
            return CAMWIRE_FAILURE;
        }
        fflush(outfile.get());
        return CAMWIRE_SUCCESS;
    }
    catch(std::runtime_error &re)
    {
        DPRINTF("Failed to write config file to disk");
        return CAMWIRE_FAILURE;
    }
}

int camwire::camwire::write_config_to_output(const Camwire_conf_ptr &cfg)
{
    try
    {
        std::cout <<
                   "Camwire IEEE 1394 IIDC DCAM hardware configuration:\n"
                   "  bus_speed:           "<< cfg->bus_speed << std::endl <<
                   "  format:              "<< cfg->format << std::endl <<
                   "  mode:                "<< cfg->mode << std::endl <<
                   "  max_packets:         "<< cfg->max_packets << std::endl <<
                   "  min_pixels:          "<< cfg->min_pixels << std::endl <<
                   "  trig_setup_time:     "<< cfg->trig_setup_time << std::endl <<
                   "  exposure_quantum:    "<< cfg->exposure_quantum << std::endl <<
                   "  exposure_offset:     "<< cfg->exposure_offset << std::endl <<
                   "  line_transfer_time:  "<< cfg->line_transfer_time << std::endl <<
                   "  transmit_setup_time: "<< cfg->transmit_setup_time << std::endl <<
                   "  transmit_overlap:    "<< cfg->transmit_overlap << std::endl <<
                   "  drop_frames:         "<< cfg->bus_speed << std::endl <<
                   "  dma_device_name:     "<< cfg->dma_device_name << std::endl << std::endl;

        return CAMWIRE_SUCCESS;
    }
    catch(std::runtime_error &re)
    {
        DPRINTF("Failed to write config file to disk");
        return CAMWIRE_FAILURE;
    }
}

int camwire::camwire::fixed_image_size(const dc1394video_mode_t video_mode)
{
    return (dc1394_is_video_mode_scalable(video_mode) == DC1394_FALSE &&
            dc1394_is_video_mode_still_image(video_mode) == DC1394_FALSE);
}

int camwire::camwire::variable_image_size(const dc1394video_mode_t video_mode)
{
    return (dc1394_is_video_mode_scalable(video_mode) == DC1394_TRUE &&
            dc1394_is_video_mode_still_image(video_mode) == DC1394_FALSE);
}

double camwire::camwire::convert_index2framerate(const dc1394framerate_t frame_rate_index)
{
    int dc1394_return = 0;
    float frame_rate = 0.0;

    try
    {
        dc1394_return = dc1394_framerate_as_float(frame_rate_index, &frame_rate);
        if (dc1394_return == DC1394_SUCCESS)
            return (double)frame_rate;
        else
            return -1.0;
    }
    catch(std::runtime_error &re)
    {
        DPRINTF("Failed to convert index to framerate");
        return -1.0;
    }
}

int camwire::camwire::convert_framerate2index(const double frame_rate, const dc1394framerates_t *framerate_list)
{
    try
    {
        float min_fr;
        double fps, log2f, best, diff;
        int nearest_index, r, rate_index;

        dc1394_framerate_as_float(DC1394_FRAMERATE_MIN, &min_fr);  /* 1.875.*/
        if (frame_rate > 0.0)
            fps = frame_rate;
        else
            fps = min_fr;

        log2f = log(fps/min_fr)/log(2);  /* 1.875->0, 3.75->1, 7.5->2, etc.*/
        best = DBL_MAX;
        nearest_index = -1;
        for (r=0; r<framerate_list->num; ++r)
        {
            rate_index = framerate_list->framerates[r];
            diff = fabs(log2f - rate_index + DC1394_FRAMERATE_MIN);
            if (diff < best)
            {
                best = diff;
                nearest_index = rate_index;
            }
        }
        if (nearest_index >= 0)
            return nearest_index;
        else
            return 0;  /* Empty list?*/
    }
    catch(std::runtime_error &re)
    {
        DPRINTF("Failed to convert framerate to index");
        return 0;
    }
}

camwire::Camwire_pixel camwire::camwire::convert_videomode2pixelcoding(const dc1394video_mode_t video_mode)
{
    switch (video_mode)
    {
        case DC1394_VIDEO_MODE_160x120_YUV444:
            return CAMWIRE_PIXEL_YUV444;  /* 24 bits/pixel.*/
            break;
        case DC1394_VIDEO_MODE_320x240_YUV422:
        case DC1394_VIDEO_MODE_640x480_YUV422:
        case DC1394_VIDEO_MODE_800x600_YUV422:
        case DC1394_VIDEO_MODE_1024x768_YUV422:
        case DC1394_VIDEO_MODE_1280x960_YUV422:
        case DC1394_VIDEO_MODE_1600x1200_YUV422:
            return CAMWIRE_PIXEL_YUV422;  /* 16 bits/pixel.*/
            break;
        case DC1394_VIDEO_MODE_640x480_YUV411:
            return CAMWIRE_PIXEL_YUV411;  /* 12 bits/pixel.*/
            break;
        case DC1394_VIDEO_MODE_640x480_RGB8:
        case DC1394_VIDEO_MODE_800x600_RGB8:
        case DC1394_VIDEO_MODE_1024x768_RGB8:
        case DC1394_VIDEO_MODE_1280x960_RGB8:
        case DC1394_VIDEO_MODE_1600x1200_RGB8:
            return CAMWIRE_PIXEL_RGB8;  /* 24 bits/pixel.*/
            break;
        case DC1394_VIDEO_MODE_640x480_MONO8:
        case DC1394_VIDEO_MODE_800x600_MONO8:
        case DC1394_VIDEO_MODE_1024x768_MONO8:
        case DC1394_VIDEO_MODE_1280x960_MONO8:
        case DC1394_VIDEO_MODE_1600x1200_MONO8:
            return CAMWIRE_PIXEL_MONO8;  /* 8 bits/pixel.*/
            break;
        case DC1394_VIDEO_MODE_640x480_MONO16:
        case DC1394_VIDEO_MODE_800x600_MONO16:
        case DC1394_VIDEO_MODE_1024x768_MONO16:
        case DC1394_VIDEO_MODE_1280x960_MONO16:
        case DC1394_VIDEO_MODE_1600x1200_MONO16:
            return CAMWIRE_PIXEL_MONO16;  /* 16 bits/pixel.*/
            break;
        default:
            return CAMWIRE_PIXEL_INVALID;  /* Unknown.*/
            break;
    }
}

camwire::Camwire_pixel camwire::camwire::convert_colorid2pixelcoding(const dc1394color_coding_t color_id)
{
    switch (color_id)
    {
        case DC1394_COLOR_CODING_MONO8:
            return CAMWIRE_PIXEL_MONO8;  /* 8 bits/pixel.*/
            break;
        case DC1394_COLOR_CODING_YUV411:
            return CAMWIRE_PIXEL_YUV411;  /* 12 bits/pixel.*/
            break;
        case DC1394_COLOR_CODING_YUV422:
            return CAMWIRE_PIXEL_YUV422;  /* 16 bits/pixel.*/
            break;
        case DC1394_COLOR_CODING_YUV444:
            return CAMWIRE_PIXEL_YUV444;  /* 24 bits/pixel.*/
            break;
        case DC1394_COLOR_CODING_RGB8:
            return CAMWIRE_PIXEL_RGB8;  /* 24 bits/pixel.*/
            break;
        case DC1394_COLOR_CODING_MONO16:
            return CAMWIRE_PIXEL_MONO16;  /* 16 bits/pixel.*/
            break;
        case DC1394_COLOR_CODING_RGB16:
            return CAMWIRE_PIXEL_RGB16;  /* 48 bits/pixel.*/
            break;
        case DC1394_COLOR_CODING_MONO16S:
            return CAMWIRE_PIXEL_MONO16S;  /* 16 bits/pixel.*/
            break;
        case DC1394_COLOR_CODING_RGB16S:
            return CAMWIRE_PIXEL_RGB16S;  /* 48 bits/pixel.*/
            break;
        case DC1394_COLOR_CODING_RAW8:
            return CAMWIRE_PIXEL_RAW8;  /* 8 bits/pixel.*/
            break;
        case DC1394_COLOR_CODING_RAW16:
            return CAMWIRE_PIXEL_RAW16;  /* 16 bits/pixel.*/
            break;
        default:
            return CAMWIRE_PIXEL_INVALID;  /* Not supported.*/
            break;
    }
}

camwire::Camwire_tiling camwire::camwire::convert_filterid2pixeltiling(const dc1394color_filter_t filter_id)
{
    switch (filter_id)
    {
        case DC1394_COLOR_FILTER_RGGB:
            return CAMWIRE_TILING_RGGB;
            break;
        case DC1394_COLOR_FILTER_GBRG:
            return CAMWIRE_TILING_GBRG;
            break;
        case DC1394_COLOR_FILTER_GRBG:
            return CAMWIRE_TILING_GRBG;
            break;
        case DC1394_COLOR_FILTER_BGGR:
            return CAMWIRE_TILING_BGGR;
            break;
        default:
            return CAMWIRE_TILING_INVALID;  /* Not supported.*/
            break;
    }
}

int camwire::camwire::create(const Camwire_bus_handle_ptr &c_handle)
{
    Camwire_state_ptr settings(new Camwire_state);
    try
    {
        /* Get factory default start-up settings: */
        ERROR_IF_NULL(c_handle);
        if (get_state(c_handle, settings) != CAMWIRE_SUCCESS)
        {
            DPRINTF("camwire_get_state() failed.");
            return CAMWIRE_FAILURE;
        }

        //return create(c_handle, &settings);

        return CAMWIRE_SUCCESS;
    }
    catch(std::runtime_error &re)
    {
        DPRINTF("Failed to initialize camera with default settings");
        return CAMWIRE_FAILURE;
    }
    //return create(c_handle, &settings);

}

int camwire::camwire::create_from_struct(const Camwire_bus_handle_ptr &c_handle, const Camwire_state_ptr &set)
{
    try
    {
        ERROR_IF_NULL(c_handle);
        return create(c_handle, set);

    }
    catch(std::runtime_error &re)
    {
        DPRINTF("Failed to initialize camera with settings");
        return CAMWIRE_FAILURE;
    }
}

int camwire::camwire::destroy(const Camwire_bus_handle_ptr &c_handle)
{
    ERROR_IF_NULL(c_handle);
    if (c_handle)
    {
        try
        {
            set_run_stop(c_handle);
            sleep_frametime(c_handle, 1.5);
            /* Reset causes problems with too many cameras, so comment it out: */
            /* dc1394_camera_reset(camwire_handle_get_camera(c_handle)); */
            disconnect_cam(c_handle);
            free_internals(c_handle);
            return CAMWIRE_SUCCESS;
        }
        catch(std::runtime_error &re)
        {
            DPRINTF("Failed to disconnect camera from bus");
            return CAMWIRE_FAILURE;
        }
    }
}

int camwire::camwire::get_state(const Camwire_bus_handle_ptr &c_handle, Camwire_state_ptr &set)
{
    try
    {
        ERROR_IF_NULL(c_handle);
        User_handle internal_status = c_handle->userdata;
        if (!internal_status || !internal_status->camera_connected)
        {  /* Camera does not exit.*/

           return generate_default_settings(c_handle, set);
        }
        else
        {  /* Camera exists.*/
           return generate_default_settings(c_handle, set);
        }
    }
    catch(std::runtime_error &re)
    {
        DPRINTF("Failed to get camera state");
        return CAMWIRE_FAILURE;
    }
}

int camwire::camwire::set_run_stop(const Camwire_bus_handle_ptr &c_handle, const int runsts)
{
    try
    {
        dc1394switch_t iso_en;
        dc1394bool_t one_shot_set;

        ERROR_IF_NULL(c_handle);
        User_handle internal_status = c_handle->userdata;
        ERROR_IF_NULL(internal_status);
        Camwire_state_ptr shadow_state = internal_status->current_set;
        ERROR_IF_NULL(shadow_state);
        if (shadow_state->shadow)
        {
            if (shadow_state->single_shot)
            {  /* Single-shot.*/
                if (shadow_state->running && !runsts)
                { 	/* Stop camera (even if it has already stopped): */
                ERROR_IF_DC1394_FAIL(
                    dc1394_video_set_one_shot(c_handle->camera.get(),
                                  DC1394_OFF));
                }
                else if (runsts)
                {  /* Run in single-shot mode (even if we think it is already): */
                ERROR_IF_DC1394_FAIL(
                    dc1394_video_set_one_shot(c_handle->camera.get(),
                                  DC1394_ON));
                }
                /* else do nothing.*/
            }
            else
            {  /* Continuous.*/
                if (shadow_state->running && !runsts)
                {  /* Stop camera: */
                ERROR_IF_DC1394_FAIL(
                    dc1394_video_set_transmission(
                    c_handle->camera.get(),
                    DC1394_OFF));
                }
                else if (!shadow_state->running && runsts)
                { 	/* Run camera: */
                ERROR_IF_DC1394_FAIL(
                    dc1394_video_set_transmission(
                    c_handle->camera.get(),
                    DC1394_ON));
                }
                /* else do nothing.*/
            }
        }
        else
        {  /* Don't use shadow: ask the camera: */
            ERROR_IF_DC1394_FAIL(
                dc1394_video_get_transmission(c_handle->camera.get(),
                              &iso_en));
            if (iso_en == DC1394_ON)
            {  /* Camera is running in continuous mode: */
                shadow_state->single_shot = 0;
                if (!runsts)
                {  /* Stop camera: */
                ERROR_IF_DC1394_FAIL(
                    dc1394_video_set_transmission(
                    c_handle->camera.get(),
                    DC1394_OFF));
                }
                /* else do nothing.*/
            }
            else
            {
                if (internal_status->extras->single_shot_capable)
                {
                ERROR_IF_DC1394_FAIL(
                    dc1394_video_get_one_shot(c_handle->camera.get(),
                                  &one_shot_set));
                if (one_shot_set == DC1394_TRUE)
                {  /* Camera is running in single-shot mode: */
                    shadow_state->single_shot = 1;
                    if (!runsts)
                    { 	/* Stop camera: */
                    ERROR_IF_DC1394_FAIL(
                        dc1394_video_set_one_shot(c_handle->camera.get(),
                                      DC1394_OFF));
                    }
                    /* else do nothing.*/
                }
                else if (runsts)
                {  /* Camera is stopped.  Have to use shadow to decide: */
                    if (!shadow_state->single_shot)
                    {  /* Run in continuous mode: */
                    ERROR_IF_DC1394_FAIL(
                        dc1394_video_set_transmission(
                        c_handle->camera.get(),
                        DC1394_ON));
                    }
                    else
                    {  /* Run in single-shot mode: */
                    ERROR_IF_DC1394_FAIL(
                        dc1394_video_set_one_shot(c_handle->camera.get(),
                                      DC1394_ON));
                    }
                }
                }
                else if (runsts)
                {  /* Camera is stopped.  Run in continuous mode: */
                ERROR_IF_DC1394_FAIL(
                    dc1394_video_set_transmission(
                    c_handle->camera.get(),
                    DC1394_ON));
                }
            }
        }
        shadow_state->running = runsts;
        return CAMWIRE_SUCCESS;
    }
    catch(std::runtime_error &re)
    {
        DPRINTF("Failed to start/stop camera");
        return CAMWIRE_FAILURE;
    }
}

int camwire::camwire::get_config(const Camwire_bus_handle_ptr &c_handle, Camwire_conf_ptr &cfg)
{
    try
    {
        ERROR_IF_NULL(c_handle);
        User_handle internal_status = c_handle->userdata;
        Camwire_id identifier;

        /* Use cached config if it is available: */
        if(internal_status && config_cache_exists(internal_status))
        {
            cfg = internal_status->config_cache;
        }
        else
        { 	/* Read a conf file and cache it.*/
            ERROR_IF_CAMWIRE_FAIL(get_identifier(c_handle, identifier));
            std::shared_ptr<FILE> conffile(new FILE);
            ERROR_IF_CAMWIRE_FAIL(find_conf_file(identifier, conffile));
            if (conffile)
            {
                ERROR_IF_CAMWIRE_FAIL(read_conf_file(conffile, cfg));
                fclose(conffile.get());
                if (internal_status && internal_status->config_cache)
                { /* A camera has been created (not strictly necessary).*/
                    internal_status->config_cache = cfg;
                }
            }
            else
            {
                std::cerr << std::endl <<
                "Camwire could not find a hardware configuration file.\n"
                "Generating a default configuration..." << std::endl;
                    ERROR_IF_CAMWIRE_FAIL(
                    generate_default_config(c_handle, cfg));
                std::cout << std::endl <<
                "----------------------------------------------------------------" << std::endl;

                    ERROR_IF_CAMWIRE_FAIL(
                    write_config_to_output(cfg));

                std::cout << std::endl <<
                "----------------------------------------------------------------\n"
                "\n"
                "This is a best guess of the camera and its bus's hardware\n"
                "configuration.  See README_conf in the Camwire distribution for\n"
                "details.\n\n"
                "To create a configuration file, copy the text between (but not\n"
                "including) the ----- lines into a file (and edit as needed).\n"
                "The filename must be identical to one of the camera's ID strings\n"
                "(as may be obtained from camwire_get_identifier()) appended by\n"
                "an extension of \".conf\".\n\n"
                "For the current camera suitable filenames are: \n" <<
                identifier.chip << ".conf \t(chip)" << std::endl <<
                identifier.model << ".conf \t(model)" << std::endl <<
                identifier.vendor << ".conf \t(vendor)\n"
                "Camwire checks for filenames like these in this\n"
                "chip-model-vendor order.  It first looks for the three filenames\n"
                "in the current working directory and after that in a directory\n"
                "given by the CAMWIRE_CONF environment variable.\n\n" << std::endl;
                return CAMWIRE_FAILURE;
            }
        }

        return CAMWIRE_SUCCESS;
    }
    catch(std::runtime_error &re)
    {
        DPRINTF("Failed to retrieve camera configuration");
        return CAMWIRE_FAILURE;
    }
}

int camwire::camwire::get_identifier(const Camwire_bus_handle_ptr &c_handle, Camwire_id &identifier)
{
    try
    {
        ERROR_IF_NULL(c_handle);
        Camera_handle camera = c_handle->camera;
        identifier.vendor = camera.get()->vendor;
        identifier.model = camera.get()->model;
        identifier.chip = std::to_string(camera.get()->guid);
        return CAMWIRE_SUCCESS;
    }
    catch(std::runtime_error &re)
    {
        DPRINTF("Failed to get identifier");
        return CAMWIRE_FAILURE;
    }
}
