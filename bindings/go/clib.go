//go:build !yyaml_no_cgo

package yyaml

/*
#cgo CFLAGS: -I${SRCDIR}/../../yyaml

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

// Include the actual C source files
#include "../../yyaml/yyaml.h"
#include "../../yyaml/yyaml.c"
*/
import "C"
