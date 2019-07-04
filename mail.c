#include <string.h>
#include <stdlib.h>
#include <curl/curl.h>

#include <urweb.h>

struct headers {
  uw_Basis_string from, to, cc, bcc, subject, user_agent;
};

typedef struct headers *uw_Mail_headers;

static uw_Basis_string copy_string(uw_Basis_string s) {
  if (s == NULL)
    return NULL;
  else
    return strdup(s);
}

static void free_string(uw_Basis_string s) {
  if (s == NULL)
    return;
  else
    free(s);
}

static uw_Mail_headers copy_headers(uw_Mail_headers h) {
  uw_Mail_headers h2 = malloc(sizeof(struct headers));
  h2->from = copy_string(h->from);
  h2->to = copy_string(h->to);
  h2->cc = copy_string(h->cc);
  h2->bcc = copy_string(h->bcc);
  h2->subject = copy_string(h->subject);
  h2->user_agent = copy_string(h->user_agent);
  return h2;
}

static void free_headers(uw_Mail_headers h) {
  free_string(h->from);
  free_string(h->to);
  free_string(h->cc);
  free_string(h->bcc);
  free_string(h->subject);
  free_string(h->user_agent);
  free(h);
}

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
  uw_Mail_headers h2 = uw_malloc(ctx, sizeof(struct headers));

  if (h)
    *h2 = *h;
  else
    memset(h2, 0, sizeof(*h2));

  if (h2->from)
    uw_error(ctx, FATAL, "Duplicate From header");

  address(ctx, s);
  h2->from = uw_strdup(ctx, s);

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
    h2->to = uw_strdup(ctx, s);

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
    h2->cc = uw_strdup(ctx, s);

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
    h2->bcc = uw_strdup(ctx, s);

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
  h2->subject = uw_strdup(ctx, s);

  return h2;
}

uw_Mail_headers uw_Mail_user_agent(uw_context ctx, uw_Basis_string s, uw_Mail_headers h) {
  uw_Mail_headers h2 = uw_malloc(ctx, sizeof(struct headers));

  if (h)
    *h2 = *h;
  else
    memset(h2, 0, sizeof(*h2));

  if (h2->user_agent)
    uw_error(ctx, FATAL, "Duplicate User-Agent header");

  header(ctx, s);
  h2->user_agent = uw_strdup(ctx, s);

  return h2;
}

typedef struct {
  uw_context ctx;
  uw_Mail_headers h;
  uw_Basis_string server, ca, user, password, body, xbody;
  uw_Basis_bool ssl;
} job;

typedef struct {
  const char *content;
  size_t length;
} upload_status;

static size_t do_upload(void *ptr, size_t size, size_t nmemb, void *userp)
{
  upload_status *upload_ctx = (upload_status *)userp;
  size *= nmemb;
  if (size > upload_ctx->length)
    size = upload_ctx->length;

  memcpy(ptr, upload_ctx->content, size);
  upload_ctx->content += size;
  upload_ctx->length -= size;
  return size;
}

// Extract e-mail address from a string that is either *just* an e-mail address or looks like "Recipient <address>".
// Note: it's destructive!
// Luckily, we only apply it to strings we are done using for other purposes (copied into buffer with e-mail contents).
static char *addrOf(char *s) {
  char *p = strchr(s, '<');
  if (p) {
    char *p2 = strchr(p+1, '>');
    if (p2) {
      *p2 = 0;
      return p+1;
    } else
      return s;
  } else
    return s;
}

static void commit(void *data) {
  job *j = data;
  char *buf, *cur;
  size_t buflen = 50;
  CURL *curl;
  CURLcode res;
  upload_status upload_ctx;
  struct curl_slist *recipients = NULL;

  buflen += 8 + strlen(j->h->from);
  if (j->h->to)
    buflen += 6 + strlen(j->h->to);
  if (j->h->cc)
    buflen += 6 + strlen(j->h->cc);
  if (j->h->bcc)
    buflen += 7 + strlen(j->h->bcc);
  if (j->h->subject)
    buflen += 11 + strlen(j->h->subject);
  if (j->h->user_agent)
    buflen += 14 + strlen(j->h->user_agent);
  buflen += strlen(j->body);
  if (j->xbody)
    buflen += 219 + strlen(j->xbody);

  cur = buf = malloc(buflen);
  if (!buf) {
    uw_set_error_message(j->ctx, "Can't allocate buffer for message contents");
    return;
  }

  if (j->h->from) {
    int written = sprintf(cur, "From: %s\r\n", j->h->from);
    if (written < 0) {
      uw_set_error_message(j->ctx, "Error writing From address");
      free(buf);
      return;
    } else
      cur += written;
  }

  if (j->h->subject) {
    int written = sprintf(cur, "Subject: %s\r\n", j->h->subject);
    if (written < 0) {
      uw_set_error_message(j->ctx, "Error writing Subject");
      free(buf);
      return;
    } else
      cur += written;
  }
  
  if (j->h->to) {
    int written = sprintf(cur, "To: %s\r\n", j->h->to);
    if (written < 0) {
      uw_set_error_message(j->ctx, "Error writing To addresses");
      free(buf);
      return;
    } else
      cur += written;
  }

  if (j->h->cc) {
    int written = sprintf(cur, "Cc: %s\r\n", j->h->cc);
    if (written < 0) {
      uw_set_error_message(j->ctx, "Error writing Cc addresses");
      free(buf);
      return;
    } else
      cur += written;
  }

  if (j->h->bcc) {
    int written = sprintf(cur, "Bcc: %s\r\n", j->h->bcc);
    if (written < 0) {
      uw_set_error_message(j->ctx, "Error writing Bcc addresses");
      free(buf);
      return;
    } else
      cur += written;
  }

  if (j->h->user_agent) {
    int written = sprintf(cur, "User-Agent: %s\r\n", j->h->user_agent);
    if (written < 0) {
      uw_set_error_message(j->ctx, "Error writing User-Agent");
      free(buf);
      return;
    } else
      cur += written;
  }

  if (j->xbody) {
    int written;
    char separator[11];
    separator[sizeof(separator)-1] = 0;

    do {
      int i;

      for (i = 0; i < sizeof(separator)-1; ++i)
        separator[i] = 'A' + (rand() % 26);
    } while (strstr(j->body, separator) || strstr(j->xbody, separator));

    written = sprintf(cur, "MIME-Version: 1.0\r\n"
                      "Content-Type: multipart/alternative; boundary=\"%s\"\r\n"
                      "\r\n"
                      "--%s\r\n"
                      "Content-Type: text/plain; charset=utf-8\r\n"
                      "\r\n"
                      "%s\r\n"
                      "--%s\r\n"
                      "Content-Type: text/html; charset=utf-8\r\n"
                      "\r\n"
                      "%s\r\n"
                      "--%s--",
                      separator, separator, j->body, separator, j->xbody, separator);

    if (written < 0) {
      uw_set_error_message(j->ctx, "Error writing bodies, including HTML");
      free(buf);
      return;
    }
  } else {
    int written = sprintf(cur, "Content-Type: text/plain; charset=utf-8\r\n"
                          "\r\n"
                          "%s",
                          j->body);

    if (written < 0) {
      uw_set_error_message(j->ctx, "Error writing body");
      free(buf);
      return;
    }
  }

  upload_ctx.content = buf;
  upload_ctx.length = strlen(buf);

  if (j->h->to) {
    char *saveptr, *addr = strtok_r(j->h->to, ",", &saveptr);
    do {
      recipients = curl_slist_append(recipients, addrOf(addr));
    } while ((addr = strtok_r(NULL, ",", &saveptr)));
  }

  if (j->h->cc) {
    char *saveptr, *addr = strtok_r(j->h->cc, ",", &saveptr);
    do {
      recipients = curl_slist_append(recipients, addrOf(addr));
    } while ((addr = strtok_r(NULL, ",", &saveptr)));
  }

  if (j->h->bcc) {
    char *saveptr, *addr = strtok_r(j->h->bcc, ",", &saveptr);
    do {
      recipients = curl_slist_append(recipients, addrOf(addr));
    } while ((addr = strtok_r(NULL, ",", &saveptr)));
  }
  
  curl = curl_easy_init();
  if (!curl) {
    free(buf);
    uw_set_error_message(j->ctx, "Can't create curl object");
    return;
  }

  curl_easy_setopt(curl, CURLOPT_USERNAME, j->user);
  curl_easy_setopt(curl, CURLOPT_PASSWORD, j->password);
  curl_easy_setopt(curl, CURLOPT_URL, j->server);

  if (j->ssl) {
    curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
    if (j->ca) {
      curl_easy_setopt(curl, CURLOPT_CAINFO, j->ca);
    } else {
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    }
  }

  curl_easy_setopt(curl, CURLOPT_MAIL_FROM, addrOf(j->h->from));
  curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
  curl_easy_setopt(curl, CURLOPT_READFUNCTION, do_upload);
  curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);
  curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
  //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

  res = curl_easy_perform(curl);

  if (res != CURLE_OK)
    uw_set_error_message(j->ctx, "Curl error sending e-mail: %s", curl_easy_strerror(res));

  curl_slist_free_all(recipients);
  curl_easy_cleanup(curl);
  free(buf);
}

static void free_job(void *p, int will_retry) {
  job *j = p;

  free_headers(j->h);
  free_string(j->server);
  free_string(j->ca);
  free_string(j->user);
  free_string(j->password);
  free_string(j->body);
  free_string(j->xbody);
  free(j);
}

uw_unit uw_Mail_send(uw_context ctx, uw_Basis_string server,
                     uw_Basis_bool ssl, uw_Basis_string ca,
                     uw_Basis_string user, uw_Basis_string password,
                     uw_Mail_headers h, uw_Basis_string body, uw_Basis_string xbody) {
  job *j;

  if (!h || !h->from)
    uw_error(ctx, FATAL, "No From address set for e-mail message");

  if (!h->to && !h->cc && !h->bcc)
    uw_error(ctx, FATAL, "No recipients specified for e-mail message");

  j = malloc(sizeof(job));

  j->ctx = ctx;
  j->h = copy_headers(h);
  j->server = copy_string(server);
  j->ssl = ssl;
  j->ca = copy_string(ca);
  j->user = copy_string(user);
  j->password = copy_string(password);
  j->body = copy_string(body);
  j->xbody = copy_string(xbody);

  uw_register_transactional(ctx, j, commit, NULL, free_job);

  return uw_unit_v;
}
