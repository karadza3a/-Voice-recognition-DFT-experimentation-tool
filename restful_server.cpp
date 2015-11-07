/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#include "mongoose.h"
#include "WavFile.hpp"
#include <cmath>

// ------------- Globals ------------------------------

static const char *s_http_port = "8000";
static struct mg_serve_http_opts s_http_server_opts;

static double window_width;
static double window_offset;
static int window_function = 0; // None, hamm, hann
static WavFile *wf;

// ------------- Globals end --------------------------

static void hamming(double *data, int n) {
  for (int i = 0; i < n; i++) {
    double multiplier = 0.54 - 0.46 * cos(2 * M_PI * i / (n - 1));
    data[i] = multiplier * data[i];
  }
}

static void hanning(double *data, int n) {
  for (int i = 0; i < n; i++) {
    double multiplier = 0.5 * (1 - cos(2 * M_PI * i / (n - 1)));
    data[i] = multiplier * data[i];
  }
}

static void handle_dft(struct mg_connection *nc, struct http_message *hm) {
  char n1[100];

  /* Get form variables */
  mg_get_http_var(&hm->body, "windowWidth", n1, sizeof(n1));
  window_width = strtod(n1, NULL);
  mg_get_http_var(&hm->body, "windowOffset", n1, sizeof(n1));
  window_offset = strtod(n1, NULL);
  mg_get_http_var(&hm->body, "windowFunction", n1, sizeof(n1));
  if (n1[0] == 'N')
    window_function = 0;
  else
    window_function = (n1[2] == 'm') ? 1 : 2;

  /* Send headers */
  mg_printf(nc, "%s", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");

  /* Compute the result and send it back as a JSON object */
  mg_printf_http_chunk(nc, "{ \"song\": [");

  int start = wf->millisecondsToSample(window_offset);
  int end = wf->millisecondsToSample(window_offset + window_width);
  end = std::min(end, wf->numSamples() - 1);

  int n = end - start + 1;

  double time[n];
  double data[n];

  for (int i = start; i <= end; i++) {
    time[i - start] = wf->sampleToMilliseconds(i);
    data[i - start] = wf->data()[i];
  }

  if (window_function == 1) {
    hamming(data, n);
  } else if (window_function == 2) {
    hanning(data, n);
  }

  for (int i = 0; i < n; i++) {
    mg_printf_http_chunk(nc, "{\"time\": %lf, \"value\": %lf}", time[i],
                         data[i]);
    if (i != n - 1) {
      mg_printf_http_chunk(nc, ",");
    }
  }
  mg_printf_http_chunk(nc, "] }");
  mg_send_http_chunk(nc, "", 0); /* Send empty chunk, the end of response */
}

static void handle_process_file(struct mg_connection *nc,
                                struct http_message *hm) {

  /* Send headers */
  mg_printf(nc, "%s", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");

  char c[300];
  mg_get_http_var(&hm->body, "increment", c, sizeof(c));
  int inc = strtod(c, NULL);

  mg_get_http_var(&hm->body, "filename", c, sizeof(c));
  strcpy(c, "/Users/karadza3a/Downloads/primer.wav");
  wf = new WavFile(c);

  mg_printf_http_chunk(nc, "{ \"song\": [");
  for (int i = 0; i < wf->numSamples(); i += inc) {
    mg_printf_http_chunk(nc, "{\"time\": %lf, \"value\": %lf}",
                         wf->sampleToMilliseconds(i), wf->data()[i]);
    if (i + inc < wf->numSamples()) {
      mg_printf_http_chunk(nc, ",");
    }
  }
  mg_printf_http_chunk(nc, "] }");
  mg_send_http_chunk(nc, "", 0); /* Send empty chunk, the end of response */
}

static void handle_process_sine(struct mg_connection *nc,
                                struct http_message *hm) {

  /* Send headers */
  mg_printf(nc, "%s", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");

  char message[3000];
  mg_get_http_var(&hm->body, "nSamples", message, sizeof(message));
  int n = strtod(message, NULL);

  mg_get_http_var(&hm->body, "sines", message, sizeof(message));
  char *sines = message;

  double x1[n];
  double y1[n];

  for (int i = 0; i < n; i++) {
    x1[i] = 0;
    y1[i] = 0;
  }

  double a, b, c, bb, cc;
  int charsRead;
  while (sscanf(sines, "%lf,%lf,%lf%n", &a, &b, &c, &charsRead) == 3) {
    bb = n / (2.0 * b);
    cc = c == 0 ? 0 : M_PI / c;
    for (int i = 0; i < n; i++) {
      x1[i] += a * sin(M_PI * (i / bb) + cc);
    }
    sines += charsRead;
  }

  mg_printf_http_chunk(nc, "{ \"song\": [");
  for (int i = 0; i < n; i++) {
    mg_printf_http_chunk(nc, "{\"time\": %d, \"value\": %lf}", i, x1[i]);
    if (i + 1 < n) {
      mg_printf_http_chunk(nc, ",");
    }
  }
  mg_printf_http_chunk(nc, "] }");
  mg_send_http_chunk(nc, "", 0); /* Send empty chunk, the end of response */
}

static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
  struct http_message *hm = (struct http_message *)ev_data;

  switch (ev) {
  case MG_EV_HTTP_REQUEST:
    if (mg_vcmp(&hm->uri, "/dft") == 0) {
      handle_dft(nc, hm); /* Handle RESTful call */
    } else if (mg_vcmp(&hm->uri, "/process/file") == 0) {
      handle_process_file(nc, hm); /* Handle RESTful call */
    } else if (mg_vcmp(&hm->uri, "/process/sine") == 0) {
      handle_process_sine(nc, hm); /* Handle RESTful call */
    } else {
      mg_serve_http(nc, hm, s_http_server_opts); /* Serve static content */
    }
    break;
  default:
    break;
  }
}

int main(int argc, char *argv[]) {

  struct mg_mgr mgr;
  struct mg_connection *nc;
  int i;
  char *cp;
#ifdef MG_ENABLE_SSL
  const char *ssl_cert = NULL;
#endif

  mg_mgr_init(&mgr, NULL);

  /* Process command line options to customize HTTP server */
  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-D") == 0 && i + 1 < argc) {
      mgr.hexdump_file = argv[++i];
    } else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
      s_http_server_opts.document_root = argv[++i];
    } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
      s_http_port = argv[++i];
    } else if (strcmp(argv[i], "-a") == 0 && i + 1 < argc) {
      s_http_server_opts.auth_domain = argv[++i];
#ifdef MG_ENABLE_JAVASCRIPT
    } else if (strcmp(argv[i], "-j") == 0 && i + 1 < argc) {
      const char *init_file = argv[++i];
      mg_enable_javascript(&mgr, v7_create(), init_file);
#endif
    } else if (strcmp(argv[i], "-P") == 0 && i + 1 < argc) {
      s_http_server_opts.global_auth_file = argv[++i];
    } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
      s_http_server_opts.per_directory_auth_file = argv[++i];
    } else if (strcmp(argv[i], "-r") == 0 && i + 1 < argc) {
      s_http_server_opts.url_rewrites = argv[++i];
#ifndef MG_DISABLE_CGI
    } else if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
      s_http_server_opts.cgi_interpreter = argv[++i];
#endif
#ifdef MG_ENABLE_SSL
    } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
      ssl_cert = argv[++i];
#endif
    } else {
      fprintf(stderr, "Unknown option: [%s]\n", argv[i]);
      exit(1);
    }
  }

  /* Set HTTP server options */
  nc = mg_bind(&mgr, s_http_port, ev_handler);
  if (nc == NULL) {
    fprintf(stderr, "Error starting server on port %s\n", s_http_port);
    exit(1);
  }

#ifdef MG_ENABLE_SSL
  if (ssl_cert != NULL) {
    const char *err_str = mg_set_ssl(nc, ssl_cert, NULL);
    if (err_str != NULL) {
      fprintf(stderr, "Error loading SSL cert: %s\n", err_str);
      exit(1);
    }
  }
#endif

  mg_set_protocol_http_websocket(nc);
  s_http_server_opts.document_root = ".";
  s_http_server_opts.enable_directory_listing = "yes";

  /* Use current binary directory as document root */
  if (argc > 0 && ((cp = strrchr(argv[0], '/')) != NULL ||
                   (cp = strrchr(argv[0], '/')) != NULL)) {
    *cp = '\0';
    s_http_server_opts.document_root = argv[0];
  }

  printf("Starting RESTful server on port %s\n", s_http_port);
  for (;;) {
    mg_mgr_poll(&mgr, 1000);
  }
  mg_mgr_free(&mgr);

  return 0;
}
