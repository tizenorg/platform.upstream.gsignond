
/* Generated data (by glib-mkenums) */

#include "gsignond-identity-enum-types.h"
/* enumerations from "./gsignond-identity.h" */
#include "./gsignond-identity.h"

GType
gsignond_identity_change_type_get_type (void)
{
    static GType the_type = 0;

    if (the_type == 0) {
        static const GEnumValue values[] = {
            { GSIGNOND_IDENTITY_DATA_UPDATED, "GSIGNOND_IDENTITY_DATA_UPDATED", "data-updated" },
            { GSIGNOND_IDENTITY_REMOVED, "GSIGNOND_IDENTITY_REMOVED", "removed" },
            { GSIGNOND_IDENTITY_SIGNED_OUT, "GSIGNOND_IDENTITY_SIGNED_OUT", "signed-out" },
            {0, NULL, NULL}
        };

        the_type = g_enum_register_static (
                        g_intern_static_string ("GSignondIdentityChangeType"),
                        values);
    }

    return the_type;
}

/* Generated data ends here */

