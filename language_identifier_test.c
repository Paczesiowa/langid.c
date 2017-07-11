#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "language_identifier.h"

void print_double(double x) {
  char s[100], *p;
  sprintf(s, "%.12f", x);
  p = s + strlen(s) - 1;
  while (*p == '0' && *p-- != '.');
  *(p+1) = '\0';
  if (*p == '.') *p = '\0';
  printf("%s\n", s);
}

void check_model(LangID *model) {
  printf("%zu %zu\n", model->nb_ptc_rows, model->nb_ptc_cols);
  for (int i = 0; i < model->nb_ptc_rows; i++) {
    for (int j = 0; j < model->nb_ptc_cols; j++) {
      print_double(model->nb_ptc[j * model->nb_ptc_rows + i]);
    }
  }
  printf("%zu\n", model->nb_pc_length);
  for (int i = 0; i < model->nb_pc_length; i++) {
    print_double(model->nb_pc[i]);
  }
  printf("%zu\n", model->nb_classes_length);
  for (int i = 0; i < model->nb_classes_length; i++) {
    printf("%s\n", model->nb_classes[i]);
  }
  printf("%zu\n", model->tk_nextmove_length);
  for (int i = 0; i < model->tk_nextmove_length; i++) {
    printf("%hu\n", model->tk_nextmove[i]);
  }
  for (int i = 0; i <= model->tk_output_max_key; i++) {
    int *p = &model->tk_output[i * model->tk_output_max_tuple];
    if (*p != -1) {
      printf("%d ", i);
    } else {
      continue;
    }
    for (int k = 0; k < 4; k++) {
      if (*p == -1) {
        break;
      }
      printf("%d ", *p);
      p++;
    }
    printf("\n");
  }
  printf("%zu\n", model->nb_numfeats);
}

void check_output(LangID *model) {
  char language[3];
  char *text = "quick brown fox jumped over the lazy dog";
  double probability = classify(model, text, language, false);
  printf("The text '%s' has language %s (with probability %lf)\n", text, language, probability);
  probability = classify(model, text, language, true);
  printf("The text '%s' has language %s (with norm. probability %lf)\n", text, language, probability);
}

void benchmark(LangID *model) {
  char language[3];
  char *text = "quick brown fox jumped over the lazy dog";
  struct timeval t0, t1;
  gettimeofday(&t0, 0);
  int run_count = 5000;
  for (int i = 0; i < run_count; i++) {
    classify(model, text, language, false);
  }
  gettimeofday(&t1, 0);
  long elapsed = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;
  elapsed /= run_count;
  printf("%ld microseconds per run\n", elapsed);

  gettimeofday(&t0, 0);
  for (int i = 0; i < run_count; i++) {
    classify(model, text, language, true);
  }
  gettimeofday(&t1, 0);
  elapsed = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;
  elapsed /= run_count;
  printf("%ld microseconds per normalized run\n", elapsed);
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "language_identifier check_model|check_output|benchmark\n");
    return 1;
  }

  LangID* model = load_model();
  if (model == NULL) {
    return 1;
  }

  if (strcmp(argv[1], "check_model") == 0) {
    check_model(model);
  } else if (strcmp(argv[1], "check_output") == 0) {
    check_output(model);
  } else if (strcmp(argv[1], "benchmark") == 0) {
    benchmark(model);
  }

  free_model(model);
  return 0;
}
