#include <urweb.h>

typedef struct headers *uw_Email_headers;

extern uw_Email_headers uw_Email_empty;

uw_Email_headers uw_Email_from(uw_context, uw_Basis_string, uw_Email_headers);
uw_Email_headers uw_Email_to(uw_context, uw_Basis_string, uw_Email_headers);
uw_Email_headers uw_Email_cc(uw_context, uw_Basis_string, uw_Email_headers);
uw_Email_headers uw_Email_bcc(uw_context, uw_Basis_string, uw_Email_headers);
uw_Email_headers uw_Email_subject(uw_context, uw_Basis_string, uw_Email_headers);

uw_unit uw_Email_send(uw_context, uw_Basis_string server,
                     uw_Basis_bool ssl, uw_Basis_string ca,
                     uw_Basis_string user, uw_Basis_string password,
                     uw_Email_headers, uw_Basis_string body, uw_Basis_string xbody);
