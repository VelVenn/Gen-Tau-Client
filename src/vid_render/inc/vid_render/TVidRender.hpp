#pragma once

#include <gst/gst.h>
#include <gst/gstelement.h>

namespace gentau {
class TVidRender
{
  private:
    GstElement* pipeline;
    GstElement* src;
    GstElement* parser;
    GstElement* decoder;
    GstElement* vconv;
    GstElement* conv;
    GstElement* sink;
};
}  // namespace gentau