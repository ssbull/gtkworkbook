/* @author: John `jb Bellone <jvb4@njit.edu> */
#ifndef H_SHEET
#define H_SHEET

#include <shared.h>
#include <gtk/gtk.h>
#include <gtkextra/gtksheet.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GEOMETRY_FILE_VERSION 0x000001

typedef struct _Sheet Sheet;

#include "workbook.h"
#include "cell.h"

  /*
    @description: This object abstracts away all of the calls to the native
    GtkSheet methods. It is meant to be used with the Cell object(s).
    
    Please keep the following in mind:
    a. All calls to gtk_* methods should be performed in here and not inside of
       a cell object. 
    b. All calls to gtk_* methods should be performed inside of a lock, e.g. 
       you should always use gdk_threads_enter and gtk_threads_exit. Any calls
       outside of the window (main) thread will usually result in problems if
       you do not get a mutex. 
  */
  struct _Sheet
  {
    /* Members */
    Sheet * next;
    Sheet * prev;
    gchar * name;
    Workbook * workbook;
    GtkWidget * gtk_label;
    GtkWidget * gtk_sheet;
    GtkWidget * gtk_box;
    gint page;
    gint attention;
    gint notices;
    gboolean has_focus;

    /* Methods */
    void (*destroy) (Sheet *);
    void (*set_attention) (Sheet *, gint);
    void (*apply_range) (Sheet *, 
			 const GtkSheetRange *, 
			 const CellAttributes *);
    void (*apply_array) (Sheet *, const Cell **, gint);
    void (*apply_cell) (Sheet *, const Cell *);

    void (*set_cell) (Sheet *, gint, gint, const gchar *);  
    void (*range_set_background) (Sheet *, 
				  const GtkSheetRange *, 
				  const gchar *);
    void (*range_set_foreground) (Sheet *, 
				  const GtkSheetRange *,
				  const gchar *);
    gboolean (*save) (Sheet *, const gchar *);
    gboolean (*load) (Sheet *, const gchar *);
  };

  /* sheet.c */
  Sheet *sheet_new (Workbook *, const gchar *, gint, gint);

#ifdef __cplusplus
}
#endif
#endif /*H_SHEET*/