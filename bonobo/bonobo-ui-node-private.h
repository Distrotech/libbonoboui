#ifndef _BONOBO_UI_NODE_PRIVATE_H_
#define _BONOBO_UI_NODE_PRIVATE_H_

/* All this for xmlChar, xmlStrdup !? */
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>

#include <bonobo/bonobo-ui-node.h>

struct _BonoboUINode {
	/* Tree management */
	BonoboUINode *parent;
	BonoboUINode *children;
	BonoboUINode *prev;
	BonoboUINode *next;

	/* The useful bits */
	GQuark        name_id;
	xmlChar      *content;
	GArray       *attrs;
	gpointer      user_data;
};

typedef struct {
	GQuark        id;
	xmlChar      *value;
} BonoboUIAttr;

void        bonobo_ui_node_set_attr_by_id (BonoboUINode *node,
					   GQuark        id,
					   const char   *value);
const char *bonobo_ui_node_get_attr_by_id (BonoboUINode *node,
					   GQuark        id);
const char *bonobo_ui_node_peek_content   (BonoboUINode *node);
gboolean    bonobo_ui_node_has_name_by_id (BonoboUINode *node,
					   GQuark        id);
void        bonobo_ui_node_add_after      (BonoboUINode *before,
					   BonoboUINode *new_after);
void        bonobo_ui_node_move_children  (BonoboUINode *from,
					   BonoboUINode *to);

#endif /* _BONOBO_UI_NODE_PRIVATE_H_ */
