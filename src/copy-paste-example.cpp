#include <cstdio>

#pragma warning(push, 0)
#include <FL/Fl.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Copy_Surface.H>
#include <FL/fl_draw.H>
#pragma warning(pop)

#define CW 8 // canvas width in pixels
#define CH 8 // canvas height in pixels
#define PX 16 // pixel size
#define ON 0xABCDEF00 // on pixel color
#define OFF 0x12345600 // off pixel color
#define WH 24 // standard widget height
#define MR 12 // standard margin size

class Canvas : public Fl_Box {
public:
	Canvas(int X, int Y, int W, int H, const char *L = NULL);
	void draw();
	int handle(int event);
	bool pixels[CW * CH];
	bool copying;
};

void copy_source(Fl_Widget *, Canvas *source) {
	fprintf(stderr, "copy_source(wgt, source)\n");
	source->copying = true;
	Fl_Copy_Surface *surface = new Fl_Copy_Surface(CW, CH);
	surface->set_current();
	surface->draw(source);
	delete surface;
	Fl_Display_Device::display_device()->set_current();
	source->copying = false;
}

void paste_dest(Fl_Widget *wgt, Canvas *dest) {
	fprintf(stderr, "paste_dest(%s, dest)\n", wgt ? "wgt" : "NULL");
	if (!Fl::clipboard_contains(Fl::clipboard_image)) {
		fprintf(stderr, "!Fl::clipboard_contains(Fl::clipboard_image)\n");
//		return; // XXX Uncomment to guard against pasting non-image data
	}
	if (wgt) {
		// Triggers dest->handle(FL_PASTE), which then calls paste_dest(NULL, dest)
		fprintf(stderr, "Fl::paste(*dest, 1, Fl::clipboard_image)\n");
		Fl::paste(*dest, 1, Fl::clipboard_image);
		return;
	}
	Fl_Image *pasted = (Fl_Image *)Fl::event_clipboard();
	fprintf(stderr, "pasted == %p\n", pasted);
	if (pasted->count()) {
		fprintf(stderr, "pasted->data() == %p\n", pasted->data());
		if (pasted->data()) {
			fprintf(stderr, "*pasted->data() == {");
			for (int i = 0; i < 5; i++) {
				fprintf(stderr, "0x%02x, ", *(*pasted->data() + i));
			}
			fprintf(stderr, "...}\n");
		}
	} else {
		fprintf(stderr, "pasted->count() == 0\n");
	}
	int pw = pasted->w(), ph = pasted->h();
	if (pw > CW) { pw = CW; }
	if (ph > CH) { ph = CH; }
	for (int i = 0; i < ph; i++) {
		for (int j = 0; j < pw; j++) {
			const char *p = *pasted->data() + (i * pasted->w() + j) * pasted->d();
			Fl_Color c = fl_rgb_color(p[0], p[1], p[2]);
			dest->pixels[i * CW + j] = c == ON;
		}
	}
	dest->redraw();
}

Canvas::Canvas(int X, int Y, int W, int H, const char *L) : Fl_Box(X, Y, W, H, L), pixels(), copying(false) {}

void Canvas::draw() {
	int X = x(), Y = y(), W = w(), H = h();
	if (copying) {
		// 1:1 canvas pixels
		for (int i = 0; i < CH; i++) {
			for (int j = 0; j < CW; j++) {
				fl_color(pixels[i * CW + j] ? ON : OFF);
				fl_point(X + j, Y + i);
			}
		}
		return;
	}
	// FL_THIN_DOWN_FRAME
	fl_color(FL_GRAY_RAMP + 7);
	fl_xyline(X+W-1, Y, X, Y+H-1);
	fl_color(FL_GRAY_RAMP + 22);
	fl_xyline(X, Y+H-1, X+W-1, Y);
	// Large canvas pixels
	int dx = X + 1, dy = Y + 1;
	int pw = (W - 2) / CW, ph = (H - 2) / CH;
	for (int i = 0; i < CH; i++) {
		for (int j = 0; j < CW; j++) {
			fl_color(pixels[i * CW + j] ? ON : OFF);
			fl_rectf(dx + pw * j, dy + ph * i, pw, ph);
		}
	}
}

int Canvas::handle(int event) {
	int px, py;
	switch (event) {
	case FL_PUSH:
		// Toggle clicked pixel
		px = (Fl::event_x() - x() - 1) / PX;
		py = (Fl::event_y() - y() - 1) / PX;
		pixels[py * CW + px] = !pixels[py * CW + px];
		redraw();
		return 1;
	case FL_RELEASE:
		return 1;
	case FL_PASTE:
		// Copy 1:1 pixels to clipboard
		fprintf(stderr, "dest->handle(FL_PASTE)\n");
		if (Fl::event_clipboard_type() == Fl::clipboard_image) {
			fprintf(stderr, "Fl::event_clipboard_type() == Fl::clipboard_image\n");
			paste_dest(NULL, this);
			return 1;
		} else {
			fprintf(stderr, "Fl::event_clipboard_type() != Fl::clipboard_image\n");
			paste_dest(NULL, this); // XXX Comment to guard against pasting non-image data
			return 1;
		}
		// fallthrough
	default:
		return Fl_Box::handle(event);
	}
}

int main(int argc, char **argv) {
	fl_register_images(); // required for Linux/X11 to allow pasting tile graphics
	Fl::visual(FL_DOUBLE | FL_RGB);
	Fl::visible_focus(false);

	Fl_Window *window = new Fl_Window(MR+PX*CW+2+MR+PX*CW+2+MR, MR+WH+MR+PX*CH+2+MR+WH+MR);

	Fl_Box *heading = new Fl_Box(MR, MR, PX*CW+2+MR+PX*CW+2, WH, "Click pixels to change color:");
	heading->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);

	Canvas *source = new Canvas(MR, MR+WH+MR, PX*CW+2, PX*CH+2);
	Fl_Button *copy = new Fl_Button(MR, MR+WH+MR+PX*CH+2+MR, PX*CW+2, WH, "Copy");
	copy->shortcut(FL_COMMAND + 'c');
	copy->tooltip("Copy (Ctrl+C)");
	copy->callback((Fl_Callback *)copy_source, source);

	Canvas *dest = new Canvas(MR+PX*CW+2+MR, MR+24+MR, PX*CW+2, PX*CH+2);
	Fl_Button *paste = new Fl_Button(MR+PX*CW+2+MR, MR+WH+MR+PX*CH+2+MR, PX*CW+2, WH, "Paste");
	paste->shortcut(FL_COMMAND + 'v');
	paste->tooltip("Paste (Ctrl+V)");
	paste->callback((Fl_Callback *)paste_dest, dest);

	window->end();

	window->show(argc, argv);

	return Fl::run();
}
