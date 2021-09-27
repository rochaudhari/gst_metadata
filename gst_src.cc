#include <string.h>
#include <math.h>
#include <iostream>

#include "gstmark.h"

#include <gst/gst.h>

#define DEST_HOST "127.0.0.1"
#define AUDIO_CAPS "audio/x-raw,format=S16LE,channels=1"
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



GstPadProbeReturn Callback1(GstPad *pad,
                                                         GstPadProbeInfo *info,
                                                         gpointer user_data) {
  GstBuffer *buffer = (GstBuffer *)info->data;
 
  GstMapInfo inmap = GST_MAP_INFO_INIT;
  
  g_print("\n data sending from source ");
  GstMapInfo map = GST_MAP_INFO_INIT;

  GstMetaMarking* meta = GST_META_MARKING_ADD(buffer);

  meta->in_timestamp = 99000;
 
  /*GstMetaMarking* meta1 = GST_META_MARKING_GET(buffer);
  if(meta1)
    g_print("\n data is %lu ", meta1->in_timestamp);
*/

  return GST_PAD_PROBE_OK;
}

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

int
main (int argc,char *argv[])
{
	GstElement *source,*maddecoder,*audioconv;
	GstElement *rtpbin,*rtpsink,*rtppay;
	GstElement *pipeline;
	GMainLoop *loop;
	GstCaps *caps;
	GstPad *srcpad,*sinkpad;


	gst_init(&argc,&argv);

	pipeline = gst_pipeline_new(NULL);
	g_assert(pipeline);

	source = gst_element_factory_make("filesrc","source");
	g_assert (pipeline);

  ADD_PAD_PROBE(source, "src", Callback1,
                    GST_PAD_PROBE_TYPE_BUFFER, nullptr);
                    

//	maddecoder=gst_element_factory_make("mad","maddecoder");
//	g_assert (maddecoder);
	GstElement* wavparse = gst_element_factory_make("wavparse","wavparse");
  g_assert (wavparse);

	audioconv = gst_element_factory_make("audioconvert","audioconv");
	g_assert (audioconv);
  GstElement* capsfilterAFX = gst_element_factory_make("capsfilter", NULL);

	g_object_set(G_OBJECT(source),"location","/data1/roshan/spanish_story.wav",NULL);

	caps=gst_caps_from_string(AUDIO_CAPS);
	
  g_object_set(G_OBJECT(capsfilterAFX), "caps", caps, NULL);

  GstElement* rtpgstpay = gst_element_factory_make("rtpgstpay","rtpgstpay");

	gst_bin_add_many (GST_BIN (pipeline),source, wavparse, audioconv, rtpgstpay, capsfilterAFX, NULL);


	rtpsink=gst_element_factory_make ("udpsink","rtpsink");
	g_assert(rtpsink);
	g_object_set(rtpsink,"port",50000,"host","10.24.235.253",NULL);
	gst_bin_add_many(GST_BIN(pipeline),rtpsink,NULL);

	if(!gst_element_link_many(source, wavparse, audioconv,capsfilterAFX, rtpgstpay, rtpsink, NULL)){
		g_error("Failed to link ");
	}
/*
ADD_PAD_PROBE(capsfilterAFX, "src", Callback1,
                    GST_PAD_PROBE_TYPE_BUFFER, nullptr);
                    

ADD_PAD_PROBE(rtpgstpay, "sink", Callback1,
                    GST_PAD_PROBE_TYPE_BUFFER, nullptr);
*/
/*ADD_PAD_PROBE(rtpsink, "sink", Callback1,
                    GST_PAD_PROBE_TYPE_BUFFER, nullptr);

*/
	/*sinkpad = gst_element_get_request_pad (rtpbin, "send_rtp_sink_0");
  	srcpad = gst_element_get_static_pad (rtppay, "src");
  	if (gst_pad_link (srcpad, sinkpad) != GST_PAD_LINK_OK)
    		g_error ("Failed to link audio payloader to rtpbin");
  	gst_object_unref (srcpad);
	
	srcpad = gst_element_get_static_pad (rtpbin, "send_rtp_src_0");
  	sinkpad = gst_element_get_static_pad (rtpsink, "sink");
  	if (gst_pad_link (srcpad, sinkpad) != GST_PAD_LINK_OK)
    		g_error ("Failed to link rtpbin to rtpsink");
  	gst_object_unref (srcpad);
  	gst_object_unref (sinkpad);
*/
guint bus_watch_id;
//GMainLoop *loop;
//loop = g_main_loop_new(NULL, FALSE);
	loop=g_main_loop_new(NULL,FALSE);
  GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
  bus_watch_id = gst_bus_add_watch(bus, bus_call, loop);
  gst_object_unref(bus);


	g_print("starting sender pipeline\n");
	gst_element_set_state(pipeline,GST_STATE_PLAYING);
  GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(
      GST_BIN(pipeline), GST_DEBUG_GRAPH_SHOW_ALL,
      "test");

  g_print("started \n");

	g_main_loop_run(loop);
	g_print("stopping sender pipeline\n");
	gst_element_set_state(pipeline,GST_STATE_NULL);
	return 0;

}

