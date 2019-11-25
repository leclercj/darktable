/*
    This file is part of darktable,
    copyright (c) 2019 Jacques Le Clerc

    darktable is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    darktable is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with darktable.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "bauhaus/bauhaus.h"
#include "develop/imageop.h"
#include "dtgtk/button.h"
#include "dtgtk/resetlabel.h"
#include "gui/gtk.h"
#include "iop/iop_api.h"

#include <gtk/gtk.h>
#include <stdlib.h>

#include "inpaint_bct.h"


// this is the version of the modules parameters,
// and includes version information about compile-time dt
DT_MODULE_INTROSPECTION(1, dt_iop_inpaint_params_t)


typedef enum dt_iop_inpaint_algo_t
{
  DT_IOP_INPAINT_BCT = 0,
} dt_iop_inpaint_algo_t;

typedef enum dt_iop_mask_t
{
  DT_IOP_MASK_RED = 0,
  DT_IOP_MASK_GREEN = 1,
  DT_IOP_MASK_BLUE = 2,
  DT_IOP_MASK_BLACK = 3,
  DT_IOP_MASK_WHITE = 4
} dt_iop_mask_t;

#define DT_IOP_MASK_DEFAULT DT_IOP_MASK_RED

#define DT_IOP_INPAINT_MASK_DILATION_MIN 0
#define DT_IOP_INPAINT_MASK_DILATION_MAX 32
#define DT_IOP_INPAINT_MASK_DILATION_STEP 1
#define DT_IOP_INPAINT_MASK_DILATION_DEFAULT 0

typedef struct dt_iop_inpaint_params_t
{
  // these are stored in db.
  dt_iop_inpaint_algo_t algo;
  dt_iop_mask_t mask;

  float mask_threshold;
  uint32_t mask_dilation;

  float bct_epsilon;
  float bct_kappa;
  float bct_sigma;
  float bct_rho;

} dt_iop_inpaint_params_t;


typedef struct dt_iop_inpaint_gui_data_t
{
  GtkWidget *algo;

  GtkWidget *mask_area;
  GtkWidget *mask;
  GtkWidget *mask_color;
  GtkWidget *mask_threshold;
  GtkWidget *mask_dilation;

  GtkWidget *bct_epsilon;
  GtkWidget *bct_kappa;
  GtkWidget *bct_sigma;
  GtkWidget *bct_rho;


  GSList *gw_list;
} dt_iop_inpaint_gui_data_t;

typedef struct dt_iop_inpaint_params_t dt_iop_inpaint_data_t;


// this returns a translatable name
const char *name()
{
  return _("inpaint");
}

int default_group()
{
  return IOP_GROUP_CORRECT;
}

int flags()
{
  return IOP_FLAGS_SUPPORTS_BLENDING;
}

int default_colorspace(dt_iop_module_t *self, dt_dev_pixelpipe_t *pipe, dt_dev_pixelpipe_iop_t *piece)
{
  return iop_cs_rgb;
}


void process(struct dt_iop_module_t *self, dt_dev_pixelpipe_iop_t *piece, const void *const ibuf, void *const obuf,
             const dt_iop_roi_t *const roi_in, const dt_iop_roi_t *const roi_out)
{
  const int width = roi_in->width;
  const int height = roi_in->height;
  const int ch = piece->colors;
  Data data;

  dt_iop_inpaint_params_t *p = (dt_iop_inpaint_params_t *)piece->data;

  switch(p->mask)
  {
   default:
   case DT_IOP_MASK_RED: ; break;
   case DT_IOP_MASK_GREEN: ; break;
   case DT_IOP_MASK_BLUE: ; break;
   case DT_IOP_MASK_BLACK: ; break;
   case DT_IOP_MASK_WHITE: ; break;
  }

  switch(p->algo)
  {
    case DT_IOP_INPAINT_BCT:

      data.rows = roi_in->width;
      data.cols = roi_in->height;
      data.channels = piece->colors;
      data.size = 1;
      data.Image = NULL;
      data.MImage = NULL;
  
      // data domain info
      data.Domain = NULL;
      data.MDomain = NULL;
  
      // time info
      data.heap = NULL;
      data.Tfield = NULL;
      data.ordered_points = NULL;
      data.nof_points2inpaint = 0;
  
      // parameters
      data.radius = 5;
      data.epsilon = p->bct_epsilon;
      data.kappa = p->bct_kappa;
      data.sigma = p->bct_sigma;
      data.rho = p->bct_rho;
      data.thresh = p->mask_threshold;
      data.delta_quant4 = 1;
      data.convex = NULL;
  
      // smoothing kernels and buffer
      data.lenSK1 = 0;
      data.lenSK2 = 0;
      data.SKernel1 = NULL;
      data.SKernel2 = NULL;
      data.Shelp = NULL;
  
      // inpaint buffer
      data.Ihelp = NULL;
  
      // flags
      data.ordergiven = 0;
      data.guidance = 1;
      data.inpaint_undefined = 0;
  
      // extension
      data.GivenGuidanceT = NULL;

      InpaintImage(&data);

      break;

    default:
      memcpy(obuf, ibuf, width * height * ch * sizeof(float));
      break;
  }
}


/** init, cleanup, commit to pipeline */
void init(dt_iop_module_t *module)
{
  // we don't need global data:
  module->global_data = NULL; // malloc(sizeof(dt_iop_inpaint_global_data_t));
  module->params = calloc(1, sizeof(dt_iop_inpaint_params_t));
  module->default_params = calloc(1, sizeof(dt_iop_inpaint_params_t));
  // our module is disabled by default
  module->default_enabled = 0;
  module->params_size = sizeof(dt_iop_inpaint_params_t);
  module->gui_data = NULL;

  // init defaults:
  dt_iop_inpaint_params_t tmp = {
    .algo = DT_IOP_INPAINT_BCT,
    .mask = DT_IOP_MASK_DEFAULT,
    .mask_threshold = DT_IOP_INPAINT_BCT_THRESH_DEFAULT,
    .mask_dilation = DT_IOP_INPAINT_MASK_DILATION_DEFAULT,
    .bct_epsilon = DT_IOP_INPAINT_BCT_EPSILON_DEFAULT,
    .bct_kappa = DT_IOP_INPAINT_BCT_KAPPA_DEFAULT,
    .bct_sigma = DT_IOP_INPAINT_BCT_SIGMA_DEFAULT,
    .bct_rho = DT_IOP_INPAINT_BCT_RHO_DEFAULT
  };

  memcpy(module->params, &tmp, sizeof(dt_iop_inpaint_params_t));
  memcpy(module->default_params, &tmp, sizeof(dt_iop_inpaint_params_t));
}

void cleanup(dt_iop_module_t *module)
{
  free(module->params);
  module->params = NULL;
}



/** commit is the synch point between core and gui, so it copies params to pipe data. */
void commit_params(struct dt_iop_module_t *self, dt_iop_params_t *params, dt_dev_pixelpipe_t *pipe,
                   dt_dev_pixelpipe_iop_t *piece)
{
  memcpy(piece->data, params, sizeof(dt_iop_inpaint_params_t));
}


static void display_algo_param_widget(dt_iop_inpaint_gui_data_t *g, dt_iop_inpaint_algo_t algo)
{
  for (GSList *gw = g->gw_list; gw ;gw = gw->next)
    switch(algo)
    {
      case DT_IOP_INPAINT_BCT:
        if (gw->data == g->mask_area
         || gw->data == g->mask_threshold
         || gw->data == g->mask_dilation
         || gw->data == g->bct_epsilon
         || gw->data == g->bct_kappa
         || gw->data == g->bct_sigma
         || gw->data == g->bct_rho)
          gtk_widget_show(gw->data);
        else
          gtk_widget_hide(gw->data);
        break;

      default:
        gtk_widget_hide(gw->data);
        break;
    }
}

static void algo_callback(GtkWidget *w, dt_iop_module_t *self)
{
  if (darktable.gui->reset) return;
  dt_iop_inpaint_gui_data_t *g = (dt_iop_inpaint_gui_data_t *)self->gui_data;
  dt_iop_inpaint_params_t *p = (dt_iop_inpaint_params_t *)self->params;
  p->algo = dt_bauhaus_combobox_get(w);
  display_algo_param_widget(g, p->algo);
  dt_dev_add_history_item(darktable.develop, self, TRUE);
}

static void mask_callback(GtkWidget *w, dt_iop_module_t *self)
{
  if (darktable.gui->reset) return;
  dt_iop_inpaint_params_t *p = (dt_iop_inpaint_params_t *)self->params;
  p->mask = dt_bauhaus_combobox_get(w);
  dt_dev_add_history_item(darktable.develop, self, TRUE);
}

static void mask_threshold_callback(GtkWidget *w, dt_iop_module_t *self)
{
  if (darktable.gui->reset) return;
  dt_iop_inpaint_params_t *p = (dt_iop_inpaint_params_t *)self->params;
  p->mask_threshold = dt_bauhaus_slider_get(w);
  dt_dev_add_history_item(darktable.develop, self, TRUE);
}

static void mask_dilation_callback(GtkWidget *w, dt_iop_module_t *self)
{
  if (darktable.gui->reset) return;
  dt_iop_inpaint_params_t *p = (dt_iop_inpaint_params_t *)self->params;
  p->mask_dilation = dt_bauhaus_slider_get(w);
  dt_dev_add_history_item(darktable.develop, self, TRUE);
}

static void bct_epsilon_callback(GtkWidget *w, dt_iop_module_t *self)
{
  if (darktable.gui->reset) return;
  dt_iop_inpaint_params_t *p = (dt_iop_inpaint_params_t *)self->params;
  p->bct_epsilon = dt_bauhaus_slider_get(w);
  dt_dev_add_history_item(darktable.develop, self, TRUE);
}

static void bct_kappa_callback(GtkWidget *w, dt_iop_module_t *self)
{
  if (darktable.gui->reset) return;
  dt_iop_inpaint_params_t *p = (dt_iop_inpaint_params_t *)self->params;
  p->bct_kappa = dt_bauhaus_slider_get(w);
  dt_dev_add_history_item(darktable.develop, self, TRUE);
}

static void bct_sigma_callback(GtkWidget *w, dt_iop_module_t *self)
{
  if (darktable.gui->reset) return;
  dt_iop_inpaint_params_t *p = (dt_iop_inpaint_params_t *)self->params;
  p->bct_sigma = dt_bauhaus_slider_get(w);
  dt_dev_add_history_item(darktable.develop, self, TRUE);
}

static void bct_rho_callback(GtkWidget *w, dt_iop_module_t *self)
{
  if (darktable.gui->reset) return;
  dt_iop_inpaint_params_t *p = (dt_iop_inpaint_params_t *)self->params;
  p->bct_rho = dt_bauhaus_slider_get(w);
  dt_dev_add_history_item(darktable.develop, self, TRUE);
}


/** gui callbacks, these are needed. */
void gui_update(dt_iop_module_t *self)
{
  // let gui match current parameters:
  dt_iop_inpaint_gui_data_t *g = (dt_iop_inpaint_gui_data_t *)self->gui_data;
  dt_iop_inpaint_params_t *p = (dt_iop_inpaint_params_t *)self->params;

  dt_bauhaus_combobox_set(g->algo, p->algo);
  dt_bauhaus_combobox_set(g->mask, p->mask);
  dt_bauhaus_slider_set(g->mask_threshold, p->mask_threshold);
  dt_bauhaus_slider_set(g->mask_dilation, p->mask_dilation);
  dt_bauhaus_slider_set(g->bct_epsilon, p->bct_epsilon);
  dt_bauhaus_slider_set(g->bct_kappa, p->bct_kappa);
  dt_bauhaus_slider_set(g->bct_sigma, p->bct_sigma);
  dt_bauhaus_slider_set(g->bct_rho, p->bct_rho);

  display_algo_param_widget(g, p->algo);
}

void gui_init(dt_iop_module_t *self)
{
  // init the slider 
  self->gui_data = malloc(sizeof(dt_iop_inpaint_gui_data_t));
  dt_iop_inpaint_gui_data_t *g = (dt_iop_inpaint_gui_data_t *)self->gui_data;
  g->gw_list = NULL;

  dt_iop_inpaint_params_t *p = (dt_iop_inpaint_params_t *)self->params;

  self->widget = gtk_box_new(GTK_ORIENTATION_VERTICAL, DT_BAUHAUS_SPACE);
  dt_gui_add_help_link(self->widget, dt_get_help_url(self->op));

  // Algorithm combobox
  g->algo = dt_bauhaus_combobox_new(self);
  dt_bauhaus_widget_set_label(g->algo, NULL, _("algorithm"));
  gtk_box_pack_start(GTK_BOX(self->widget), g->algo, TRUE, TRUE, 0);
  dt_bauhaus_combobox_add(g->algo, _("Based on Coherence Transport"));
  gtk_widget_set_tooltip_text(g->algo, _("in-paint algorithm"));
  g_signal_connect(G_OBJECT(g->algo), "value-changed", G_CALLBACK(algo_callback), self);

  g->mask_area = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, DT_BAUHAUS_SPACE);
  g->gw_list = g_slist_append(g->gw_list, g->mask_area);
  gtk_box_pack_start(GTK_BOX(self->widget), g->mask_area, TRUE, TRUE, 0);

  // Mask combobox
  g->mask = dt_bauhaus_combobox_new(self);
  dt_bauhaus_widget_set_label(g->mask, NULL, _("mask"));
  gtk_box_pack_start(GTK_BOX(g->mask_area), g->mask, TRUE, TRUE, 0);
  dt_bauhaus_combobox_add(g->mask, _("Red"));
  dt_bauhaus_combobox_add(g->mask, _("Green"));
  dt_bauhaus_combobox_add(g->mask, _("Blue"));
  dt_bauhaus_combobox_add(g->mask, _("Black"));
  dt_bauhaus_combobox_add(g->mask, _("White"));
  gtk_widget_set_tooltip_text(g->mask, _("Mask"));
  g_signal_connect(G_OBJECT(g->mask), "value-changed", G_CALLBACK(mask_callback), self);

  g->mask_color = dtgtk_togglebutton_new(dtgtk_cairo_paint_color, CPF_STYLE_BOX, NULL);
  gtk_box_pack_start(GTK_BOX(g->mask_area), g->mask_color, FALSE, FALSE, 0);

  // Mask Threshold slider
  g->mask_threshold = dt_bauhaus_slider_new_with_range(self,
    DT_IOP_INPAINT_BCT_THRESH_MIN, DT_IOP_INPAINT_BCT_THRESH_MAX, DT_IOP_INPAINT_BCT_THRESH_STEP, p->mask_threshold, 0);
  g->gw_list = g_slist_append(g->gw_list, g->mask_threshold);
  gtk_widget_set_tooltip_text(g->mask_threshold, _("Mask Threshold"));
  dt_bauhaus_widget_set_label(g->mask_threshold, NULL, _("mask threshold"));
  gtk_box_pack_start(GTK_BOX(self->widget), g->mask_threshold, TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->mask_threshold), "value-changed", G_CALLBACK(mask_threshold_callback), self);

   // Mask Dilation slider
  g->mask_dilation = dt_bauhaus_slider_new_with_range(self,
    DT_IOP_INPAINT_MASK_DILATION_MIN, DT_IOP_INPAINT_MASK_DILATION_MAX, DT_IOP_INPAINT_MASK_DILATION_STEP, p->mask_dilation, 0);
  g->gw_list = g_slist_append(g->gw_list, g->mask_dilation);
  gtk_widget_set_tooltip_text(g->mask_dilation, _("Mask Dilation"));
  dt_bauhaus_widget_set_label(g->mask_dilation, NULL, _("mask dilation"));
  gtk_box_pack_start(GTK_BOX(self->widget), g->mask_dilation, TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->mask_dilation), "value-changed", G_CALLBACK(mask_dilation_callback), self);

  // Epsilon slider
  g->bct_epsilon = dt_bauhaus_slider_new_with_range(self,
    DT_IOP_INPAINT_BCT_EPSILON_MIN, DT_IOP_INPAINT_BCT_EPSILON_MAX, DT_IOP_INPAINT_BCT_EPSILON_STEP, p->bct_epsilon, 0);
  g->gw_list = g_slist_append(g->gw_list, g->bct_epsilon);
  gtk_widget_set_tooltip_text(g->bct_epsilon, _("Pixel neighborhood (epsilon)"));
  dt_bauhaus_widget_set_label(g->bct_epsilon, NULL, _("pixel neighborhood"));
  gtk_box_pack_start(GTK_BOX(self->widget), g->bct_epsilon, TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->bct_epsilon), "value-changed", G_CALLBACK(bct_epsilon_callback), self);

  // Kappa slider
  g->bct_kappa = dt_bauhaus_slider_new_with_range(self,
    DT_IOP_INPAINT_BCT_KAPPA_MIN, DT_IOP_INPAINT_BCT_KAPPA_MAX, DT_IOP_INPAINT_BCT_KAPPA_STEP, p->bct_kappa, 0);
  g->gw_list = g_slist_append(g->gw_list, g->bct_kappa);
  gtk_widget_set_tooltip_text(g->bct_kappa, _("Sharpness (kappa in %)"));
  dt_bauhaus_widget_set_label(g->bct_kappa, NULL, _("sharpness (%)"));
  gtk_box_pack_start(GTK_BOX(self->widget), g->bct_kappa, TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->bct_kappa), "value-changed", G_CALLBACK(bct_kappa_callback), self);

  // Sigma slider
  g->bct_sigma = dt_bauhaus_slider_new_with_range(self,
    DT_IOP_INPAINT_BCT_SIGMA_MIN, DT_IOP_INPAINT_BCT_SIGMA_MAX, DT_IOP_INPAINT_BCT_SIGMA_STEP, p->bct_sigma, 0);
  g->gw_list = g_slist_append(g->gw_list, g->bct_sigma);
  gtk_widget_set_tooltip_text(g->bct_sigma, _("Pre-smoothing (sigma)"));
  dt_bauhaus_widget_set_label(g->bct_sigma, NULL, _("pre-smoothing"));
  gtk_box_pack_start(GTK_BOX(self->widget), g->bct_sigma, TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->bct_sigma), "value-changed", G_CALLBACK(bct_sigma_callback), self);

  // Rho slider
  g->bct_rho = dt_bauhaus_slider_new_with_range(self,
    DT_IOP_INPAINT_BCT_RHO_MIN, DT_IOP_INPAINT_BCT_RHO_MAX, DT_IOP_INPAINT_BCT_RHO_STEP, p->bct_rho, 0);
  g->gw_list = g_slist_append(g->gw_list, g->bct_rho);
  gtk_widget_set_tooltip_text(g->bct_rho, _("Post-smoothing (rho)"));
  dt_bauhaus_widget_set_label(g->bct_rho, NULL, _("post-smoothing"));
  gtk_box_pack_start(GTK_BOX(self->widget), g->bct_rho, TRUE, TRUE, 0);
  g_signal_connect(G_OBJECT(g->bct_rho), "value-changed", G_CALLBACK(bct_rho_callback), self);

}

void gui_cleanup(dt_iop_module_t *self)
{
  dt_iop_inpaint_gui_data_t *g = (dt_iop_inpaint_gui_data_t *)self->gui_data;
  g_slist_free(g->gw_list);

  // nothing else necessary, gtk will clean up the labels
  free(self->gui_data);
  self->gui_data = NULL;
}


// modelines: These editor modelines have been set for all relevant files by tools/update_modelines.sh
// vim: shiftwidth=2 expandtab tabstop=2 cindent
// kate: tab-indents: off; indent-width 2; replace-tabs on; indent-mode cstyle; remove-trailing-spaces modified;
