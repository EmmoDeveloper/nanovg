// Stub implementations for MSDF (Multi-channel Signed Distance Field) functions
// These are placeholders until full text rendering is implemented

// Stub for building MSDF glyph bitmap
void fons__tt_buildGlyphBitmapMSDF(void* font, unsigned char* output,
                                    int out_w, int out_h, int out_stride,
                                    float scale_x, float scale_y,
                                    int glyph)
{
	// Not implemented - would generate MSDF bitmap
	(void)font; (void)output; (void)out_w; (void)out_h;
	(void)out_stride; (void)scale_x; (void)scale_y; (void)glyph;
}

// Stub for rendering MSDF glyph bitmap
void fons__tt_renderGlyphBitmapMSDF(void* font, unsigned char* output,
                                     int out_w, int out_h, int out_stride,
                                     float scale_x, float scale_y,
                                     int glyph)
{
	// Not implemented - would render MSDF bitmap
	(void)font; (void)output; (void)out_w; (void)out_h;
	(void)out_stride; (void)scale_x; (void)scale_y; (void)glyph;
}
