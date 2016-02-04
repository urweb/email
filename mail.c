#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <urweb.h>

struct headers {
  uw_Basis_string from, to, cc, bcc, subject;
};

typedef struct headers *uw_Mail_headers;

uw_Mail_headers uw_Mail_empty = NULL;

static void header(uw_context ctx, uw_Basis_string s) {
  if (strlen(s) > 100)
    uw_error(ctx, FATAL, "Header value too long");

  for (; *s; ++s)
    if (*s == '\r' || *s == '\n')
      uw_error(ctx, FATAL, "Header value contains newline");
}

static void address(uw_context ctx, uw_Basis_string s) {
  header(ctx, s);

  if (strchr(s, ','))
    uw_error(ctx, FATAL, "E-mail address contains comma");
}

uw_Mail_headers uw_Mail_from(uw_context ctx, uw_Basis_string s, uw_Mail_headers h) {
  // char **allowed = uw_get_global(ctx, "mail_from");
  // Might add this policy checking (or some expanded version of it) back later.
  uw_Mail_headers h2 = uw_malloc(ctx, sizeof(struct headers));

  if (h)
    *h2 = *h;
  else
    memset(h2, 0, sizeof(*h2));

  if (h2->from)
    uw_error(ctx, FATAL, "Duplicate From header");

  /*
  if (!allowed)
    uw_error(ctx, FATAL, "No From address whitelist has been set.  Perhaps you are not authorized to send e-mail.");

  if (!(allowed[0] && !strcmp(allowed[0], "*"))) {
    for (; *allowed; ++allowed)
      if (!strcmp(*allowed, s))
        goto ok;

    uw_error(ctx, FATAL, "From address is not in whitelist");
  }

 ok:
  */
  address(ctx, s);
  h2->from = s;

  return h2;
}

uw_Mail_headers uw_Mail_to(uw_context ctx, uw_Basis_string s, uw_Mail_headers h) {
  uw_Mail_headers h2 = uw_malloc(ctx, sizeof(struct headers));
  if (h)
    *h2 = *h;
  else
    memset(h2, 0, sizeof(*h2));

  address(ctx, s);
  if (h2->to) {
    uw_Basis_string all = uw_malloc(ctx, strlen(h2->to) + 2 + strlen(s));
    sprintf(all, "%s,%s", h2->to, s);
    h2->to = all;
  } else
    h2->to = s;

  return h2;
}

uw_Mail_headers uw_Mail_cc(uw_context ctx, uw_Basis_string s, uw_Mail_headers h) {
  uw_Mail_headers h2 = uw_malloc(ctx, sizeof(struct headers));
  if (h)
    *h2 = *h;
  else
    memset(h2, 0, sizeof(*h2));

  address(ctx, s);
  if (h2->cc) {
    uw_Basis_string all = uw_malloc(ctx, strlen(h2->cc) + 2 + strlen(s));
    sprintf(all, "%s,%s", h2->cc, s);
    h2->cc = all;
  } else
    h2->cc = s;

  return h2;
}

uw_Mail_headers uw_Mail_bcc(uw_context ctx, uw_Basis_string s, uw_Mail_headers h) {
  uw_Mail_headers h2 = uw_malloc(ctx, sizeof(struct headers));
  if (h)
    *h2 = *h;
  else
    memset(h2, 0, sizeof(*h2));

  address(ctx, s);
  if (h2->bcc) {
    uw_Basis_string all = uw_malloc(ctx, strlen(h2->bcc) + 2 + strlen(s));
    sprintf(all, "%s,%s", h2->bcc, s);
    h2->bcc = all;
  } else
    h2->bcc = s;

  return h2;
}

uw_Mail_headers uw_Mail_subject(uw_context ctx, uw_Basis_string s, uw_Mail_headers h) {
  uw_Mail_headers h2 = uw_malloc(ctx, sizeof(struct headers));

  if (h)
    *h2 = *h;
  else
    memset(h2, 0, sizeof(*h2));

  if (h2->subject)
    uw_error(ctx, FATAL, "Duplicate Subject header");

  header(ctx, s);
  h2->subject = s;

  return h2;
}

typedef struct {
  uw_context ctx;
  uw_Mail_headers h;
  uw_Basis_string body, xbody;
} job;

#define BUFLEN (1024*1024)

static int smtp_read(uw_context ctx, int sock, char *buf, ssize_t *pos) {
  char *s;

  while (1) {
    ssize_t recvd;

    buf[*pos] = 0;
  
    if ((s = strchr(buf, '\n'))) {
      int n;

      *s = 0;

      if (sscanf(buf, "%d ", &n) != 1) {
        close(sock);
        uw_set_error_message(ctx, "Mail server response does not begin with a code.");
        return 0;
      }

      *pos -= s - buf + 1;
      memmove(buf, s+1, *pos);

      return n;
    }

    recvd = recv(sock, buf + *pos, BUFLEN - *pos - 1, 0);

    if (recvd == 0) {
      close(sock);
      uw_set_error_message(ctx, "Mail server response ends unexpectedly.");
      return 0;
    } else if (recvd < 0) {
      close(sock);
      uw_set_error_message(ctx, "Error reading mail server response.");
      return 0;
    }

    *pos += recvd;
  }
}

static int really_string(int sock, const char *s) {
  fprintf(stderr, "MAIL: %s\n", s);
  return uw_really_send(sock, s, strlen(s));
}

static int sendAddrs(const char *kind, uw_context ctx, int sock, char *s, char *buf, ssize_t *pos) {
  char *p;
  char out[BUFLEN];

  if (!s)
    return 0;

  for (p = strchr(s, ','); p; p = strchr(p+1, ',')) {
    *p = 0;

    snprintf(out, sizeof(out), "RCPT TO:%s\r\n", s);
    out[sizeof(out)-1] = 0;
    *p = ',';

    if (really_string(sock, out) < 0) {
      close(sock);
      uw_set_error_message(ctx, "Error sending RCPT TO for %s", kind);
      return 1;
    }

    if (smtp_read(ctx, sock, buf, pos) != 250) {
      close(sock);
      uw_set_error_message(ctx, "Mail server doesn't respond to %s RCPT TO with code 250.", kind);
      return 1;
    }

    s = p+1;
  }

  if (*s) {
    snprintf(out, sizeof(out), "RCPT TO:%s\r\n", s);
    out[sizeof(out)-1] = 0;

    if (really_string(sock, out) < 0) {
      close(sock);
      uw_set_error_message(ctx, "Error sending RCPT TO for %s", kind);
      return 1;
    }

    if (smtp_read(ctx, sock, buf, pos) != 250) {
      close(sock);
      uw_set_error_message(ctx, "Mail server doesn't respond to %s RCPT TO with code 250.", kind);
      return 1;
    }
  }

  return 0;
}

static void commit(void *data) {
  job *j = data;
  int sock;
  struct sockaddr_in my_addr;
  char buf[BUFLEN], out[BUFLEN];
  ssize_t pos = 0;
  char *s;

  if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
    uw_set_error_message(j->ctx, "Can't create socket for mail server connection");
    return;
  }

  my_addr.sin_family = AF_INET;
  my_addr.sin_port = htons(25);
  my_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  memset(my_addr.sin_zero, 0, sizeof my_addr.sin_zero);

  if (connect(sock, (struct sockaddr *)&my_addr, sizeof my_addr) < 0) {
    close(sock);
    uw_set_error_message(j->ctx, "Error connecting to mail server");
    return;
  }

  if (smtp_read(j->ctx, sock, buf, &pos) != 220) {
    close(sock);
    uw_set_error_message(j->ctx, "Mail server doesn't greet with code 220.");
    return;
  }

  if (really_string(sock, "HELO localhost\r\n") < 0) {
    close(sock);
    uw_set_error_message(j->ctx, "Error sending HELO");
    return;
  }

  if (smtp_read(j->ctx, sock, buf, &pos) != 250) {
    close(sock);
    uw_set_error_message(j->ctx, "Mail server doesn't respond to HELO with code 250.");
    return;
  }

  snprintf(out, sizeof(out), "MAIL FROM:%s\r\n", j->h->from);
  out[sizeof(out)-1] = 0;

  if (really_string(sock, out) < 0) {
    close(sock);
    uw_set_error_message(j->ctx, "Error sending MAIL FROM");
    return;
  }

  if (smtp_read(j->ctx, sock, buf, &pos) != 250) {
    close(sock);
    uw_set_error_message(j->ctx, "Mail server doesn't respond to MAIL FROM with code 250.");
    return;
  }

  if (sendAddrs("To", j->ctx, sock, j->h->to, buf, &pos)) return;
  if (sendAddrs("Cc", j->ctx, sock, j->h->cc, buf, &pos)) return;
  if (sendAddrs("Bcc", j->ctx, sock, j->h->bcc, buf, &pos)) return;

  if (really_string(sock, "DATA\r\n") < 0) {
    close(sock);
    uw_set_error_message(j->ctx, "Error sending DATA");
    return;
  }

  if (smtp_read(j->ctx, sock, buf, &pos) != 354) {
    close(sock);
    uw_set_error_message(j->ctx, "Mail server doesn't respond to DATA with code 354.");
    return;
  }

  snprintf(out, sizeof(out), "From: %s\r\n", j->h->from);
  out[sizeof(out)-1] = 0;

  if (really_string(sock, out) < 0) {
    close(sock);
    uw_set_error_message(j->ctx, "Error sending From");
    return;
  }

  if (j->h->subject) {
    snprintf(out, sizeof(out), "Subject: %s\r\n", j->h->subject);
    out[sizeof(out)-1] = 0;

    if (really_string(sock, out) < 0) {
      close(sock);
      uw_set_error_message(j->ctx, "Error sending Subject");
      return;
    }
  }

  if (j->h->to) {
    snprintf(out, sizeof(out), "To: %s\r\n", j->h->to);
    out[sizeof(out)-1] = 0;

    if (really_string(sock, out) < 0) {
      close(sock);
      uw_set_error_message(j->ctx, "Error sending To");
      return;
    }
  }

  if (j->h->cc) {
    snprintf(out, sizeof(out), "Cc: %s\r\n", j->h->cc);
    out[sizeof(out)-1] = 0;

    if (really_string(sock, out) < 0) {
      close(sock);
      uw_set_error_message(j->ctx, "Error sending Cc");
      return;
    }
  }

  if ((s = uw_get_global(j->ctx, "extra_mail_headers"))) {
    if (really_string(sock, s) < 0) {
      close(sock);
      uw_set_error_message(j->ctx, "Error sending extra headers");
      return;
    }
  }

  if (j->xbody) {
    char separator[11];
    separator[sizeof(separator)-1] = 0;

    do {
      int i;

      for (i = 0; i < sizeof(separator)-1; ++i)
        separator[i] = 'A' + (rand() % 26);
    } while (strstr(j->body, separator) || strstr(j->xbody, separator));

    snprintf(out, sizeof(out), "MIME-Version: 1.0\r\n"
             "Content-Type: multipart/alternative; boundary=\"%s\"\r\n"
             "\r\n"
             "--%s\r\n"
             "Content-Type: text/plain; charset=utf-8\r\n"
             "\r\n",
             separator, separator);
    out[sizeof(out)-1] = 0;

    if (really_string(sock, out) < 0) {
      close(sock);
      uw_set_error_message(j->ctx, "Error sending multipart beginning");
      return;
    }

    if (really_string(sock, j->body) < 0) {
      close(sock);
      uw_set_error_message(j->ctx, "Error sending message text body");
      return;
    }

    snprintf(out, sizeof(out), "\r\n"
             "--%s\r\n"
             "Content-Type: text/html; charset=utf-8\r\n"
             "\r\n",
             separator);
    out[sizeof(out)-1] = 0;

    if (really_string(sock, out) < 0) {
      close(sock);
      uw_set_error_message(j->ctx, "Error sending multipart middle");
      return;
    }

    if (really_string(sock, j->xbody) < 0) {
      close(sock);
      uw_set_error_message(j->ctx, "Error sending message HTML body");
      return;
    }

    snprintf(out, sizeof(out), "\r\n"
             "--%s--",
             separator);
    out[sizeof(out)-1] = 0;

    if (really_string(sock, out) < 0) {
      close(sock);
      uw_set_error_message(j->ctx, "Error sending multipart end");
      return;
    }
  } else {
    if (really_string(sock, "Content-Type: text/plain; charset=utf-8\r\n\r\n") < 0) {
      close(sock);
      uw_set_error_message(j->ctx, "Error sending text Content-Type");
      return;
    }

    if (really_string(sock, j->body) < 0) {
      close(sock);
      uw_set_error_message(j->ctx, "Error sending message body");
      return;
    }
  }

  if (really_string(sock, "\r\n.\r\n") < 0) {
    close(sock);
    uw_set_error_message(j->ctx, "Error sending message terminator");
    return;
  }

  if (smtp_read(j->ctx, sock, buf, &pos) != 250) {
    close(sock);
    uw_set_error_message(j->ctx, "Mail server doesn't respond to end of message with code 250.");
    return;
  }

  if (really_string(sock, "QUIT\r\n") < 0) {
    close(sock);
    uw_set_error_message(j->ctx, "Error sending QUIT");
    return;
  }

  if (smtp_read(j->ctx, sock, buf, &pos) != 221) {
    close(sock);
    uw_set_error_message(j->ctx, "Mail server doesn't respond to QUIT with code 221.");
    return;
  }

  close(sock);
}

uw_unit uw_Mail_send(uw_context ctx, uw_Mail_headers h, uw_Basis_string body, uw_Basis_string xbody) {
  job *j;
  char *s;

  if (!h || !h->from)
    uw_error(ctx, FATAL, "No From address set for e-mail message");

  if (!h->to && !h->cc && !h->bcc)
    uw_error(ctx, FATAL, "No recipients specified for e-mail message");

  for (s = strchr(body, '.'); s; s = strchr(s+1, '.'))
    if ((s[1] == '\n' || s[1] == '\r')
        && (s <= body || s[-1] == '\n' || s[-1] == '\r'))
      uw_error(ctx, FATAL, "Message body contains a line with just a period");

  if (xbody) {
    for (s = strchr(xbody, '.'); s; s = strchr(s+1, '.'))
      if ((s[1] == '\n' || s[1] == '\r')
          && (s <= xbody || s[-1] == '\n' || s[-1] == '\r'))
        uw_error(ctx, FATAL, "HTML message body contains a line with just a period");
  }

  j = uw_malloc(ctx, sizeof(job));

  j->ctx = ctx;
  j->h = h;
  j->body = body;
  j->xbody = xbody;

  uw_register_transactional(ctx, j, commit, NULL, NULL);

  return uw_unit_v;
}
