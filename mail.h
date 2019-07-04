#include <urweb.h>

typedef struct headers *uw_Mail_headers;

extern uw_Mail_headers uw_Mail_empty;

uw_Mail_headers uw_Mail_from(uw_context, uw_Basis_string, uw_Mail_headers);
uw_Mail_headers uw_Mail_to(uw_context, uw_Basis_string, uw_Mail_headers);
uw_Mail_headers uw_Mail_cc(uw_context, uw_Basis_string, uw_Mail_headers);
uw_Mail_headers uw_Mail_bcc(uw_context, uw_Basis_string, uw_Mail_headers);
uw_Mail_headers uw_Mail_subject(uw_context, uw_Basis_string, uw_Mail_headers);

uw_unit uw_Mail_send(uw_context, uw_Basis_string server,
                     uw_Basis_bool ssl, uw_Basis_string ca,
                     uw_Basis_string user, uw_Basis_string password,
                     uw_Mail_headers, uw_Basis_string body, uw_Basis_string xbody);
