#include <gnome.h>

#include <libgnomeprint/gnome-print.h>
#include <libgnomeprint/gnome-print-job.h>
#include <libgnomeprint/gnome-print-job-preview.h>

#include "container-print.h"
#include "component.h"

void
sample_app_print_preview (SampleApp *app)
{
	GList *l;
	double ypos = 0.0;
	GnomePrintMaster *pm;
	GnomePrintContext *ctx;
	GnomePrintMasterPreview *pv;

	pm = gnome_print_job_new (NULL);
	ctx = gnome_print_job_get_context (pm);

	for (l = app->components; l; l = l->next) {
		BonoboClientSite *site = l->data;

		object_print (bonobo_client_site_get_embeddable (site),
			      ctx, 0.0, ypos, 320.0, 320.0);
		ypos += 320.0;
	}

	gnome_print_showpage (ctx);
	gnome_print_context_close (ctx);
	gnome_print_job_close (pm);

	pv = gnome_print_job_preview_new (pm, "Component demo");
	gtk_widget_show  (GTK_WIDGET (pv));
	g_object_unref (G_OBJECT (pm));
}
