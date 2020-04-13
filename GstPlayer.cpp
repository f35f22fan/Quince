#include "GstPlayer.hpp"

#include <QUrl>

namespace quince {

static gboolean bus_callback(GstBus *bus, GstMessage *msg, gpointer data)
{
	//GMainLoop *loop = (GMainLoop*) data;
	
	switch (GST_MESSAGE_TYPE(msg))
	{
	case GST_MESSAGE_EOS:
		mtl_info("End of stream");
		//g_main_loop_quit(loop);
		break;
		
	case GST_MESSAGE_ERROR: {
		gchar *debug;
		GError *error;
		gst_message_parse_error(msg, &error, &debug);
		g_free(debug);
		mtl_warn("Error %s", error->message);
		g_error_free(error);
		//g_main_loop_quit(loop);
		break;
	}
	
	default: break;
	}
	
	return TRUE;
}

GstPlayer::GstPlayer(int argc, char *argv[])
{
	InitGst(argc, argv);
//	const char *file_path = "file:///home/fox/file.mp3";
//	Play(file_path);
}

GstPlayer::~GstPlayer()
{
	gst_element_set_state(play_elem_, GST_STATE_NULL);
	gst_object_unref(GST_OBJECT(play_elem_));
}

void
GstPlayer::InitGst(int argc, char *argv[])
{
	gst_init(&argc, &argv);
	play_elem_ = gst_element_factory_make("playbin", "play");
	GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(play_elem_));
	gst_bus_add_watch(bus, bus_callback, NULL);// loop_);
	gst_object_unref(bus);
}

void
GstPlayer::Play(const QString &full_path) {
	gst_element_set_state(play_elem_, GST_STATE_NULL);
	QString uri_path = QString("file://") + full_path;
	QByteArray encoded = QUrl(uri_path).toEncoded();
	g_object_set(G_OBJECT(play_elem_), "uri", encoded.data(), NULL);
	gst_element_set_state(play_elem_, GST_STATE_PLAYING);
}

}
