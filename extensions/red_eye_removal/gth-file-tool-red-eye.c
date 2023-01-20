/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/*
 *  GThumb
 *
 *  Copyright (C) 2009 Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <gthumb.h>
#include <extensions/image_viewer/gth-image-viewer-page.h>
#include "gth-file-tool-red-eye.h"


#define GET_WIDGET(x) (_gtk_builder_get_widget (self->priv->builder, (x)))
#define REGION_SEARCH_SIZE 3


static const double RED_FACTOR = 0.5133333;
static const double GREEN_FACTOR  = 1.0;
static const double BLUE_FACTOR =  0.1933333;


struct _GthFileToolRedEyePrivate {
	GdkPixbuf        *src_pixbuf;
	GtkBuilder       *builder;
	GthImageSelector *selector;
	GthZoomChange     original_zoom_change;
	GdkPixbuf        *new_pixbuf;
	char             *is_red;
};


G_DEFINE_TYPE_WITH_CODE (GthFileToolRedEye,
			 gth_file_tool_red_eye,
			 GTH_TYPE_IMAGE_VIEWER_PAGE_TOOL,
			 G_ADD_PRIVATE (GthFileToolRedEye))


static int
find_region (int   row,
 	     int   col,
 	     int  *rtop,
 	     int  *rbot,
	     int  *rleft,
	     int  *rright,
	     char *isred,
	     int   width,
	     int   height)
{
	int *rows, *cols, list_length = 0;
	int  mydir;
	int  total = 1;

	/* a relatively efficient way to find all connected points in a
	 * region.  It considers points that have isred == 1, sets them to 2
	 * if they are connected to the starting point.
	 * row and col are the starting point of the region,
	 * the next four params define a rectangle our region fits into.
	 * isred is an array that tells us if pixels are red or not.
	 */

	*rtop = row;
	*rbot = row;
	*rleft = col;
	*rright = col;

	rows = g_malloc (width * height * sizeof(int));
	cols = g_malloc (width * height * sizeof(int));

	rows[0] = row;
	cols[0] = col;
	list_length = 1;

	do {
		list_length -= 1;
		row = rows[list_length];
		col = cols[list_length];
		for (mydir = 0; mydir < 8 ; mydir++) {
			switch (mydir) {
			case 0:
				/*  going left */
				if (col - 1 < 0) break;
				if (isred[col-1+row*width] == 1) {
					isred[col-1+row*width] = 2;
					if (*rleft > col-1) *rleft = col-1;
					rows[list_length] = row;
					cols[list_length] = col-1;
					list_length+=1;
					total += 1;
				}
				break;
			case 1:
				/* up and left */
				if (col - 1 < 0 || row -1 < 0 ) break;
				if (isred[col-1+(row-1)*width] == 1 ) {
					isred[col-1+(row-1)*width] = 2;
					if (*rleft > col -1) *rleft = col-1;
					if (*rtop > row -1) *rtop = row-1;
					rows[list_length] = row-1;
					cols[list_length] = col-1;
					list_length += 1;
					total += 1;
				}
				break;
			case 2:
				/* up */
				if (row -1 < 0 ) break;
				if (isred[col + (row-1)*width] == 1) {
					isred[col + (row-1)*width] = 2;
					if (*rtop > row-1) *rtop=row-1;
					rows[list_length] = row-1;
					cols[list_length] = col;
					list_length +=1;
					total += 1;
				}
				break;
			case 3:
				/*  up and right */
				if (col + 1 >= width || row -1 < 0 ) break;
				if (isred[col+1+(row-1)*width] == 1) {
					isred[col+1+(row-1)*width] = 2;
					if (*rright < col +1) *rright = col+1;
					if (*rtop > row -1) *rtop = row-1;
					rows[list_length] = row-1;
					cols[list_length] = col+1;
					list_length += 1;
					total +=1;
				}
				break;
			case 4:
				/* going right */
				if (col + 1 >= width) break;
				if (isred[col+1+row*width] == 1) {
					isred[col+1+row*width] = 2;
					if (*rright < col+1) *rright = col+1;
					rows[list_length] = row;
					cols[list_length] = col+1;
					list_length += 1;
					total += 1;
				}
				break;
			case 5:
				/* down and right */
				if (col + 1 >= width || row +1 >= height ) break;
				if (isred[col+1+(row+1)*width] ==1) {
					isred[col+1+(row+1)*width] = 2;
					if (*rright < col +1) *rright = col+1;
					if (*rbot < row +1) *rbot = row+1;
					rows[list_length] = row+1;
					cols[list_length] = col+1;
					list_length += 1;
					total += 1;
				}
				break;
			case 6:
				/* down */
				if (row +1 >=  height ) break;
				if (isred[col + (row+1)*width] == 1) {
					isred[col + (row+1)*width] = 2;
					if (*rbot < row+1) *rbot=row+1;
					rows[list_length] = row+1;
					cols[list_length] = col;
					list_length += 1;
					total += 1;
				}
				break;
			case 7:
				/* down and left */
				if (col - 1 < 0  || row +1 >= height ) break;
				if (isred[col-1+(row+1)*width] == 1) {
					isred[col-1+(row+1)*width] = 2;
					if (*rleft  > col -1) *rleft = col-1;
					if (*rbot < row +1) *rbot = row+1;
					rows[list_length] = row+1;
					cols[list_length] = col-1;
					list_length += 1;
					total += 1;
				}
				break;
			default:
				break;
			}
		}
	}
	while (list_length > 0);  /* stop when we add no more */

	g_free (rows);
	g_free (cols);

	return total;
}


static void
init_is_red (GthFileToolRedEye *self,
	     GdkPixbuf         *pixbuf)
{
	int        width, height;
	int        rowstride, channels;
	guchar    *pixels;
	int        i, j;
	int        ad_red, ad_green, ad_blue;
	const int  THRESHOLD = 0;

	width = gdk_pixbuf_get_width (pixbuf);
	height = gdk_pixbuf_get_height (pixbuf);
	rowstride = gdk_pixbuf_get_rowstride (pixbuf);
	channels = gdk_pixbuf_get_n_channels (pixbuf);
	pixels = gdk_pixbuf_get_pixels(pixbuf);

	g_free (self->priv->is_red);
	self->priv->is_red = g_new0 (char, width * height);

	for (i = 0; i < height; i++)  {
		for (j = 0; j < width; j++) {
			int ofs = channels * j + i * rowstride;

      			ad_red = pixels[ofs] * RED_FACTOR;
			ad_green = pixels[ofs + 1] * GREEN_FACTOR;
			ad_blue = pixels[ofs + 2] * BLUE_FACTOR;

			//  This test from the gimp redeye plugin.

			if ((ad_red >= ad_green - THRESHOLD) && (ad_red >= ad_blue - THRESHOLD))
				self->priv->is_red[j + i * width] = 1;
     		}
	}
}


/* returns TRUE if the pixbuf has been modified */
static gboolean
fix_redeye (GdkPixbuf *pixbuf,
	    char      *isred,
	    int        x,
	    int        y)
{
	gboolean  region_fixed = FALSE;
	int       width;
	int       height;
	int       rowstride;
	int       channels;
	guchar   *pixels;
	int       search, i, j, ii, jj;
	int       ad_blue, ad_green;
	int       rtop, rbot, rleft, rright; /* edges of region */

	width = gdk_pixbuf_get_width (pixbuf);
	height = gdk_pixbuf_get_height (pixbuf);
	rowstride = gdk_pixbuf_get_rowstride (pixbuf);
	channels = gdk_pixbuf_get_n_channels (pixbuf);
	pixels = gdk_pixbuf_get_pixels (pixbuf);

	/*
   	 * if isred is 0, we don't think the point is red, 1 means red, 2 means
   	 * part of our region already.
   	 */

	for (search = 0; ! region_fixed && (search < REGION_SEARCH_SIZE); search++)
    		for (i = MAX (0, y - search); ! region_fixed && (i <= MIN (height - 1, y + search)); i++ )
      			for (j = MAX (0, x - search); ! region_fixed && (j <= MIN (width - 1, x + search)); j++) {
				if (isred[j + i * width] == 0)
					continue;

				isred[j + i * width] = 2;

				find_region (i, j, &rtop, &rbot, &rleft, &rright, isred, width, height);

				/* Fix the region. */
				for (ii = rtop; ii <= rbot; ii++)
					for (jj = rleft; jj <= rright; jj++)
						if (isred[jj + ii * width] == 2) { /* Fix the pixel. */
							int ofs;

							ofs = channels*jj + ii*rowstride;
							/*ad_red = pixels[ofs] * RED_FACTOR;*/
							ad_green = pixels[ofs + 1] * GREEN_FACTOR;
							ad_blue = pixels[ofs + 2] * BLUE_FACTOR;

							pixels[ofs] = ((float) (ad_green + ad_blue)) / (2.0 * RED_FACTOR);

							isred[jj + ii * width] = 0;
						}

				region_fixed = TRUE;
      			}

	return region_fixed;
}


static void
selector_selected_cb (GthImageSelector  *selector,
		      int                x,
		      int                y,
		      GthFileToolRedEye *self)
{
	GthViewerPage *viewer_page;

	viewer_page = gth_image_viewer_page_tool_get_page (GTH_IMAGE_VIEWER_PAGE_TOOL (self));

	_g_object_unref (self->priv->new_pixbuf);
	self->priv->new_pixbuf = gth_image_viewer_page_get_pixbuf (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	init_is_red (self, self->priv->new_pixbuf);
	if (fix_redeye (self->priv->new_pixbuf, self->priv->is_red, x, y))
		gth_image_viewer_page_set_pixbuf (GTH_IMAGE_VIEWER_PAGE (viewer_page), self->priv->new_pixbuf, FALSE);
}


static void
selector_motion_notify_cb (GthImageSelector  *selector,
		           int                x,
		           int                y,
		           GthFileToolRedEye *self)
{
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("x_spinbutton")), (double) x);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (GET_WIDGET ("y_spinbutton")), (double) y);
}


static GtkWidget *
gth_file_tool_red_eye_get_options (GthFileTool *base)
{
	GthFileToolRedEye *self;
	GtkWidget         *window;
	GthViewerPage     *viewer_page;
	GtkWidget         *viewer;
	GtkWidget         *options;

	self = (GthFileToolRedEye *) base;

	window = gth_file_tool_get_window (base);
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	if (! GTH_IS_IMAGE_VIEWER_PAGE (viewer_page))
		return NULL;

	self->priv->builder = _gtk_builder_new_from_file ("red-eye-removal-options.ui", "red_eye_removal");
	options = _gtk_builder_get_widget (self->priv->builder, "options");
	gtk_widget_show (options);

	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));
	self->priv->original_zoom_change = gth_image_viewer_get_zoom_change (GTH_IMAGE_VIEWER (viewer));
	gth_image_viewer_set_zoom_change (GTH_IMAGE_VIEWER (viewer), GTH_ZOOM_CHANGE_KEEP_PREV);
	self->priv->selector = (GthImageSelector *) gth_image_selector_new (GTH_SELECTOR_TYPE_POINT);
	gth_image_selector_set_mask_visible (self->priv->selector, FALSE);
	g_signal_connect (self->priv->selector,
			  "selected",
			  G_CALLBACK (selector_selected_cb),
			  self);
	g_signal_connect (self->priv->selector,
			  "motion_notify",
			  G_CALLBACK (selector_motion_notify_cb),
			  self);
	gth_image_viewer_set_tool (GTH_IMAGE_VIEWER (viewer), (GthImageViewerTool *) self->priv->selector);

	return options;
}


static void
gth_file_tool_red_eye_destroy_options (GthFileTool *base)
{
	GthFileToolRedEye *self;
	GtkWidget         *window;
	GthViewerPage     *viewer_page;

	self = (GthFileToolRedEye *) base;

	window = gth_file_tool_get_window (GTH_FILE_TOOL (self));
	viewer_page = gth_browser_get_viewer_page (GTH_BROWSER (window));
	gth_image_viewer_page_reset_viewer_tool (GTH_IMAGE_VIEWER_PAGE (viewer_page));

	_g_object_unref (self->priv->builder);
	_g_object_unref (self->priv->selector);
	g_free (self->priv->is_red);
	self->priv->builder = NULL;
	self->priv->selector = NULL;
	self->priv->is_red = NULL;
}


static void
gth_file_tool_red_eye_apply_options (GthFileTool *base)
{
	GthFileToolRedEye *self;
	GthViewerPage     *viewer_page;
	GtkWidget         *viewer;

	self = (GthFileToolRedEye *) base;

	if (self->priv->new_pixbuf == NULL)
		return;

	viewer_page = gth_image_viewer_page_tool_get_page (GTH_IMAGE_VIEWER_PAGE_TOOL (self));
	viewer = gth_image_viewer_page_get_image_viewer (GTH_IMAGE_VIEWER_PAGE (viewer_page));

	gth_image_viewer_set_zoom_change (GTH_IMAGE_VIEWER (viewer), self->priv->original_zoom_change);
	gth_image_viewer_page_set_pixbuf (GTH_IMAGE_VIEWER_PAGE (viewer_page), self->priv->new_pixbuf, TRUE);
	gth_file_tool_hide_options (GTH_FILE_TOOL (self));
}


static void
gth_file_tool_red_eye_finalize (GObject *object)
{
	GthFileToolRedEye *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GTH_IS_FILE_TOOL_RED_EYE (object));

	self = (GthFileToolRedEye *) object;

	_g_object_unref (self->priv->new_pixbuf);
	g_free (self->priv->is_red);
	_g_object_unref (self->priv->selector);
	_g_object_unref (self->priv->builder);

	/* Chain up */
	G_OBJECT_CLASS (gth_file_tool_red_eye_parent_class)->finalize (object);
}


static void
gth_file_tool_red_eye_reset_image (GthImageViewerPageTool *self)
{
	gth_image_viewer_page_reset (GTH_IMAGE_VIEWER_PAGE (gth_image_viewer_page_tool_get_page (GTH_IMAGE_VIEWER_PAGE_TOOL (self))));
	gth_file_tool_hide_options (GTH_FILE_TOOL (self));
}


static void
gth_file_tool_red_eye_class_init (GthFileToolRedEyeClass *klass)
{
	GObjectClass		    *gobject_class;
	GthFileToolClass	    *file_tool_class;
	GthImageViewerPageToolClass *image_viewer_page_tool_class;

	gobject_class = (GObjectClass*) klass;
	gobject_class->finalize = gth_file_tool_red_eye_finalize;

	file_tool_class = (GthFileToolClass *) klass;
	file_tool_class->get_options = gth_file_tool_red_eye_get_options;
	file_tool_class->destroy_options = gth_file_tool_red_eye_destroy_options;
	file_tool_class->apply_options = gth_file_tool_red_eye_apply_options;

	image_viewer_page_tool_class = (GthImageViewerPageToolClass *) klass;
	image_viewer_page_tool_class->reset_image = gth_file_tool_red_eye_reset_image;
}


static void
gth_file_tool_red_eye_init (GthFileToolRedEye *self)
{
	self->priv = gth_file_tool_red_eye_get_instance_private (self);
	self->priv->new_pixbuf = NULL;
	self->priv->is_red = NULL;
	gth_file_tool_construct (GTH_FILE_TOOL (self), "image-red-eye-symbolic", _("Red Eye Removal"), GTH_TOOLBOX_SECTION_COLORS);
	gtk_widget_set_tooltip_text (GTK_WIDGET (self), _("Remove the red eye effect caused by camera flashes"));
}
