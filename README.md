This project demonstrates some issues with copying and pasting graphics in FLTK 1.3.5 with `FL_ABI_VERSION` 10305.

It presents two canvases, and buttons to Copy and Paste.

![Screenshot](screenshot.png)

- Click a pixel on either canvas to change its color.
- Click Copy or press Ctrl+C to copy from the left canvas.
- Click Paste or press Ctrl+V to paste onto the right canvas.

I have observed two system-specific issues with this program:

1. On Windows (8 and 10 tested), pasting from this program to [paint.net](https://www.getpaint.net/) adds an extra translucent rightmost column and bottommost row. This also happens with FLTK's `device` test program. It does not happen with other editors I've tried, including MS Paint and GIMP.
2. On Linux (Xubuntu and Slackware tested), copying and then pasting in this program does not work: `Fl::clipboard_contains(Fl::clipboard_image)` returns `false`. Copying from a separate editor to this program does work, including Pinta and GIMP. The `copy_source` function is based on the usage example from the [`Fl_Copy_Surface` documentation](https://www.fltk.org/doc-1.3/classFl__Copy__Surface.html).

A fix for issue #1, [by Manolo](https://groups.google.com/d/msg/fltkgeneral/8Js7Fgf_86c/ETTY_DnLDgAJ):

```diff
 /** Destructor.
  */
 Fl_Copy_Surface::~Fl_Copy_Surface()
 {
 #ifdef __APPLE__
   complete_copy_pdf_and_tiff();
   fl_gc = oldgc;
   delete (Fl_Quartz_Surface_*)helper;
 #elif defined(WIN32)
   if(oldgc == fl_gc) oldgc = NULL;
   HENHMETAFILE hmf = CloseEnhMetaFile (gc);
   if ( hmf != NULL ) {
     if ( OpenClipboard (NULL) ){
       EmptyClipboard ();
       SetClipboardData (CF_ENHMETAFILE, hmf);
+      // now put BITMAP version of the graphics in the clipboard
+      RECT rect = {0, 0, width, height};
+      Fl_Offscreen of = CreateCompatibleBitmap(fl_GetDC(0), width, height);
+      fl_begin_offscreen(of);
+      fl_color(FL_WHITE); // draw white background
+      fl_rectf(0, 0, width, height);
+      PlayEnhMetaFile((HDC)fl_gc, hmf, &rect); // draw metafile to offscreen buffer
+      fl_end_offscreen();
+      SetClipboardData(CF_BITMAP, (HBITMAP)of);
+      fl_delete_offscreen(of);
       CloseClipboard ();
     }
     DeleteEnhMetaFile(hmf);
   }
   DeleteDC(gc);
   fl_gc = oldgc;
   delete (Fl_GDI_Surface_*)helper;
 #else // Xlib
   fl_pop_clip(); 
   unsigned char *data = fl_read_image(NULL,0,0,width,height,0);
   fl_window = oldwindow; 
   _ss->set_current();
   Fl::copy_image(data,width,height,1);
   delete[] data;
   fl_delete_offscreen(xid);
   delete (Fl_Xlib_Surface_*)helper;
 #endif
 }
```

Issue #2 is not yet solved.
