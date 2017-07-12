#include <stdbool.h>
#include <zmq.h>
#include <stdio.h>
/* #include <unistd.h> */
#include <string.h>
/* #include <assert.h> */

#include "language_identifier.h"

int main (void) {
  LangID* model = load_model();
  void *context = zmq_ctx_new();
  void *responder = zmq_socket(context, ZMQ_REP);
  zmq_bind (responder, "tcp://127.0.0.1:5555");

  int size;
  char language[3];
  double probability;
  char buffer[256];
  char response[20];
  while (true) {
    size = zmq_recv(responder, buffer, 255, 0);
    buffer[size] = '\0';
    probability = classify(model, &buffer[1], language, buffer[0] == '1');
    strcpy(response, language);
    strcpy(&response[2], " ");
    sprintf(&response[3], "%.2lf", probability);
    zmq_send(responder, response, strlen(response), 0);
  }
  return 0;
}
