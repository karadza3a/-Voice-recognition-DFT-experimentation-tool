/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#include "mongoose.h"
#include "kiss_fftr.h"
#include "WavFile.hpp"
#include <cmath>

// ------------- Globals ------------------------------

static const char *s_http_port = "8000";
static struct mg_serve_http_opts s_http_server_opts;

static bool usingFile;

static WavFile *wf;
static double *sinusoidData;
static int sinusoidDataLength;

static int display_increment;
static double window_width;
static double window_offset;
static int window_function = 0; // None, hamm, hann
static bool average_mode_enabled;

static double *window_time;
static double *window_data;
static int window_data_length;
static int window_length;
static int window_start;
static double *dft_out;

// ------------- Globals end --------------------------

static void hamming() {
  int n = window_length;
  for (int i = 0; i < n; i++) {
    double multiplier = 0.54 - 0.46 * cos(2 * M_PI * i / (n - 1));
    window_data[window_start + i] = multiplier * window_data[window_start + i];
  }
}

static void hanning() {
  int n = window_length;
  for (int i = 0; i < n; i++) {
    double multiplier = 0.5 * (1 - cos(2 * M_PI * i / (n - 1)));
    window_data[window_start + i] = multiplier * window_data[window_start + i];
  }
}

static void cropDataFromFile() {
  int start = wf->millisecondsToSample(window_offset);
  int end = std::min(wf->millisecondsToSample(window_offset + window_width),
                     (double)wf->numSamples());

  int n = end - start;
  window_length = n;

  if (window_length % 2 == 1)
    window_length++;
  window_time = new double[window_length];
  window_data = new double[window_length];

  for (int i = start; i < end; i++) {
    window_time[i - start] = wf->sampleToMilliseconds(i);
    window_data[i - start] = wf->data()[i];
  }
  if (window_length != n) {
    window_time[n] = wf->sampleToMilliseconds(n);
    window_data[n] = 0;
  }
}
static void cropDataFromSinusoidData() {
  int start = window_offset;
  int end = std::min((int)(window_offset + window_width), sinusoidDataLength);

  if (start > end)
    start = 0;

  int n = end - start;

  window_length = n;
  if (window_length % 2 == 1)
    window_length++;

  window_time = new double[window_length];
  window_data = new double[window_length];

  for (int i = start; i < end; i++) {
    window_time[i - start] = i;
    window_data[i - start] = sinusoidData[i];
  }
  if (window_length != n) {
    window_time[n] = n;
    window_data[n] = 0;
  }
}

static void execDft() {
  int n = window_length;

  kiss_fft_cpx sout[n];
  kiss_fftr_cfg kiss_fftr_state;
  kiss_fft_scalar rin[n + 2];

  for (int i = 0; i < n; ++i) {
    rin[i] = window_data[window_start + i];
  }
  kiss_fftr_state = kiss_fftr_alloc(n, 0, 0, 0);
  kiss_fftr(kiss_fftr_state, rin, sout);

  for (int i = 0; i < n / 2; ++i) {
    float re = sout[i].r / (n / 2.0);
    float im = sout[i].i / (n / 2.0);
    dft_out[i] += sqrt(re * re + im * im);
  }
}

static void get_spectre() {

  if (!average_mode_enabled) {
    if (usingFile) {
      cropDataFromFile();
    } else {
      cropDataFromSinusoidData();
    }
    window_data_length = window_length;
  } else { // average_mode_enabled
    if (usingFile) {
      window_data_length = wf->numSamples();
      window_length = std::min(wf->millisecondsToSample(window_width),
                               (double)wf->numSamples());
    } else {
      window_length = std::min((int)window_width, sinusoidDataLength);
      window_data_length = sinusoidDataLength;
    }

    if (window_length % 2 == 1)
      window_length++;

    int n = ceil(window_data_length / (double)window_length) * window_length;

    window_time = new double[n];
    window_data = new double[n];

    if (usingFile)
      for (int i = 0; i < window_data_length; i++)
        window_data[i] = wf->data()[i];
    else
      for (int i = 0; i < window_data_length; i++)
        window_data[i] = sinusoidData[i];
    for (int i = window_data_length; i < n; i++)
      window_data[i] = 0;
    window_data_length = n;

    if (usingFile)
      for (int i = 0; i < window_data_length; i++)
        window_time[i] = wf->millisecondsToSample(i);
    else
      for (int i = 0; i < window_data_length; i++)
        window_time[i] = i;
  }

  dft_out = new double[window_data_length / 2];
  for (int i = 0; i < window_data_length / 2; i++)
    dft_out[i] = 0;

  for (window_start = 0; window_start < window_data_length;
       window_start += window_length) {
    if (window_function == 1)
      hamming();
    else if (window_function == 2)
      hanning();
    execDft();
  }

  for (int i = 0; i < window_data_length / 2; i++)
    dft_out[i] /= (window_data_length / window_length);
}

static void handle_dft(struct mg_connection *nc, struct http_message *hm) {
  char n1[100];

  /* Get form variables */
  mg_get_http_var(&hm->body, "windowWidth", n1, sizeof(n1));
  window_width = strtod(n1, NULL);
  mg_get_http_var(&hm->body, "windowOffset", n1, sizeof(n1));
  window_offset = strtod(n1, NULL);
  mg_get_http_var(&hm->body, "average", n1, sizeof(n1));
  average_mode_enabled = n1[0] == 'y';
  mg_get_http_var(&hm->body, "windowFunction", n1, sizeof(n1));

  if (n1[0] == 'N')
    window_function = 0;
  else
    window_function = (n1[2] == 'm') ? 1 : 2;

  /* Send headers */
  mg_printf(nc, "%s", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");

  get_spectre();

  mg_printf_http_chunk(nc, "{ \"song\": [");
  for (int i = 0; i < window_data_length; i += display_increment) {
    mg_printf_http_chunk(nc, "{\"time\": %lf, \"value\": %lf}", window_time[i],
                         window_data[i]);
    if (i + display_increment < window_data_length) {
      mg_printf_http_chunk(nc, ",");
    }
  }
  mg_printf_http_chunk(nc, "], \"dft\": [");

  for (int i = 0; i < window_length / 2; i++) {
    mg_printf_http_chunk(nc, "{\"frequency\": %d, \"magnitude\": %lf}", i,
                         dft_out[i]);
    if (i + 1 < window_length / 2) {
      mg_printf_http_chunk(nc, ",");
    }
  }
  mg_printf_http_chunk(nc, "] } ");
  mg_send_http_chunk(nc, "", 0); /* Send empty chunk, the end of response */

  delete window_data;
  delete window_time;
}

static void handle_process_file(struct mg_connection *nc,
                                struct http_message *hm) {
  usingFile = true;

  char c[300];
  mg_get_http_var(&hm->body, "increment", c, sizeof(c));
  display_increment = strtod(c, NULL);

  mg_get_http_var(&hm->body, "filename", c, sizeof(c));

  wf = new WavFile(c);

  mg_printf(nc, "%s", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
  mg_printf_http_chunk(nc, "{ \"song\": [");
  for (int i = 0; i < wf->numSamples(); i += display_increment) {
    mg_printf_http_chunk(nc, "{\"time\": %lf, \"value\": %lf}",
                         wf->sampleToMilliseconds(i), wf->data()[i]);
    if (i + display_increment < wf->numSamples()) {
      mg_printf_http_chunk(nc, ",");
    }
  }
  mg_printf_http_chunk(nc, "] }");
  mg_send_http_chunk(nc, "", 0); /* Send empty chunk, the end of response */
}

static void handle_process_sine(struct mg_connection *nc,
                                struct http_message *hm) {
  usingFile = false;
  display_increment = 1;

  mg_printf(nc, "%s", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");

  char message[3000];
  mg_get_http_var(&hm->body, "nSamples", message, sizeof(message));
  int n = strtod(message, NULL);

  sinusoidDataLength = n;

  mg_get_http_var(&hm->body, "sines", message, sizeof(message));
  char *sines = message;

  sinusoidData = new double[n + 1];

  for (int i = 0; i < n + 1; i++) {
    sinusoidData[i] = 0;
  }

  double a, b, c, bb;
  int charsRead;
  while (sscanf(sines, "%lf,%lf,%lf%n", &a, &b, &c, &charsRead) == 3) {
    bb = n / (2.0 * b);
    for (int i = 0; i < n + 1; i++) {
      sinusoidData[i] += a * sin(M_PI * (i / bb) + M_PI * c);
    }
    sines += charsRead;
  }

  mg_printf_http_chunk(nc, "{ \"song\": [");
  for (int i = 0; i < n; i++) {
    mg_printf_http_chunk(nc, "{\"time\": %d, \"value\": %lf}", i,
                         sinusoidData[i]);
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
