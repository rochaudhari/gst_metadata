#include <string.h>
#include <math.h>
#include <iostream>
#include <gst/gst.h>
#include "gstmark.h"

#define AUDIO_CAPS "audio/x-raw,format=S16LE,channels=1"


gboolean bus_call(GstBus *bus, GstMessage *msg,
                                         gpointer data) {
  GMainLoop *loop = (GMainLoop *)data;
  switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_EOS:
      std::cout << "End of stream" << std::endl;
      g_main_loop_quit(loop);
      break;
    case GST_MESSAGE_WARNING: {
      gchar *debug;
      GError *error;
      gst_message_parse_warning(msg, &error, &debug);
      std::cout << "WARNING from element " << GST_OBJECT_NAME(msg->src) << ": "
                << error->message << std::endl;
      g_free(debug);
      g_error_free(error);
      break;
    }
    case GST_MESSAGE_ERROR: {
      gchar *debug;
      GError *error;
      gst_message_parse_error(msg, &error, &debug);
      std::cout << "ERROR from element " << GST_OBJECT_NAME(msg->src) << ": "
                << error->message << std::endl;
      if (debug) std::cout << "Error details: " << debug << std::endl;
      g_free(debug);
      g_error_free(error);
      g_main_loop_quit(loop);
      break;
    }
    
    case GST_MESSAGE_STATE_CHANGED: {
      break;
    }
    default:
      break;
  }
  return TRUE;
}

#define ADD_PAD_PROBE(elem, probe_pad, probe_func, probe_type, probe_data) \
  do {                                                                     \
    GstPad *pad = gst_element_get_static_pad(elem, probe_pad);             \
    if (!pad) {                                                            \
      g_printerr("Failed to get %s pad in %s.\n", probe_pad,               \
                 GST_ELEMENT_NAME(elem));                                  \
    }                                                                      \
    gst_pad_add_probe(pad, probe_type, probe_func, probe_data, NULL);      \
    gst_object_unref(pad);                                                 \
  } while (0)

GstPadProbeReturn Callback(GstPad *pad,
                           GstPadProbeInfo *info,
                           gpointer user_data) {
                             
  GstBuffer *buffer = (GstBuffer *)info->data;
  //MaxineDSPipeline *pipeline = (MaxineDSPipeline *)user_data;
  GstMapInfo inmap = GST_MAP_INFO_INIT;

  GstMetaMarking* meta = GST_META_MARKING_GET(buffer);
  if(!meta)
    g_print("\n it is null ");
  else
    g_print("\n data received %lu", meta->in_timestamp);
 
  //g_print("\n data received");
  return GST_PAD_PROBE_OK;
}

int 
main (int argc,char *argv[])
{
	GstElement *rtpbin,*rtpsrc,*buffer,*rtppay,*audioconver, *audiosink;
	GstElement *pipeline;
	GMainLoop *loop;
	GstCaps *caps;
	GstPadLinkReturn lres;
	GstPad *srcpad,*sinkpad;

	gst_init(&argc,&argv);
	
	pipeline=gst_pipeline_new(NULL);
	g_assert (pipeline);
	rtpsrc=gst_element_factory_make("udpsrc","rtpsrc");
	g_assert (rtpsrc);
	g_object_set (rtpsrc,"port",50000,NULL);
  GstElement* rtpgstdepay = gst_element_factory_make("rtpgstdepay","rtpgstdepay");

  GstCaps *filterCaps = gst_caps_new_simple("application/x-rtp", NULL);
  g_object_set(G_OBJECT(rtpsrc), "caps", filterCaps, NULL);
  gst_caps_unref(filterCaps);

  GstElement* capsfilterAFX = gst_element_factory_make("capsfilter", NULL);

	caps=gst_caps_from_string(AUDIO_CAPS);

  g_object_set(G_OBJECT(capsfilterAFX), "caps", caps, NULL);
	
  gst_caps_unref(caps);
	
	gst_bin_add_many(GST_BIN (pipeline),rtpsrc,NULL);

 	GstElement* queue =gst_element_factory_make("queue","queue");
	g_assert (queue);

	audioconver=gst_element_factory_make("audioconvert","audioconver");
	g_assert (audioconver);

	GstElement* fakesink =gst_element_factory_make("fakesink","fakesink");
	g_assert (fakesink);

	GstElement* audioresample=gst_element_factory_make("audioresample","audioresample");
	g_assert (audioresample);

	gst_bin_add_many (GST_BIN(pipeline), rtpgstdepay, capsfilterAFX, audioconver,audioresample, fakesink,NULL);

	gboolean res=gst_element_link_many(rtpsrc, rtpgstdepay, capsfilterAFX, audioconver,audioresample, fakesink,NULL);
	g_assert(res==TRUE);

ADD_PAD_PROBE(capsfilterAFX, "src", Callback,
                    GST_PAD_PROBE_TYPE_BUFFER, nullptr);

	guint bus_watch_id;

	
	loop=g_main_loop_new(NULL,FALSE);
	GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
	bus_watch_id = gst_bus_add_watch(bus, bus_call, loop);
	gst_object_unref(bus);


	g_print ("starting receiver pipeline\n");
        gst_element_set_state (pipeline, GST_STATE_PLAYING);


  	g_main_loop_run (loop);

	g_print ("stopping receiver pipeline\n");
  	gst_element_set_state (pipeline, GST_STATE_NULL);

  	gst_object_unref (pipeline);

  	return 0;
}