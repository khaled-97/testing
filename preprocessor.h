/* Preprocessor for handling macros */
#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include "globals.h"

/* Process a .as file and create a .am file with expanded macros */
Bool preprocess_file(const char *filename);

#endif /* PREPROCESSOR_H */
