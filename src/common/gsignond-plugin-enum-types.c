
/* Generated data (by glib-mkenums) */

#include "gsignond-plugin-enum-types.h"
/* enumerations from "../../include/gsignond/gsignond-plugin-interface.h" */
#include "../../include/gsignond/gsignond-plugin-interface.h"

GType
gsignond_plugin_state_get_type (void)
{
    static GType the_type = 0;

    if (the_type == 0) {
        static const GEnumValue values[] = {
            { GSIGNOND_PLUGIN_STATE_NONE, "GSIGNOND_PLUGIN_STATE_NONE", "none" },
            { GSIGNOND_PLUGIN_STATE_RESOLVING, "GSIGNOND_PLUGIN_STATE_RESOLVING", "resolving" },
            { GSIGNOND_PLUGIN_STATE_CONNECTING, "GSIGNOND_PLUGIN_STATE_CONNECTING", "connecting" },
            { GSIGNOND_PLUGIN_STATE_SENDING_DATA, "GSIGNOND_PLUGIN_STATE_SENDING_DATA", "sending-data" },
            { GSIGNOND_PLUGIN_STATE_WAITING, "GSIGNOND_PLUGIN_STATE_WAITING", "waiting" },
            { GSIGNOND_PLUGIN_STATE_USER_PENDING, "GSIGNOND_PLUGIN_STATE_USER_PENDING", "user-pending" },
            { GSIGNOND_PLUGIN_STATE_REFRESHING, "GSIGNOND_PLUGIN_STATE_REFRESHING", "refreshing" },
            { GSIGNOND_PLUGIN_STATE_PROCESS_PENDING, "GSIGNOND_PLUGIN_STATE_PROCESS_PENDING", "process-pending" },
            { GSIGNOND_PLUGIN_STATE_STARTED, "GSIGNOND_PLUGIN_STATE_STARTED", "started" },
            { GSIGNOND_PLUGIN_STATE_CANCELING, "GSIGNOND_PLUGIN_STATE_CANCELING", "canceling" },
            { GSIGNOND_PLUGIN_STATE_DONE, "GSIGNOND_PLUGIN_STATE_DONE", "done" },
            { GSIGNOND_PLUGIN_STATE_HOLDING, "GSIGNOND_PLUGIN_STATE_HOLDING", "holding" },
            {0, NULL, NULL}
        };

        the_type = g_enum_register_static (
                        g_intern_static_string ("GSignondPluginState"),
                        values);
    }

    return the_type;
}

/* Generated data ends here */

