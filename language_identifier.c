#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <linux/limits.h>

typedef char lang_code[3];

typedef struct LangID {
  double *nb_ptc;
  size_t nb_ptc_rows;
  size_t nb_ptc_cols;
  double *nb_pc;
  size_t nb_pc_length;
  lang_code *nb_classes;
  size_t nb_classes_length;
  unsigned short *tk_nextmove;
  size_t tk_nextmove_length;
  int *tk_output;
  size_t tk_output_max_key;
  size_t tk_output_max_tuple;
  size_t nb_numfeats;
} LangID;

void print_double(double x) {
  char s[100], *p;
  sprintf(s, "%.12f", x);
  p = s + strlen(s) - 1;
  while (*p == '0' && *p-- != '.');
  *(p+1) = '\0';
  if (*p == '.') *p = '\0';
  printf("%s\n", s);
}

size_t *read_size_t(FILE *fp, size_t *value) {
  char line[20];
  if (!fgets(line, 20, fp)) {
    return NULL;
  }
  if (sscanf(line, "%zu", value) != 1) {
    return NULL;
  }
  return value;
}

int *read_int(FILE *fp, int *value) {
  char line[20];
  if (!fgets(line, 20, fp)) {
    return NULL;
  }
  if (sscanf(line, "%d", value) != 1) {
    return NULL;
  }
  return value;
}

unsigned short *read_ushort(FILE *fp, unsigned short *value) {
  char line[20];
  if (!fgets(line, 20, fp)) {
    return NULL;
  }
  if (sscanf(line, "%hu", value) != 1) {
    return NULL;
  }
  return value;
}

double *read_double(FILE *fp, double *value) {
  char line[20];
  if (!fgets(line, 20, fp)) {
    return NULL;
  }
  if (sscanf(line, "%lf", value) != 1) {
    return NULL;
  }
  return value;
}

LangID* load_nb_ptc(LangID* model, const char *nb_ptc_path) {
  printf("Loading nb_ptc from %s\n", nb_ptc_path);
  FILE *fp = fopen(nb_ptc_path, "r");
  if (fp == NULL) {
    fprintf(stderr, "Error opening %s\n", nb_ptc_path);
    return NULL;
  }

  /* read first 2 lines that have number of rows and columns */
  if (!read_size_t(fp, &model->nb_ptc_rows)) {
    printf("Error reading number of rows\n");
    fclose(fp);
    return NULL;
  }
  if (!read_size_t(fp, &model->nb_ptc_cols)) {
    printf("Error reading number of columns\n");
    fclose(fp);
    return NULL;
  }

  model->nb_ptc = malloc(model->nb_ptc_rows * model->nb_ptc_cols * sizeof(double));
  if (model->nb_ptc == NULL) {
    printf("Error allocating memory\n");
    fclose(fp);
    return NULL;
  }

  /* read all matrix values in row-major order */
  for (int i = 0; i < model->nb_ptc_rows; i++) {
    for (int j = 0; j < model->nb_ptc_cols; j++) {
      if (!read_double(fp, &model->nb_ptc[i * model->nb_ptc_cols + j])) {
        printf("Error reading matrix value\n");
        fclose(fp);
        return NULL;
      }
    }
  }
  fclose(fp);
  return model;
}

LangID* load_nb_pc(LangID* model, const char *nb_pc_path) {
  printf("Loading nb_pc from %s\n", nb_pc_path);
  FILE *fp = fopen(nb_pc_path, "r");
  if (fp == NULL) {
    fprintf(stderr, "Error opening %s\n", nb_pc_path);
    return NULL;
  }

  /* read first line that contains length */
  if (!read_size_t(fp, &model->nb_pc_length)) {
    printf("Error reading length\n");
    fclose(fp);
    return NULL;
  }

  model->nb_pc = malloc(model->nb_pc_length * sizeof(double));
  if (model->nb_pc == NULL) {
    printf("Error allocating memory\n");
    fclose(fp);
    return NULL;
  }

  /* read whole array */
  for (int i = 0; i < model->nb_pc_length; i++) {
    if (!read_double(fp, &model->nb_pc[i])) {
      printf("Error reading array value\n");
      fclose(fp);
      return NULL;
    }
  }
  fclose(fp);
  return model;
}

LangID* load_nb_classes(LangID* model, const char *nb_classes_path) {
  printf("Loading nb_classes from %s\n", nb_classes_path);
  FILE *fp = fopen(nb_classes_path, "r");
  if (fp == NULL) {
    fprintf(stderr, "Error opening %s\n", nb_classes_path);
    return NULL;
  }

  /* read first line that contains length */
  if (!read_size_t(fp, &model->nb_classes_length)) {
    printf("Error reading length\n");
    fclose(fp);
    return NULL;
  }

  model->nb_classes = malloc(model->nb_classes_length * sizeof(lang_code));
  if (model->nb_classes == NULL) {
    printf("Error allocating memory\n");
    fclose(fp);
    return NULL;
  }

  char line[4];
  char *nb_class;
  
  /* read whole array */
  for (int i = 0; i < model->nb_classes_length; i++) {
    if (!fgets(line, 4, fp)) {
      printf("Error reading array value\n");
      fclose(fp);
      return NULL;
    }
    nb_class = (char *)&model->nb_classes[i];
    nb_class[0] = line[0];
    nb_class[1] = line[1];
    nb_class[2] = '\0';
  }
  fclose(fp);
  return model;
}

LangID* load_tk_nextmove(LangID* model, const char *tk_nextmove_path) {
  printf("Loading tk_nextmove from %s\n", tk_nextmove_path);
  FILE *fp = fopen(tk_nextmove_path, "r");
  if (fp == NULL) {
    fprintf(stderr, "Error opening %s\n", tk_nextmove_path);
    return NULL;
  }

  /* read first line that contains length */
  if (!read_size_t(fp, &model->tk_nextmove_length)) {
    printf("Error reading length\n");
    fclose(fp);
    return NULL;
  }

  model->tk_nextmove = malloc(model->tk_nextmove_length * sizeof(unsigned short));
  if (model->tk_nextmove == NULL) {
    printf("Error allocating memory\n");
    fclose(fp);
    return NULL;
  }

  /* read whole array */
  for (int i = 0; i < model->tk_nextmove_length; i++) {
    if (!read_ushort(fp, &model->tk_nextmove[i])) {
      printf("Error reading array value\n");
      fclose(fp);
      return NULL;
    }
  }
  fclose(fp);
  return model;
}

LangID* load_tk_output(LangID* model, const char *tk_output_path) {
  printf("Loading tk_output from %s\n", tk_output_path);
  FILE *fp = fopen(tk_output_path, "r");
  if (fp == NULL) {
    fprintf(stderr, "Error opening %s\n", tk_output_path);
    return NULL;
  }

  /* read first line that contains dict's max key value */
  if (!read_size_t(fp, &model->tk_output_max_key)) {
    printf("Error reading max key value\n");
    fclose(fp);
    return NULL;
  }

  /* read 2nd line that contains dict's value/tuple max size */
  if (!read_size_t(fp, &model->tk_output_max_tuple)) {
    printf("Error reading tuple size\n");
    fclose(fp);
    return NULL;
  }

  model->tk_output = malloc((model->tk_output_max_key + 1) * model->tk_output_max_tuple * sizeof(int));
  if (model->tk_output == NULL) {
    printf("Error allocating memory\n");
    fclose(fp);
    return NULL;
  }

  /* read whole dict */
  for (int i = 0; i <= model->tk_output_max_key; i++) { /* inclusive of max key */
    for (int j = 0; j < model->tk_output_max_tuple; j++) {
      if (!read_int(fp, &model->tk_output[i * model->tk_output_max_tuple] + j)) {
        printf("Error reading array value\n");
        fclose(fp);
        return NULL;
      }
    }
  }
  fclose(fp);
  return model;
}

void free_model(LangID *model) {
  if (model->nb_ptc) {
    free(model->nb_ptc);
  }
  if (model->nb_pc) {
    free(model->nb_pc);
  }
  if (model->nb_classes) {
    free(model->nb_classes);
  }
  if (model->tk_nextmove) {
    free(model->tk_nextmove);
  }
  if (model->tk_output) {
    free(model->tk_output);
  }
  free(model);
}

LangID* load_model() {
  printf("Loading model\n");

  LangID* model = malloc(sizeof(LangID));
  if (model == NULL) {
    fprintf(stderr, "Error allocating memory");
    return NULL;
  }

  char path[PATH_MAX];
  if (!getcwd(path, PATH_MAX)) {
    fprintf(stderr, "Error figuring out cwd");
    return NULL;
  }
  char *end_of_cwd_path = path + strlen(path);

  strcpy(end_of_cwd_path, "/model/nb_ptc.bin");
  if (!load_nb_ptc(model, path)) {
    free_model(model);
    return NULL;
  }

  strcpy(end_of_cwd_path, "/model/nb_pc.bin");
  if (!load_nb_pc(model, path)) {
    free_model(model);
    return NULL;
  }

  strcpy(end_of_cwd_path, "/model/nb_classes.bin");
  if (!load_nb_classes(model, path)) {
    free_model(model);
    return NULL;
  }

  strcpy(end_of_cwd_path, "/model/tk_nextmove.bin");
  if (!load_tk_nextmove(model, path)) {
    free_model(model);
    return NULL;
  }

  strcpy(end_of_cwd_path, "/model/tk_output.bin");
  if (!load_tk_output(model, path)) {
    free_model(model);
    return NULL;
  }

  model->nb_numfeats = (model->nb_ptc_rows * model->nb_ptc_cols) / model->nb_pc_length;
  
  return model;
}

void check_model(LangID *model) {
  printf("%zu %zu\n", model->nb_ptc_rows, model->nb_ptc_cols);
  for (int i = 0; i < model->nb_ptc_rows; i++) {
    for (int j = 0; j < model->nb_ptc_cols; j++) {
      print_double(model->nb_ptc[i * model->nb_ptc_cols + j]);
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

int main() {
  LangID* model = load_model();
  if (model == NULL) {
    return 1;
  }
  check_model(model);
  free_model(model);
  return 0;
}
