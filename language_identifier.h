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

void free_model(LangID *model);

LangID* load_model();

double classify(LangID *model, const char *text, char *language, bool normalize);
