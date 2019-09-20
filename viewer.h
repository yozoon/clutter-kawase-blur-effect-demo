/*
 * Copyright/Licensing information.
 */

/* inclusion guard */
#ifndef __VIEWER_FILE_H__
#define __VIEWER_FILE_H__

#include <glib-object.h>
#include <clutter/clutter.h>
#include <cogl/cogl.h>
/*
 * Potentially, include other headers on which this header depends.
 */

G_BEGIN_DECLS

typedef struct _ViewerFile  ViewerFile;
typedef struct _ViewerFileClass  ViewerFileClass;

/*
 * Type declaration.
 */
#define VIEWER_TYPE_FILE viewer_file_get_type ()
//G_DECLARE_FINAL_TYPE (ViewerFile, viewer_file, VIEWER, FILE, GObject)

/*
 * Method definitions.
 */
ViewerFile *viewer_file_new (void);

G_END_DECLS

#endif /* __VIEWER_FILE_H__ */