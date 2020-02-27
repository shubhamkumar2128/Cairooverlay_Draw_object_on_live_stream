//compile using gcc cairo_overlay.c -o ca `pkg-config --cflags --libs gstreamer-1.0` -I/usr/include/cairo/ -L/usr/local/lib -lcairo
//run -> ./ca
//download gstreamer on linux
#include <gst/gst.h>
#include <gst/video/video.h>
#include <cairo.h>
#include <cairo-gobject.h>
#include <glib.h>

static void on_pad_added (GstElement *element, GstPad *pad, gpointer data){
    GstPad *sinkpad;
    GstPadLinkReturn ret;
    GstElement *decoder = (GstElement *) data;
    g_print ("Dynamic pad created, linking source/demuxer\n");
    sinkpad = gst_element_get_static_pad (decoder, "sink");

    if (gst_pad_is_linked (sinkpad)) {
        g_print("*** We are already linked ***\n");
        gst_object_unref (sinkpad);
        return;
    } else {
        g_print("proceeding to linking ...\n");
    }
    ret = gst_pad_link (pad, sinkpad){
    if (GST_PAD_LINK_FAILED (ret)) {    
        g_print("failed to link dynamically\n");

    } else {
g_print("dynamically link successful\n");

    }
    gst_object_unref (sinkpad);

}


static gboolean on_message (GstBus * bus, GstMessage * message, gpointer user_data)
{
  GMainLoop *loop = (GMainLoop *) user_data;
  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_ERROR:{
      GError *err = NULL;
      gchar *debug;

      gst_message_parse_error (message, &err, &debug);

      g_critical ("Got ERROR: %s (%s)", err->message, GST_STR_NULL (debug));

      g_main_loop_quit (loop);

      break;

    }

    case GST_MESSAGE_WARNING:{

      GError *err = NULL;

      gchar *debug;



      gst_message_parse_warning (message, &err, &debug);

      g_warning ("Got WARNING: %s (%s)", err->message, GST_STR_NULL (debug));

      g_main_loop_quit (loop);

      break;

    }

    case GST_MESSAGE_EOS:

      g_main_loop_quit (loop);

      break;

    default:

      break;

  }



  return TRUE;

}





typedef struct

{

  gboolean valid;

  int width;

  int height;

} CairoOverlayState;





static void

prepare_overlay (GstElement * overlay, GstCaps * caps, gpointer user_data)

{

  CairoOverlayState *state = (CairoOverlayState *) user_data;



  

  state->width = 250;

  state->height = 250;

  state->valid = TRUE;

}





static void draw_overlay (GstElement * overlay, cairo_t * cr, guint64 timestamp,

    guint64 duration, gpointer user_data)

{

  CairoOverlayState *s = (CairoOverlayState *) user_data;

  double scale;



  if (!s->valid)

    return;



  scale = 2 * (((timestamp / (int) 1e7) % 70) + 30) / 100.0;

  cairo_translate (cr, s->width / 2, (s->height / 2) - 30);

  cairo_scale (cr, scale, scale);



  cairo_move_to (cr, 0, 0);

  cairo_curve_to (cr, 0, -30, -50, -30, -50, 0);

  cairo_curve_to (cr, -50, 30, 0, 35, 0, 60);

  cairo_curve_to (cr, 0, 35, 50, 30, 50, 0);

  cairo_curve_to (cr, 50, -30, 0, -30, 0, 0);

  cairo_set_source_rgba (cr, 0.9, 0.0, 0.1, 0.7);

  cairo_fill (cr);

}



static GstElement *

setup_gst_pipeline (CairoOverlayState * overlay_state)

{

  GstElement *pipeline,*rtspsrc,*queue,*depayloader,*decoder;

  GstElement *cairo_overlay;

  GstElement *adaptor1, *adaptor2, *sink,*avdec;



  pipeline = gst_pipeline_new ("cairo-overlay-example");



 

  	rtspsrc = gst_element_factory_make ("rtspsrc", "rtspsr");

	queue = gst_element_factory_make ("queue", "que");

	    g_object_set (G_OBJECT (rtspsrc), "location", "rtsp://wowzaec2demo.streamlock.net/vod/mp4:BigBuckBunny_115k.mov", NULL);

	depayloader = gst_element_factory_make ("rtph264depay","depayloader");

	decoder = gst_element_factory_make ("h264parse", "decoder");

	avdec = gst_element_factory_make ("avdec_h264", "avdec");

  	adaptor1 = gst_element_factory_make ("videoconvert", "adaptor1");

  	cairo_overlay = gst_element_factory_make ("cairooverlay", "overlay");

  	adaptor2 = gst_element_factory_make ("videoconvert", "adaptor2");

  	sink = gst_element_factory_make ("autovideosink", "sink");

   

   if (!pipeline || !rtspsrc || !queue  || !depayloader || !decoder || !sink || !adaptor1 || !cairo_overlay || !adaptor2 || !avdec) {

    g_printerr ("Not all elements could be created.\n");

	return NULL;

    

  }

  

  gst_bin_add_many (GST_BIN (pipeline),rtspsrc, queue,depayloader,decoder,avdec,adaptor1, cairo_overlay, adaptor2,sink, NULL);



  g_assert (cairo_overlay);

  g_assert (rtspsrc);

  g_assert (queue);

  g_assert (depayloader);

  g_assert (sink);   



  if (!gst_element_link_many (queue,depayloader,decoder,avdec,adaptor1, cairo_overlay, adaptor2,sink, NULL)) {

    g_warning ("Failed to link elements!");

  }

  



  g_signal_connect (cairo_overlay, "draw",G_CALLBACK (draw_overlay), overlay_state);

  g_signal_connect (cairo_overlay, "caps-changed",G_CALLBACK (prepare_overlay), overlay_state);

  g_signal_connect (rtspsrc, "pad-added", G_CALLBACK (on_pad_added),queue);



  return pipeline;

}



int

main (int argc, char **argv)

{

  GMainLoop *loop;

  GstElement *pipeline;

  GstBus *bus;

  CairoOverlayState overlay_state = { FALSE, 0, 0 };



  gst_init (&argc, &argv);

  loop = g_main_loop_new (NULL, FALSE);



  pipeline = setup_gst_pipeline (&overlay_state);

  



  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));

  gst_bus_add_signal_watch (bus);

  g_signal_connect (G_OBJECT (bus), "message", G_CALLBACK (on_message), loop);

  gst_object_unref (GST_OBJECT (bus));



  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  g_main_loop_run (loop);



  gst_element_set_state (pipeline, GST_STATE_NULL);

  gst_object_unref (pipeline);



  return 0;

}
