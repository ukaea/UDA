#include "exp2imas_ssh_gui.h"

#include <stdio.h>
#include <gtk/gtk.h>
#include <stdbool.h>
#include <string.h>

static void destroy(GtkWidget* widget, gpointer data)
{
    gtk_main_quit();
}

static void initialize_window(GtkWidget* window)
{
    gtk_window_set_title(GTK_WINDOW(window), "SSH Login");
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 200);
    g_signal_connect(window, "destroy", G_CALLBACK(destroy), NULL);
}

typedef struct Data {
    GtkWidget* window;
    GtkWidget* user_entry;
    GtkWidget* pass_entry;
    char* user_name;
    char* pass_word;
} USER_DATA;

static void ok_callback(GtkWidget *widget, gpointer *data)
{
    USER_DATA* user_data = (USER_DATA*)data;

    user_data->user_name = strdup(gtk_entry_get_text(GTK_ENTRY(user_data->user_entry)));
    user_data->pass_word = strdup(gtk_entry_get_text(GTK_ENTRY(user_data->pass_entry)));

    gtk_widget_destroy(user_data->window);
    gtk_main_quit();
}

static void close_callback(GtkWidget *widget, gpointer *data)
{
    gtk_main_quit();
}

int ssh_open_dialog(char** username, char** password)
{
    int argc = 0;
    char* argv[1];
    argv[0] = "SSH";

    gtk_init(&argc, (char***)&(argv));

    GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    initialize_window(window);

    GtkWidget* grid = gtk_grid_new();

    gtk_container_add(GTK_CONTAINER(window), grid);

    GtkWidget* user_label = gtk_label_new(" Username: ");
    gtk_grid_attach(GTK_GRID(grid), user_label, 0, 0, 1, 1);

    GtkWidget* user = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(user), 0);
    gtk_grid_attach(GTK_GRID(grid), user, 1, 0, 9, 1);

    GtkWidget* pass_label = gtk_label_new(" Password: ");
    gtk_grid_attach(GTK_GRID(grid), pass_label, 0, 1, 1, 1);

    GtkWidget* pass = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(pass), false);
    gtk_entry_set_max_length(GTK_ENTRY(pass), 0);
    gtk_grid_attach(GTK_GRID(grid), pass, 1, 1, 9, 1);

    GtkWidget* ok = gtk_button_new();
    gtk_button_set_label(GTK_BUTTON(ok), "OK");
    gtk_grid_attach(GTK_GRID(grid), ok, 8, 2, 1, 1);

    USER_DATA user_data = {0};
    user_data.window = window;
    user_data.user_entry = user;
    user_data.pass_entry = pass;

    g_signal_connect(ok, "clicked", G_CALLBACK(ok_callback), (gpointer)&user_data);

    GtkWidget* cancel = gtk_button_new();
    gtk_button_set_label(GTK_BUTTON(cancel), "Cancel");
    gtk_grid_attach(GTK_GRID(grid), cancel, 9, 2, 1, 1);

    g_signal_connect(cancel, "clicked", G_CALLBACK(close_callback), (gpointer)NULL);

    gtk_window_resize(GTK_WINDOW(window), 1, 1);
    gtk_widget_show_all(window);

    gtk_main();

    //gtk_widget_destroy(window);

    if (user_data.user_name == NULL || user_data.pass_word == NULL) {
        return -1;
    }

    *username = user_data.user_name;
    *password = user_data.pass_word;

    return 0;
}
