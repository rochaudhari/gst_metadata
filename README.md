There are 2 pipelines in this program, first pipeline adds metadata into GstBuffer using `gst_buffer_add_meta` and uses `rtpgstpay` hoping this metadata will be payloaded and carried over the network.
However, retrieving this metadata in the same pipeline works, but when we try to query this metadata in receiver pipeline, it is not present when we use with `rtpgstdepay`.

To Compile:
`make`

To run:
`./gst_src`


