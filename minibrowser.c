
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

static gboolean
navigation_policy_decision_requested_cb(WebKitWebView* web_view,
                                        WebKitWebFrame* web_frame,
                                        WebKitNetworkRequest* request,
                                        WebKitWebNavigationAction* action,
                                        WebKitWebPolicyDecision* decision,
                                        gpointer data);

static void destroyWindowCb(GtkWidget* widget, GtkWidget* window);
static gboolean closeWebViewCb(WebKitWebView* webView, GtkWidget* window);

int main(int argc, char* argv[])
{
    // Initialize GTK+
    gtk_init(&argc, &argv);

    // Create an 800x600 window that will contain the browser instance
    GtkWidget *main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(main_window), 800, 600);

    // Create a browser instance
    WebKitWebView *webView = WEBKIT_WEB_VIEW(webkit_web_view_new());

    // Create a scrollable area, and put the browser instance into it
    GtkWidget *scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow),
            GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolledWindow), GTK_WIDGET(webView));

    // Set up callbacks so that if either the main window or the browser instance is
    // closed, the program will exit
    g_signal_connect(main_window, "destroy", G_CALLBACK(destroyWindowCb), NULL);
    g_signal_connect(webView, "close-web-view", G_CALLBACK(closeWebViewCb), main_window);
    g_signal_connect(webView, "navigation-policy-decision-requested",
                     G_CALLBACK(navigation_policy_decision_requested_cb), NULL);

    // Put the scrollable area into the main window
    gtk_container_add(GTK_CONTAINER(main_window), scrolledWindow);

    // Load a web page into the browser instance
    webkit_web_view_load_uri(webView, "http://www.webkitgtk.org/");

    // Make sure that when the browser area becomes visible, it will get mouse
    // and keyboard events
    gtk_widget_grab_focus(GTK_WIDGET(webView));

    // Make sure the main window and all its contents are visible
    gtk_widget_show_all(main_window);

    // Run the main GTK+ event loop
    gtk_main();

    return 0;
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
    const gchar *pattern = "http://www.webkitgtk.org/*";
    const gchar *uri = webkit_network_request_get_uri(request);
    g_message("Checking %s against %s", uri, pattern);
    flag = g_pattern_match_string(g_pattern_spec_new(pattern),
                                  uri);
    if (flag)
    {
        g_message("Accepted");
        webkit_web_policy_decision_use(decision);
    }
    else
    {
        g_message("Rejected");
        webkit_web_policy_decision_ignore(decision);
    }
    return TRUE;
}

static void destroyWindowCb(GtkWidget* widget, GtkWidget* window)
{
    gtk_main_quit();
}

static gboolean closeWebViewCb(WebKitWebView* webView, GtkWidget* window)
{
    gtk_widget_destroy(window);
    return TRUE;
}

