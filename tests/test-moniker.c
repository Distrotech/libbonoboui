/*
 * moniker-test.c: Test program for monikers resolving to various interfaces.
 *
 * Author:
 *   Vladimir Vukicevic (vladimir@helixcode.com)
 *
 * Based on moniker-control-test.c, by Joe Shaw (joe@helixcode.com)
 *
 * Copyright (C) 2000, Helix Code, Inc.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <gnome.h>
#include <liboaf/liboaf.h>
#include <bonobo.h>


static void display_as_interface (const char *moniker, CORBA_Environment *ev);
static void display_as_stream (const char *moniker, CORBA_Environment *ev);
static void display_as_storage_file_list (const char *moniker, CORBA_Environment *ev);
static void display_as_html (const char *moniker, CORBA_Environment *ev);
static void display_as_control (const char *moniker, CORBA_Environment *ev);

typedef enum {
    AS_NONE = 0,
    AS_INTERFACE,
    AS_STREAM,
    AS_STORAGE_FILE_LIST,
    AS_HTML,
    AS_CONTROL
} MonikerTestDisplayAs;

typedef void (*MonikerDisplayFunction) (const char *moniker, CORBA_Environment *ev);

typedef struct {
    MonikerTestDisplayAs disp_as;
    MonikerDisplayFunction func;
} MonikerTestDisplayers;

MonikerTestDisplayers displayers[] = {
    {AS_INTERFACE, display_as_interface},
    {AS_STREAM, display_as_stream},
    {AS_STORAGE_FILE_LIST, display_as_storage_file_list},
    {AS_HTML, display_as_html},
    {AS_CONTROL, display_as_control},
    {0}
};

typedef struct {
    gchar *requested_interface;
    gchar *requested_moniker;
    MonikerTestDisplayAs display_as;
    gchar *moniker;

    int ps, pr, pc, ph;
} MonikerTestOptions;

MonikerTestOptions global_mto = { 0 };

struct poptOption moniker_test_options[] = {
    {"interface", 'i', POPT_ARG_STRING, &global_mto.requested_interface, 'i', "requested interface", "interface"},
    {"stream", 's', 0, &global_mto.ps, 's', "request Bonobo/Stream" },
    {"storage", 'r', 0, &global_mto.pr, 'r', "request Bonobo/Storage" },
    {"control", 'c', 0, &global_mto.pc, 'c', "request Bonobo/Control" },
    {"html", 'h', 0, &global_mto.ph, 'h', "request Bonobo/Stream and display as HTML" },
    {NULL, 0, 0, NULL, 0}
};


static void do_moniker_magic (void);


int
main (int argc, char **argv)
{
    CORBA_ORB orb;

    poptContext ctx;
    int i;

    gnome_init_with_popt_table ("moniker-test", "0.0", argc, argv, moniker_test_options, 0, &ctx);
    if ((orb = oaf_init (argc, argv)) == NULL)
        g_error ("Cannot init oaf");
    if (bonobo_init (orb, NULL, NULL) == FALSE)
        g_error ("Cannot init bonobo");


    /* Do the nasty popt stuff */
    while ((i = poptGetNextOpt (ctx)) > 0)
        ;

    if (global_mto.ps + global_mto.pr + global_mto.ph +
        global_mto.pc > 1)
    {
        poptPrintUsage (ctx, stderr, 0);
        return 1;
    }

    if (global_mto.requested_interface)
        global_mto.display_as = AS_INTERFACE;
    else if (global_mto.ps)
        global_mto.display_as = AS_STREAM;
    else if (global_mto.pr)
        global_mto.display_as = AS_STORAGE_FILE_LIST;
    else if (global_mto.ph)
        global_mto.display_as = AS_HTML;
    else if (global_mto.pc)
        global_mto.display_as = AS_CONTROL;
    else {
        poptPrintUsage (ctx, stderr, 0);
        return 1;
    }


    poptSetOtherOptionHelp (ctx, "<moniker>");
    global_mto.requested_moniker = g_strdup (poptGetArg (ctx));
    if (!global_mto.requested_moniker) {
        poptPrintUsage (ctx, stderr, 0);
        return 1;
    }

    poptFreeContext (ctx);
    /* done with nasty popt stuff */

    fprintf (stderr, "Resolving moniker '%s' as ", global_mto.requested_moniker);
    switch (global_mto.display_as) {
        case AS_INTERFACE: fprintf (stderr, global_mto.requested_interface); break;
        case AS_STREAM: fprintf (stderr, "IDL:Bonobo/Stream:1.0"); break;
        case AS_STORAGE_FILE_LIST: fprintf (stderr, "IDL:Bonobo/Storage:1.0"); break;
        case AS_HTML: fprintf (stderr, "IDL:Bonobo/Control:1.0 (html)"); break;
        case AS_CONTROL: fprintf (stderr, "IDL:Bonobo/Control:1.0"); break;
        default: fprintf (stderr, "???"); break;
    }
    fprintf (stderr, "\n");

    do_moniker_magic ();
    return 0;
}


static void
do_moniker_magic (void)
{
    CORBA_Environment ev;
    MonikerTestDisplayers *iter = displayers;
    CORBA_exception_init (&ev);

    while (iter->disp_as) {
        if (iter->disp_as == global_mto.display_as) {
            (*iter->func) (global_mto.requested_moniker, &ev);
            CORBA_exception_free (&ev);
            return;
        }
        iter++;
    }

    g_error ("Didn't find handler!");
}

static void
display_as_interface (const char *moniker, CORBA_Environment *ev)
{
    g_error ("Not implemented");
}

static void
display_as_stream (const char *moniker, CORBA_Environment *ev)
{
    g_error ("Not implemented");
}

static void
display_as_storage_file_list (const char *moniker, CORBA_Environment *ev)
{
    Bonobo_Storage the_storage;
    Bonobo_Storage_DirectoryList *storage_contents;
    Bonobo_StorageInfo *bsi;
    int i;

    the_storage = bonobo_get_object (moniker, "IDL:Bonobo/Storage:1.0", ev);
    if (!the_storage) {
        g_error ("Couldn't get Bonobo/Storage interface\n");
    }

    storage_contents = Bonobo_Storage_listContents (the_storage,
                                                    "",
                                                    Bonobo_FIELD_CONTENT_TYPE |
                                                    Bonobo_FIELD_SIZE |
                                                    Bonobo_FIELD_TYPE,
                                                    ev);
    if (!storage_contents || (storage_contents && !storage_contents->_buffer)) {
        g_error ("got NULL storage_contents!\n");
    }

    bsi = storage_contents->_buffer;
    printf ("Storage List\n");
    printf ("------------\n");
    for (i = 0; i < storage_contents->_length; i++) {
        printf ("% 3d: %20s % 10d %c %15s\n",
                i,
                bsi[i].name,
                bsi[i].size,
                bsi[i].type == Bonobo_STORAGE_TYPE_DIRECTORY ? 'd' : 'r',
                bsi[i].content_type);
    }

    /* how do I free the silly dirlist? */
}

static void
display_as_html (const char *moniker, CORBA_Environment *ev)
{
    g_error ("Not implemented");
}

static void
display_as_control (const char *moniker, CORBA_Environment *ev)
{
}

