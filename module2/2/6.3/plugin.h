#ifndef PLUGIN_H
#define PLUGIN_H

/* Договор между программой и библиотекой операции.
   В библиотеке ровно одна функция - op_apply, и строка-подпись op_name,
   по которой операция показывается в меню. */

#ifdef _WIN32
#define OP_EXPORT __declspec(dllexport)
#else
#define OP_EXPORT
#endif

#define OP_NAME_SYMBOL "op_name"
#define OP_APPLY_SYMBOL "op_apply"

typedef double (*OpApplyFn)(double n1, double n2);

#endif
