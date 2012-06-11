/* -*- Mode: C; tab-width: 4; c-basic-offset: 4 -*- */
/*
* Copyright (C) 2006, 2007 Apple Inc.
* Copyright (C) 2007 Alp Toker <alp@atoker.com>
* Copyright (C) 2011 Lukasz Slachciak
* Copyright (C) 2011 Bob Murphy
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
* notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in the
* documentation and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE COMPUTER, INC. OR
* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
* PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
* OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/* gcc -o minibrowser minibrowser.c \
 * $(pkg-config --cflags --libs webkitgtk-3.0) -Wall -O1 -g
 */

#include <glib.h>
#include <gtk/gtk.h>
#include <webkit/webkit.h>

static char *pattern = NULL;
static char *url = NULL;
static gboolean debug = FALSE;
static gboolean block_windows = FALSE;

static GOptionEntry entries[] =
{
  { "restrict", 'r', 0, G_OPTION_ARG_STRING, &pattern, "Restrict navigation to URLs following the pattern P", "P" },
  { "debug", 'd', 0, G_OPTION_ARG_NONE, &debug, "Print debug messages", NULL },
  { "url", 'u', 0, G_OPTION_ARG_STRING, &url, "URL to navigate to", NULL },
  { "block", 'b', 0, G_OPTION_ARG_NONE, &block_windows, "Block all new windows", NULL },
  { NULL }
};


static void
button_plus_clicked ( GtkWidget *widget, WebKitWebView *view )
{
       webkit_web_view_set_zoom_level (view,
                0.10+ webkit_web_view_get_zoom_level(view));
}

static void
button_minus_clicked ( GtkWidget *widget, WebKitWebView *view )
{
       webkit_web_view_set_zoom_level (view,
                -0.10+ webkit_web_view_get_zoom_level(view) );
}

static void
entry_activate_cb (GtkEntry *entry, WebKitWebView *view)
{
        webkit_web_view_load_uri(view,gtk_entry_get_text (GTK_ENTRY (entry)));
}

static gboolean
new_window_policy_decision_requested_cb(WebKitWebView* web_view,
                                        WebKitWebFrame* web_frame,
                                        WebKitNetworkRequest* request,
                                        WebKitWebNavigationAction* action,
                                        WebKitWebPolicyDecision* decision,
                                        gpointer data)
{
    g_debug("New window rejected");
    webkit_web_policy_decision_ignore(decision);
    return TRUE;
}

static gboolean
navigation_policy_decision_requested_cb(WebKitWebView* web_view,
                                        WebKitWebFrame* web_frame,
                                        WebKitNetworkRequest* request,
                                        WebKitWebNavigationAction* action,
                                        WebKitWebPolicyDecision* decision,
                                        gpointer data)
{
    gboolean flag = TRUE;
    const gchar *uri = webkit_network_request_get_uri(request);
    g_debug ("Checking %s against %s", uri, pattern);
    flag = g_pattern_match_string(g_pattern_spec_new(pattern),
                                  uri);
    if (flag)
    {
        g_debug("Accepted");
        webkit_web_policy_decision_use(decision);
    }
    else
    {
        g_debug("Rejected");
        webkit_web_policy_decision_ignore(decision);
    }
    return TRUE;
}

static void destroyWindowCb(GtkWidget* widget, GtkWidget* window)
{
    gtk_main_quit();
}

static gboolean closeWebViewCb(WebKitWebView* web_view, GtkWidget* window)
{
    gtk_widget_destroy(window);
    return TRUE;
}

static void
debug_log_handler (const gchar *log_domain,
                   GLogLevelFlags log_level,
                   const gchar *message,
                   gpointer user_data)
{
    if (debug)
        g_print ("MINIBROWSER DEBUG: %s\n", message);
}

static void
load_progress_changed_cb (WebKitWebView *web_view, GParamSpec *property, gpointer data)
{
    gdouble progress;
    progress = webkit_web_view_get_progress(web_view);

    GtkProgressBar *progress_bar = (GtkProgressBar *) data;
    gtk_progress_bar_set_fraction(progress_bar, progress);
}

static GtkWidget *
setup_toolbar (WebKitWebView *web_view)
{
    GtkWidget *button_minus, *button_plus, *toolbar, *entry_text;

    /* Create widgets */
    toolbar = gtk_box_new (GTK_ORIENTATION_HORIZONTAL,  0);

    button_plus = gtk_button_new_from_stock (GTK_STOCK_ZOOM_IN);
    button_minus = gtk_button_new_from_stock (GTK_STOCK_ZOOM_OUT);

    entry_text = gtk_entry_new();
    gtk_entry_set_placeholder_text( GTK_ENTRY(entry_text) , "Type your URL" );

    /* Add widgets to toolbar */
    gtk_box_pack_start (GTK_BOX (toolbar), entry_text, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (toolbar), button_plus, FALSE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (toolbar), button_minus, FALSE, TRUE, 0);

    /* Connect callbacks */
    g_signal_connect (button_plus, "clicked",
                      G_CALLBACK (button_plus_clicked),
                      web_view);

    g_signal_connect (button_minus, "clicked",
                      G_CALLBACK (button_minus_clicked),
                      web_view);

    g_signal_connect (entry_text, "activate",
                      G_CALLBACK (entry_activate_cb),
                      web_view);

    /* Show & return */
    gtk_widget_show (button_minus);
    gtk_widget_show (button_plus);
    gtk_widget_show (entry_text);
    gtk_widget_show (toolbar);

    return toolbar;
}

static GtkWidget *
setup_scrolled_window (WebKitWebView *web_view)
{
    /* Create a scrollable area, and put the browser instance into it */

    GtkWidget *scrolledWindow = gtk_scrolled_window_new(NULL, NULL);

    gtk_widget_set_vexpand(scrolledWindow, TRUE);
    gtk_widget_set_hexpand(scrolledWindow, TRUE);
    gtk_widget_set_halign (scrolledWindow, GTK_ALIGN_FILL);
    gtk_widget_set_valign (scrolledWindow, GTK_ALIGN_FILL);

    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolledWindow), GTK_WIDGET(web_view));

    return scrolledWindow;
}

static GtkWidget *
setup_progress_bar (WebKitWebView *web_view)
{
    /* Create progress bar */
    GtkWidget *progress_bar = gtk_progress_bar_new();
    g_object_set (progress_bar, "show-text", TRUE, NULL);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress_bar), NULL);

    /* Set up callback so that load progress ir reported */
    g_signal_connect(web_view, "notify::progress",
                     G_CALLBACK(load_progress_changed_cb),
                     (gpointer) progress_bar);

    return progress_bar;
}

static GtkWidget *
setup_main_window (WebKitWebView *web_view)
{
    GtkWidget *main_window, *grid;
    GtkWidget *toolbar, *scrolledWindow, *progress_bar;

    /* Create an 800x600 window that will contain the browser instance */
    main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(main_window), 800, 600);

    /* Setup window widgets */
    toolbar = setup_toolbar (web_view);
    scrolledWindow = setup_scrolled_window (web_view);
    progress_bar = setup_progress_bar (web_view);

    /* Layout widgets in window */
    grid = gtk_grid_new();
    gtk_grid_attach(GTK_GRID(grid), toolbar, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), scrolledWindow, 1, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), progress_bar, 1, 3, 1, 1);
    gtk_container_add(GTK_CONTAINER(main_window), grid);

    return main_window;
}

int main(int argc, char* argv[])
{
    GError *error = NULL;
    GOptionContext *context;
    WebKitWebView *web_view;
    GtkWidget *main_window;

    gtk_init(&argc, &argv);

    /* Set a custom debug handler */
    g_log_set_handler (NULL, G_LOG_LEVEL_DEBUG, debug_log_handler, NULL);

    context = g_option_context_new ("- WebKitGtk+ mini browser");
    g_option_context_add_main_entries (context, entries, "minibrowser");
    g_option_context_add_group (context, gtk_get_option_group (TRUE));
    if (!g_option_context_parse (context, &argc, &argv, &error))
    {
        g_debug ("option parsing failed: %s\n", error->message);
        return 1;
    }

    /* Create a browser instance and the main window */
    web_view = WEBKIT_WEB_VIEW(webkit_web_view_new());
    main_window = setup_main_window (web_view);

    /* Set up callbacks so that if either the main window or the
       browser instance is closed, the program will exit. */
    g_signal_connect(web_view, "close-web-view", G_CALLBACK(closeWebViewCb), main_window);
    g_signal_connect(main_window, "destroy", G_CALLBACK(destroyWindowCb), NULL);

    /* Obey command line options */
    if (pattern)
        g_signal_connect(web_view, "navigation-policy-decision-requested",
                         G_CALLBACK(navigation_policy_decision_requested_cb), NULL);

    if (block_windows)
        g_signal_connect(web_view, "new-window-policy-decision-requested",
                         G_CALLBACK(new_window_policy_decision_requested_cb), NULL);

    // Load a web page into the browser instance
    webkit_web_view_load_uri(web_view, url ? url : "http://www.webkitgtk.org/");

    // Make sure that when the browser area becomes visible, it will get mouse
    // and keyboard events
    gtk_widget_grab_focus(GTK_WIDGET(web_view));

    // Make sure the main window and all its contents are visible
    gtk_widget_show_all(main_window);

    // Run the main GTK+ event loop
    gtk_main();

    return 0;
}
