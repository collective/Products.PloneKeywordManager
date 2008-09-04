/*
  Levenshtein.c v2003-09-06
  Python extension computing Levenshtein distances, string similarities,
  median strings and other goodies.

  Copyright (C) 2002-2003 David Necas (Yeti) <yeti@physics.muni.cz>.

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the Free
  Software Foundation; either version 2 of the License, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA.
*/

/**
 * TODO:
 *
 * - Implement weighted string averaging, see:
 *   H. Bunke et. al.: On the Weighted Mean of a Pair of Strings,
 *         Pattern Analysis and Applications 2002, 5(1): 23-30.
 *   X. Jiang et. al.: Dynamic Computations of Generalized Median Strings,
 *         Pattern Analysis and Applications 2002, ???.
 *   The latter also contains an interesting median-search algorithm.
 *
 * - Deal with stray symbols in greedy median() and median_improve().
 *   There are two possibilities:
 *    (i) Remember which strings contain which symbols.  This allows certain
 *        small optimizations when processing them.
 *   (ii) Use some overall heuristics to find symbols which don't worth
 *        trying.  This is very appealing, but hard to do properly
 *        (requires some inequality strong enough to allow practical exclusion
 *        of certain symbols -- at certain positions)
 *
 * - Optimize munkers_blackman(), it's pretty dumb (no memory of visited
 *   columns/rows)
 *
 * - Make it really usable as a C library (needs some wrappers, headers, ...,
 *   and maybe even documentation ;-)
 *
 * - Add interface to various interesting auxiliary results, namely
 *   set and sequence distance (only ratio is exported), the map from
 *   munkers_blackman() itself, ...
 *
 * - Generalizations:
 *   - character weight matrix/function
 *   - arbitrary edit operation costs, decomposable edit operations
 *
 * - Create a test suite
 *
 * - Add more interesting algorithms ;-)
 *
 * Postponed TODO (investigated, and a big `but' was found):
 *
 * - A linear approximate set median algorithm:
 *   P. Indyk: Sublinear time algorithms for metric space problems,
 *         STOC 1999, http://citeseer.nj.nec.com/indyk00sublinear.html.
 *   BUT: The algorithm seems to be advantageous only in the case of very
 *   large sets -- if my estimates are correct (the article itself is quite
 *   `asymptotic'), say 10^5 at least.  On smaller sets either one would get
 *   only an extermely rough median estimate, or the number of distance
 *   computations would be in fact higher than in the dumb O(n^2) algorithm.
 *
 * - Improve setmedian() speed with triangular inequality, see:
 *   Juan, A., E. Vidal: An Algorithm for Fast Median Search,
 *         1997, http://citeseer.nj.nec.com/article/juan97algorithm.html
 *   BUT: It doesn't seem to help much in spaces of high dimension (see the
 *   discussion and graphs in the article itself), a few percents at most,
 *   and strings behave like a space with a very high dimension (locally), so
 *   who knows, it probably wouldn't help much.
 *
 **/
#ifdef NO_PYTHON
#define _GNU_SOURCE
#include <string.h>
#include <math.h>
#else /* NO_PYTHON */
#define LEV_STATIC_PY static
#define lev_wchar Py_UNICODE
#include <Python.h>
#endif /* NO_PYTHON */
#include <assert.h>
#include <stdio.h>
#include "Levenshtein.h"

/* FIXME: inline avaliability should be solved in setup.py, somehow, or
 * even better in Python.h */
#ifndef __GNUC__
#define inline /* */
#endif

/* local functions */
static lev_wchar*
make_usymlist(size_t n,
              const size_t *lengths,
              const lev_wchar *strings[],
              size_t *symlistlen);

static size_t*
munkers_blackman(size_t n1,
                 size_t n2,
                 double *dists);

#ifndef NO_PYTHON
/* python interface and wrappers */
static PyObject* distance_py(PyObject *self, PyObject *args);
static PyObject* ratio_py(PyObject *self, PyObject *args);
static PyObject* median_py(PyObject *self, PyObject *args);
static PyObject* median_improve_py(PyObject *self, PyObject *args);
static PyObject* setmedian_py(PyObject *self, PyObject *args);
static PyObject* seqratio_py(PyObject *self, PyObject *args);
static PyObject* setratio_py(PyObject *self, PyObject *args);
static PyObject* editops_py(PyObject *self, PyObject *args);
static PyObject* opcodes_py(PyObject *self, PyObject *args);
static PyObject* inverse_py(PyObject *self, PyObject *args);
static PyObject* apply_edit_py(PyObject *self, PyObject *args);
static PyObject* matching_blocks_py(PyObject *self, PyObject *args);

#define Levenshtein_DESC \
  "A C extension module for fast computation of:\n" \
  "- Levenshtein (edit) distance and edit sequence manipluation\n" \
  "- string similarity\n" \
  "- approximate median strings, and generally string averaging\n" \
  "- string sequence and set similarity\n" \
  "\n" \
  "Levenshtein has a some overlap with difflib (SequenceMatcher).  It\n" \
  "supports only strings, not arbitrary sequence types, but on the\n" \
  "other hand it's much faster.\n" \
  "\n" \
  "It supports both normal and Unicode strings, but can't mix them, all\n" \
  "arguments to a function (method) have to be of the same type (or its\n" \
  "subclasses).\n"

#define distance_DESC \
  "Compute absolute Levenshtein distance of two strings.\n" \
  "\n" \
  "distance(string1, string2)\n" \
  "\n" \
  "Examples (it's hard to spell Levenshtein correctly):\n" \
  ">>> distance('Levenshtein', 'Lenvinsten')\n" \
  "4\n" \
  ">>> distance('Levenshtein', 'Levensthein')\n" \
  "2\n" \
  ">>> distance('Levenshtein', 'Levenshten')\n" \
  "1\n" \
  ">>> distance('Levenshtein', 'Levenshtein')\n" \
  "0\n" \
  "\n" \
  "Yeah, we've managed it at last.\n"

#define ratio_DESC \
  "Compute similarity of two strings.\n" \
  "\n" \
  "ratio(string1, string2)\n" \
  "\n" \
  "The similarity is a number between 0 and 1, it's usually equal or\n" \
  "somewhat higher than difflib.SequenceMatcher.ratio(), becuase it's\n" \
  "based on real minimal edit distance.\n" \
  "\n" \
  "Examples:\n" \
  ">>> ratio('Hello world!', 'Holly grail!')\n" \
  "0.58333333333333337\n" \
  ">>> ratio('Brian', 'Jesus')\n" \
  "0.0\n" \
  "\n" \
  "Really?  I thought there was some similarity.\n"

#define median_DESC \
  "Find an approximate generalized median string using greedy algorithm.\n" \
  "\n" \
  "median(string_sequence[, weight_sequence])\n" \
  "\n" \
  "You can optionally pass a weight for each string as the second\n" \
  "argument.  The weights are interpreted as item multiplicities,\n" \
  "although any non-negative real numbers are accepted.  Use them to\n" \
  "improve computation speed when strings often appear multiple times\n" \
  "in the sequence.\n" \
  "\n" \
  "Examples:\n" \
  ">>> median(['SpSm', 'mpamm', 'Spam', 'Spa', 'Sua', 'hSam'])\n" \
  "'Spam'\n" \
  ">>> fixme = ['Levnhtein', 'Leveshein', 'Leenshten',\n" \
  "...          'Leveshtei', 'Lenshtein', 'Lvenstein',\n" \
  "...          'Levenhtin', 'evenshtei']\n" \
  ">>> median(fixme)\n" \
  "'Levenshtein'\n" \
  "\n" \
  "Hm.  Even a computer program can spell Levenshtein better than me.\n"

#define median_improve_DESC \
  "Improve an approximate generalized median string by perturbations.\n" \
  "\n" \
  "median_improve(string, string_sequence[, weight_sequence])\n" \
  "\n" \
  "The first argument is the estimated generalized median string you\n" \
  "want to improve, the others are the same as in median().  It returns\n" \
  "a string with total distance less or equal to that of the given string.\n" \
  "\n" \
  "Note this is much slower than median().  Also note it performs only\n" \
  "one improvement step, calling median_improve() again on the result\n" \
  "may improve it further, though this is unlikely to happen unless the\n" \
  "given string was not very similar to the actual generalized median.\n" \
  "\n" \
  "Examples:\n" \
  ">>> fixme = ['Levnhtein', 'Leveshein', 'Leenshten',\n" \
  "...          'Leveshtei', 'Lenshtein', 'Lvenstein',\n" \
  "...          'Levenhtin', 'evenshtei']\n" \
  ">>> median_improve('spam', fixme)\n" \
  "'enhtein'\n" \
  ">>> median_improve(median_improve('spam', fixme), fixme)\n" \
  "'Levenshtein'\n" \
  "\n" \
  "It takes some work to change spam to Levenshtein.\n"

#define setmedian_DESC \
  "Find set median of a string set (passed as a sequence).\n" \
  "\n" \
  "setmedian(string[, weight_sequence])\n" \
  "\n" \
  "See median(), for argument description.\n" \
  "\n" \
  "The returned string is always one of the strings in the sequence.\n" \
  "\n" \
  "Examples:\n" \
  ">>> setmedian(['ehee', 'cceaes', 'chees', 'chreesc',\n" \
  "...            'chees', 'cheesee', 'cseese', 'chetese'])\n" \
  "'chees'\n" \
  "\n" \
  "You haven't asked me about Limburger, sir.\n"

#define seqratio_DESC \
  "Compute similarity ratio of two sequences of strings.\n" \
  "\n" \
  "seqratio(string_sequence1, string_sequence2)\n" \
  "\n" \
  "This is like ratio(), but for string sequences.  A kind of ratio()\n" \
  "is used to to measure the cost of item change operation for the\n" \
  "strings.\n" \
  "\n" \
  "Examples:\n" \
  ">>> seqratio(['newspaper', 'litter bin', 'tinny', 'antelope'],\n" \
  "...          ['caribou', 'sausage', 'gorn', 'woody'])\n" \
  "0.21517857142857144\n"

#define setratio_DESC \
  "Compute similarity ratio of two strings sets (passed as sequences).\n" \
  "\n" \
  "setratio(string_sequence1, string_sequence2)\n" \
  "\n" \
  "The best match between any strings in the first set and the second\n" \
  "set (passed as sequences) is attempted.  I.e., the order doesn't\n" \
  "matter here.\n" \
  "\n" \
  "Examples:\n" \
  ">>> setratio(['newspaper', 'litter bin', 'tinny', 'antelope'],\n" \
  "...          ['caribou', 'sausage', 'gorn', 'woody'])\n" \
  "0.28184523809523809\n" \
  "\n" \
  "No, even reordering doesn't help the tinny words to match the\n" \
  "woody ones.\n"

#define editops_DESC \
  "Find sequence of edit operations transforming one string to another.\n" \
  "\n" \
  "editops(source_string, destination_string)\n" \
  "editops(edit_operations, source_length, destination_length)\n" \
  "\n" \
  "The result is a list of triples (operation, spos, dpos), where\n" \
  "operation is one of `equal', `replace', `insert', or `delete';  spos\n" \
  "and dpos are position of characters in the first (source) and the\n" \
  "second (destination) strings.  These are operations on signle\n" \
  "characters.  In fact the returned list doesn't contain the `equal',\n" \
  "but all the related functions accept both lists with and without\n" \
  "`equal's.\n" \
  "\n" \
  "Examples:\n" \
  ">>> editops('spam', 'park')\n" \
  "[('delete', 0, 0), ('insert', 3, 2), ('replace', 3, 3)]\n" \
  "\n" \
  "The alternate form editops(opcodes, source_string, destination_string)\n" \
  "can be used for conversion from opcodes (5-tuples) to editops (you can\n" \
  "pass strings or their lengths, it doesn't matter).\n"

#define opcodes_DESC \
  "Find sequence of edit operations transforming one string to another.\n" \
  "\n" \
  "opcodes(source_string, destination_string)\n" \
  "opcodes(edit_operations, source_length, destination_length)\n" \
  "\n" \
  "The result is a list of 5-tuples with the same meaning as in\n" \
  "SequenceMatcher's get_opcodes() output.  But since the algorithms\n" \
  "differ, the actual sequences from Levenshtein and SequenceMatcher\n" \
  "may differ too.\n" \
  "\n" \
  "Examples:\n" \
  ">>> for x in opcodes('spam', 'park'):\n" \
  "...     print x\n" \
  "...\n" \
  "('delete', 0, 1, 0, 0)\n" \
  "('equal', 1, 3, 0, 2)\n" \
  "('insert', 3, 3, 2, 3)\n" \
  "('replace', 3, 4, 3, 4)\n" \
  "\n" \
  "The alternate form opcodes(editops, source_string, destination_string)\n" \
  "can be used for conversion from editops (triples) to opcodes (you can\n" \
  "pass strings or their lengths, it doesn't matter).\n"

#define inverse_DESC \
  "Invert the sense of an edit operation sequence.\n" \
  "\n" \
  "inverse(edit_operations)\n" \
  "\n" \
  "In other words, it returns a list of edit operations transforming the\n" \
  "second (destination) string to the first (source).  It can be uses\n" \
  "with both editops and opcodes.\n" \
  "\n" \
  "Examples:\n" \
  ">>> inverse(editops('spam', 'park'))\n" \
  "[('insert', 0, 0), ('delete', 2, 3), ('replace', 3, 3)]\n" \
  ">>> editops('park', 'spam')\n" \
  "[('insert', 0, 0), ('delete', 2, 3), ('replace', 3, 3)]\n"

#define apply_edit_DESC \
  "Apply a sequence of edit operations to a string.\n" \
  "\n" \
  "apply_edit(edit_operations, source_string, destination_string)\n" \
  "\n" \
  "In the case of editops, the sequence can be arbitrary ordered subset\n" \
  "of the edit sequence transforming source_string to destination_string.\n" \
  "\n" \
  "Examples:\n" \
  ">>> e = editops('man', 'scotsman')\n" \
  ">>> apply_edit(e, 'man', 'scotsman')\n" \
  "'scotsman'\n" \
  ">>> apply_edit(e[:3], 'man', 'scotsman')\n" \
  "'scoman'\n" \
  "\n" \
  "The other form of edit operations, opcodes, is not very suitable for\n" \
  "such a tricks, because it has to always span over complete strings,\n" \
  "subsets can be created by carefully replacing blocks with `equal'\n" \
  "blocks, or by enlarging `equal' block at the expense of other blocks\n" \
  "and adjusting the other blocks accordingly.\n" \
  "\n" \
  "Examples:\n" \
  ">>> a, b = 'spam and eggs', 'foo and bar'\n" \
  ">>> e = opcodes(a, b)\n" \
  ">>> apply_edit(inverse(e), b, a)\n" \
  "'spam and eggs'\n" \
  ">>> e[4] = ('equal', 10, 13, 8, 11)\n" \
  ">>> apply_edit(e, a, b)\n" \
  "'foo and ggs'\n"

#define matching_blocks_DESC \
  "Find identical blocks in two strings.\n" \
  "\n" \
  "matching_blocks(edit_operations, source_length, destination_length)\n" \
  "\n" \
  "The result is a list of triples with the same meaning as in\n" \
  "SequenceMatcher's get_matching_blocks() output.  It can be used with\n" \
  "both editops and opcodes.  The second and third arguments dont't\n" \
  "to be actually strings, their lengths are enough.\n" \
  "\n" \
  "Examples:\n" \
  ">>> a, b = 'spam', 'park'\n" \
  ">>> matching_blocks(editops(a, b), a, b)\n" \
  "[(1, 0, 2), (4, 4, 0)]\n" \
  ">>> matching_blocks(editops(a, b), len(a), len(b))\n" \
  "[(1, 0, 2), (4, 4, 0)]\n"

#define METHODS_ITEM(x) { #x, x##_py, METH_VARARGS, x##_DESC }
PyMethodDef methods[] = {
  METHODS_ITEM(distance),
  METHODS_ITEM(ratio),
  METHODS_ITEM(median),
  METHODS_ITEM(median_improve),
  METHODS_ITEM(setmedian),
  METHODS_ITEM(seqratio),
  METHODS_ITEM(setratio),
  METHODS_ITEM(editops),
  METHODS_ITEM(opcodes),
  METHODS_ITEM(inverse),
  METHODS_ITEM(apply_edit),
  METHODS_ITEM(matching_blocks),
  { NULL, NULL, 0, NULL },
};

/* opcode names, these are to be initialized in the init func,
 * indexed by LevEditType values */
struct {
  PyObject* pystring;
  const char *cstring;
  size_t len;
}
static opcode_names[] = {
  { NULL, "equal", 0 },
  { NULL, "replace", 0 },
  { NULL, "insert", 0 },
  { NULL, "delete", 0 },
};
#define N_OPCODE_NAMES ((sizeof(opcode_names)/sizeof(opcode_names[0])))

typedef lev_byte *(*MedianFuncString)(size_t n,
                                      const size_t *lengths,
                                      const lev_byte *strings[],
                                      const double *weights,
                                      size_t *medlength);
typedef Py_UNICODE *(*MedianFuncUnicode)(size_t n,
                                         const size_t *lengths,
                                         const Py_UNICODE *strings[],
                                         const double *weights,
                                         size_t *medlength);
typedef struct {
  MedianFuncString s;
  MedianFuncUnicode u;
} MedianFuncs;

typedef lev_byte *(*MedianImproveFuncString)(size_t len, const lev_byte *s,
                                             size_t n,
                                             const size_t *lengths,
                                             const lev_byte *strings[],
                                             const double *weights,
                                             size_t *medlength);
typedef Py_UNICODE *(*MedianImproveFuncUnicode)(size_t len, const Py_UNICODE *s,
                                                size_t n,
                                                const size_t *lengths,
                                                const Py_UNICODE *strings[],
                                                const double *weights,
                                                size_t *medlength);
typedef struct {
  MedianImproveFuncString s;
  MedianImproveFuncUnicode u;
} MedianImproveFuncs;

typedef double (*SetSeqFuncString)(size_t n1,
                                   const size_t *lengths1,
                                   const lev_byte *strings1[],
                                   size_t n2,
                                   const size_t *lengths2,
                                   const lev_byte *strings2[]);
typedef double (*SetSeqFuncUnicode)(size_t n1,
                                    const size_t *lengths1,
                                    const Py_UNICODE *strings1[],
                                    size_t n2,
                                    const size_t *lengths2,
                                    const Py_UNICODE *strings2[]);

typedef struct {
  SetSeqFuncString s;
  SetSeqFuncUnicode u;
} SetSeqFuncs;

static long int
levenshtein_common(PyObject *args,
                   char *name,
                   size_t xcost,
                   size_t *lensum);

static int
extract_stringlist(PyObject *list,
                   const char *name,
                   size_t n,
                   size_t **sizelist,
                   void *strlist);

static double*
extract_weightlist(PyObject *wlist,
                   const char *name,
                   size_t n);

static PyObject*
median_common(PyObject *args,
              const char *name,
              MedianFuncs foo);

static PyObject*
median_improve_common(PyObject *args,
                      const char *name,
                      MedianImproveFuncs foo);

static double
setseq_common(PyObject *args,
              const char *name,
              SetSeqFuncs foo,
              size_t *lensum);

/****************************************************************************
 *
 * Python interface and subroutines
 *
 ****************************************************************************/

static long int
levenshtein_common(PyObject *args, char *name, size_t xcost,
                   size_t *lensum)
{
  PyObject *arg1, *arg2;
  size_t len1, len2;

  if (!PyArg_UnpackTuple(args, name, 2, 2, &arg1, &arg2))
    return -1;

  if (PyObject_TypeCheck(arg1, &PyString_Type)
      && PyObject_TypeCheck(arg2, &PyString_Type)) {
    lev_byte *string1, *string2;

    len1 = PyString_GET_SIZE(arg1);
    len2 = PyString_GET_SIZE(arg2);
    *lensum = len1 + len2;
    string1 = PyString_AS_STRING(arg1);
    string2 = PyString_AS_STRING(arg2);
    {
      size_t d = lev_edit_distance(len1, string1, len2, string2, xcost);
      if (d == (size_t)(-1)) {
        PyErr_NoMemory();
        return -1;
      }
      return d;
    }
  }
  else if (PyObject_TypeCheck(arg1, &PyUnicode_Type)
      && PyObject_TypeCheck(arg2, &PyUnicode_Type)) {
    Py_UNICODE *string1, *string2;

    len1 = PyUnicode_GET_SIZE(arg1);
    len2 = PyUnicode_GET_SIZE(arg2);
    *lensum = len1 + len2;
    string1 = PyUnicode_AS_UNICODE(arg1);
    string2 = PyUnicode_AS_UNICODE(arg2);
    {
      size_t d = lev_u_edit_distance(len1, string1, len2, string2, xcost);
      if (d == (size_t)(-1)) {
        PyErr_NoMemory();
        return -1;
      }
      return d;
    }
  }
  else {
    PyErr_Format(PyExc_ValueError,
                 "%s expected two Strings or two Unicodes", name);
    return -1;
  }
}

static PyObject*
distance_py(PyObject *self, PyObject *args)
{
  size_t lensum;
  long int ldist;

  if ((ldist = levenshtein_common(args, "distance", 0, &lensum)) < 0)
    return NULL;

  return PyInt_FromLong((long)ldist);
}

static PyObject*
ratio_py(PyObject *self, PyObject *args)
{
  size_t lensum;
  long int ldist;

  if ((ldist = levenshtein_common(args, "ratio", 1, &lensum)) < 0)
    return NULL;

  if (lensum == 0)
    return PyFloat_FromDouble(1.0);

  return PyFloat_FromDouble((double)(lensum - ldist)/(lensum));
}

static PyObject*
median_py(PyObject *self, PyObject *args)
{
  MedianFuncs engines = { lev_greedy_median, lev_u_greedy_median };
  return median_common(args, "median", engines);
}

static PyObject*
median_improve_py(PyObject *self, PyObject *args)
{
  MedianImproveFuncs engines = { lev_median_improve, lev_u_median_improve };
  return median_improve_common(args, "median_improve", engines);
}

static PyObject*
setmedian_py(PyObject *self, PyObject *args)
{
  MedianFuncs engines = { lev_set_median, lev_u_set_median };
  return median_common(args, "setmedian", engines);
}

static PyObject*
median_common(PyObject *args, const char *name, MedianFuncs foo)
{
  size_t n, len;
  void *strings = NULL;
  size_t *sizes = NULL;
  PyObject *strlist = NULL;
  PyObject *wlist = NULL;
  PyObject *strseq = NULL;
  double *weights;
  int stringtype;
  PyObject *result = NULL;

  if (!PyArg_UnpackTuple(args, name, 1, 2, &strlist, &wlist))
    return NULL;

  if (!PySequence_Check(strlist)) {
    PyErr_Format(PyExc_ValueError,
                 "%s first argument must be a Sequence", name);
    return NULL;
  }
  strseq = PySequence_Fast(strlist, name);

  n = PySequence_Fast_GET_SIZE(strseq);
  if (n == 0) {
    Py_INCREF(Py_None);
    Py_DECREF(strseq);
    return Py_None;
  }

  /* get (optional) weights, use 1 if none specified. */
  weights = extract_weightlist(wlist, name, n);
  if (!weights) {
    Py_DECREF(strseq);
    return NULL;
  }

  stringtype = extract_stringlist(strseq, name, n, &sizes, &strings);
  Py_DECREF(strseq);
  if (stringtype < 0) {
    free(weights);
    return NULL;
  }

  if (stringtype == 0) {
    lev_byte *medstr = foo.s(n, sizes, strings, weights, &len);
    if (!medstr && len)
      result = PyErr_NoMemory();
    else {
      result = PyString_FromStringAndSize(medstr, len);
      free(medstr);
    }
  }
  else if (stringtype == 1) {
    Py_UNICODE *medstr = foo.u(n, sizes, strings, weights, &len);
    if (!medstr && len)
      result = PyErr_NoMemory();
    else {
      result = PyUnicode_FromUnicode(medstr, len);
      free(medstr);
    }
  }
  else
    PyErr_Format(PyExc_SystemError, "%s internal error", name);

  free(strings);
  free(weights);
  free(sizes);
  return result;
}

static PyObject*
median_improve_common(PyObject *args, const char *name, MedianImproveFuncs foo)
{
  size_t n, len;
  void *strings = NULL;
  size_t *sizes = NULL;
  PyObject *arg1 = NULL;
  PyObject *strlist = NULL;
  PyObject *wlist = NULL;
  PyObject *strseq = NULL;
  double *weights;
  int stringtype;
  PyObject *result = NULL;

  if (!PyArg_UnpackTuple(args, name, 2, 3, &arg1, &strlist, &wlist))
    return NULL;

  if (PyObject_TypeCheck(arg1, &PyString_Type))
    stringtype = 0;
  else if (PyObject_TypeCheck(arg1, &PyUnicode_Type))
    stringtype = 1;
  else {
    PyErr_Format(PyExc_ValueError,
                 "%s first argument must be a String or Unicode", name);
    return NULL;
  }

  if (!PySequence_Check(strlist)) {
    PyErr_Format(PyExc_ValueError,
                 "%s second argument must be a Sequence", name);
    return NULL;
  }
  strseq = PySequence_Fast(strlist, name);

  n = PySequence_Fast_GET_SIZE(strseq);
  if (n == 0) {
    Py_INCREF(Py_None);
    Py_DECREF(strseq);
    return Py_None;
  }

  /* get (optional) weights, use 1 if none specified. */
  weights = extract_weightlist(wlist, name, n);
  if (!weights) {
    Py_DECREF(strseq);
    return NULL;
  }

  if (extract_stringlist(strseq, name, n, &sizes, &strings) != stringtype) {
    PyErr_Format(PyExc_ValueError,
                 "%s argument types don't match", name);
    free(weights);
    return NULL;
  }

  Py_DECREF(strseq);
  if (stringtype == 0) {
    lev_byte *s = PyString_AS_STRING(arg1);
    size_t l = PyString_GET_SIZE(arg1);
    lev_byte *medstr = foo.s(l, s, n, sizes, strings, weights, &len);
    if (!medstr && len)
      result = PyErr_NoMemory();
    else {
      result = PyString_FromStringAndSize(medstr, len);
      free(medstr);
    }
  }
  else if (stringtype == 1) {
    Py_UNICODE *s = PyUnicode_AS_UNICODE(arg1);
    size_t l = PyUnicode_GET_SIZE(arg1);
    Py_UNICODE *medstr = foo.u(l, s, n, sizes, strings, weights, &len);
    if (!medstr && len)
      result = PyErr_NoMemory();
    else {
      result = PyUnicode_FromUnicode(medstr, len);
      free(medstr);
    }
  }
  else
    PyErr_Format(PyExc_SystemError, "%s internal error", name);

  free(strings);
  free(weights);
  free(sizes);
  return result;
}

static double*
extract_weightlist(PyObject *wlist, const char *name, size_t n)
{
  size_t i;
  double *weights = NULL;
  PyObject *seq;

  if (wlist) {
    if (!PySequence_Check(wlist)) {
      PyErr_Format(PyExc_ValueError,
                  "%s second argument must be a Sequence", name);
      return NULL;
    }
    seq = PySequence_Fast(wlist, name);
    if (PySequence_Fast_GET_SIZE(wlist) != n) {
      PyErr_Format(PyExc_ValueError,
                   "%s got %i strings but %i weights",
                   name, n, PyList_GET_SIZE(wlist));
      Py_DECREF(seq);
      return NULL;
    }
    weights = (double*)malloc(n*sizeof(double));
    if (!weights)
      return (double*)PyErr_NoMemory();
    for (i = 0; i < n; i++) {
      PyObject *item = PySequence_Fast_GET_ITEM(wlist, i);
      PyObject *number = PyNumber_Float(item);

      if (!number) {
        free(weights);
        PyErr_Format(PyExc_ValueError,
                     "%s weight #%i is not a Number", name, i);
        Py_DECREF(seq);
        return NULL;
      }
      weights[i] = PyFloat_AS_DOUBLE(number);
      Py_DECREF(number);
      if (weights[i] < 0) {
        free(weights);
        PyErr_Format(PyExc_ValueError,
                     "%s weight #%i is negative", name, i);
        Py_DECREF(seq);
        return NULL;
      }
    }
    Py_DECREF(seq);
  }
  else {
    weights = (double*)malloc(n*sizeof(double));
    if (!weights)
      return (double*)PyErr_NoMemory();
    for (i = 0; i < n; i++)
      weights[i] = 1.0;
  }

  return weights;
}

/* extract a list of strings or unicode strings, returns
 * 0 -- strings
 * 1 -- unicode strings
 * <0 -- failure
 */
static int
extract_stringlist(PyObject *list, const char *name,
                   size_t n, size_t **sizelist, void *strlist)
{
  size_t i;
  PyObject *first;

  /* check first object type.  when it's a string then all others must be
   * strings too; when it's a unicode string then all others must be unicode
   * strings too. */
  first = PySequence_Fast_GET_ITEM(list, 0);
  /* i don't exactly understand why the problem doesn't exhibit itself earlier
   * but a queer error message is better than a segfault :o) */
  if (first == (PyObject*)-1) {
    PyErr_Format(PyExc_TypeError,
                 "%s undecomposable Sequence???", name);
    return -1;
  }

  if (PyObject_TypeCheck(first, &PyString_Type)) {
    lev_byte **strings;
    size_t *sizes;

    strings = (lev_byte**)malloc(n*sizeof(lev_byte*));
    if (!strings) {
      PyErr_Format(PyExc_MemoryError,
                   "%s cannot allocate memory", name);
      return -1;
    }
    sizes = (size_t*)malloc(n*sizeof(size_t));
    if (!sizes) {
      free(strings);
      PyErr_Format(PyExc_MemoryError,
                   "%s cannot allocate memory", name);
      return -1;
    }

    strings[0] = PyString_AS_STRING(first);
    sizes[0] = PyString_GET_SIZE(first);
    for (i = 1; i < n; i++) {
      PyObject *item = PySequence_Fast_GET_ITEM(list, i);

      if (!PyObject_TypeCheck(item, &PyString_Type)) {
        free(strings);
        free(sizes);
        PyErr_Format(PyExc_ValueError,
                     "%s item #%i is not a String", name, i);
        return -1;
      }
      strings[i] = PyString_AS_STRING(item);
      sizes[i] = PyString_GET_SIZE(item);
    }

    *(lev_byte***)strlist = strings;
    *sizelist = sizes;
    return 0;
  }
  if (PyObject_TypeCheck(first, &PyUnicode_Type)) {
    Py_UNICODE **strings;
    size_t *sizes;

    strings = (Py_UNICODE**)malloc(n*sizeof(Py_UNICODE*));
    if (!strings) {
      PyErr_NoMemory();
      return -1;
    }
    sizes = (size_t*)malloc(n*sizeof(size_t));
    if (!sizes) {
      free(strings);
      PyErr_NoMemory();
      return -1;
    }

    strings[0] = PyUnicode_AS_UNICODE(first);
    sizes[0] = PyUnicode_GET_SIZE(first);
    for (i = 1; i < n; i++) {
      PyObject *item = PySequence_Fast_GET_ITEM(list, i);

      if (!PyObject_TypeCheck(item, &PyUnicode_Type)) {
        free(strings);
        free(sizes);
        PyErr_Format(PyExc_ValueError,
                     "%s item #%i is not a Unicode", name, i);
        return -1;
      }
      strings[i] = PyUnicode_AS_UNICODE(item);
      sizes[i] = PyUnicode_GET_SIZE(item);
    }

    *(Py_UNICODE***)strlist = strings;
    *sizelist = sizes;
    return 1;
  }

  PyErr_Format(PyExc_ValueError,
               "%s expected list of Strings or Unicodes", name);
  return -1;
}

static PyObject*
seqratio_py(PyObject *self, PyObject *args)
{
  SetSeqFuncs engines = { lev_edit_seq_distance, lev_u_edit_seq_distance };
  size_t lensum;
  double r = setseq_common(args, "seqratio", engines, &lensum);
  if (r < 0)
    return NULL;
  if (lensum == 0)
    return PyFloat_FromDouble(1.0);
  return PyFloat_FromDouble((lensum - r)/lensum);
}

static PyObject*
setratio_py(PyObject *self, PyObject *args)
{
  SetSeqFuncs engines = { lev_set_distance, lev_u_set_distance };
  size_t lensum;
  double r = setseq_common(args, "setratio", engines, &lensum);
  if (r < 0)
    return NULL;
  if (lensum == 0)
    return PyFloat_FromDouble(1.0);
  return PyFloat_FromDouble((lensum - r)/lensum);
}

static double
setseq_common(PyObject *args, const char *name, SetSeqFuncs foo,
              size_t *lensum)
{
  size_t n1, n2;
  void *strings1 = NULL;
  void *strings2 = NULL;
  size_t *sizes1 = NULL;
  size_t *sizes2 = NULL;
  PyObject *strlist1;
  PyObject *strlist2;
  PyObject *strseq1;
  PyObject *strseq2;
  int stringtype1, stringtype2;
  double r = -1.0;

  if (!PyArg_UnpackTuple(args, name, 2, 2, &strlist1, &strlist2))
    return r;

  if (!PySequence_Check(strlist1)) {
    PyErr_Format(PyExc_ValueError,
                 "%s first argument must be a Sequence", name);
    return r;
  }
  if (!PySequence_Check(strlist2)) {
    PyErr_Format(PyExc_ValueError,
                 "%s second argument must be a Sequence", name);
    return r;
  }

  strseq1 = PySequence_Fast(strlist1, name);
  strseq2 = PySequence_Fast(strlist2, name);

  n1 = PySequence_Fast_GET_SIZE(strlist1);
  n2 = PySequence_Fast_GET_SIZE(strlist2);
  *lensum = n1 + n2;
  if (n1 == 0) {
    Py_DECREF(strseq1);
    Py_DECREF(strseq2);
    return (double)n2;
  }
  if (n2 == 0) {
    Py_DECREF(strseq1);
    Py_DECREF(strseq2);
    return (double)n1;
  }

  stringtype1 = extract_stringlist(strlist1, name, n1, &sizes1, &strings1);
  Py_DECREF(strseq1);
  if (stringtype1 < 0) {
    Py_DECREF(strseq2);
    return r;
  }
  stringtype2 = extract_stringlist(strlist2, name, n2, &sizes2, &strings2);
  Py_DECREF(strseq2);
  if (stringtype2 < 0) {
    free(sizes1);
    free(strings1);
    return r;
  }

  if (stringtype1 != stringtype2) {
    PyErr_Format(PyExc_ValueError,
                  "%s both sequences must consist of items of the same type",
                  name);
  }
  else if (stringtype1 == 0) {
    r = foo.s(n1, sizes1, strings1, n2, sizes2, strings2);
    if (r < 0.0)
      PyErr_NoMemory();
  }
  else if (stringtype1 == 1) {
    r = foo.u(n1, sizes1, strings1, n2, sizes2, strings2);
    if (r < 0.0)
      PyErr_NoMemory();
  }
  else
    PyErr_Format(PyExc_SystemError, "%s internal error", name);

  free(strings1);
  free(strings2);
  free(sizes1);
  free(sizes2);
  return r;
}

static inline LevEditType
string_to_edittype(PyObject *string)
{
  const char *s;
  size_t i, len;

  for (i = 0; i < N_OPCODE_NAMES; i++) {
    if (string == opcode_names[i].pystring)
      return i;
  }

  /* With Python >= 2.2, we shouldn't get here, except when the strings are
   * not Strings but subtypes. */
  if (!PyString_Check(string))
    return LEV_EDIT_LAST;

  s = PyString_AS_STRING(string);
  len = PyString_GET_SIZE(string);
  for (i = 0; i < N_OPCODE_NAMES; i++) {
    if (len == opcode_names[i].len
        && memcmp(s, opcode_names[i].cstring, len) == 0)
      return i;
  }

  return LEV_EDIT_LAST;
}

static LevEditOp*
extract_editops(PyObject *list)
{
  LevEditOp *ops;
  size_t i;
  LevEditType type;
  size_t n = PyList_GET_SIZE(list);

  ops = (LevEditOp*)malloc(n*sizeof(LevEditOp));
  if (!ops)
    return (LevEditOp*)PyErr_NoMemory();
  for (i = 0; i < n; i++) {
    PyObject *item;
    PyObject *tuple = PyList_GET_ITEM(list, i);

    if (!PyTuple_Check(tuple) || PyTuple_GET_SIZE(tuple) != 3) {
      free(ops);
      return NULL;
    }
    item = PyTuple_GET_ITEM(tuple, 0);
    if (!PyString_Check(item)
        || ((type = string_to_edittype(item)) == LEV_EDIT_LAST)) {
      free(ops);
      return NULL;
    }
    ops[i].type = type;
    item = PyTuple_GET_ITEM(tuple, 1);
    if (!PyInt_Check(item)) {
      free(ops);
      return NULL;
    }
    ops[i].spos = (size_t)PyInt_AS_LONG(item);
    item = PyTuple_GET_ITEM(tuple, 2);
    if (!PyInt_Check(item)) {
      free(ops);
      return NULL;
    }
    ops[i].dpos = (size_t)PyInt_AS_LONG(item);
  }
  return ops;
}

static LevOpCode*
extract_opcodes(PyObject *list)
{
  LevOpCode *bops;
  size_t i;
  LevEditType type;
  size_t nb = PyList_GET_SIZE(list);

  bops = (LevOpCode*)malloc(nb*sizeof(LevOpCode));
  if (!bops)
    return (LevOpCode*)PyErr_NoMemory();
  for (i = 0; i < nb; i++) {
    PyObject *item;
    PyObject *tuple = PyList_GET_ITEM(list, i);

    if (!PyTuple_Check(tuple) || PyTuple_GET_SIZE(tuple) != 5) {
      free(bops);
      return NULL;
    }
    item = PyTuple_GET_ITEM(tuple, 0);
    if (!PyString_Check(item)
        || ((type = string_to_edittype(item)) == LEV_EDIT_LAST)) {
      free(bops);
      return NULL;
    }
    bops[i].type = type;
    item = PyTuple_GET_ITEM(tuple, 1);
    if (!PyInt_Check(item)) {
      free(bops);
      return NULL;
    }
    bops[i].sbeg = (size_t)PyInt_AS_LONG(item);
    item = PyTuple_GET_ITEM(tuple, 2);
    if (!PyInt_Check(item)) {
      free(bops);
      return NULL;
    }
    bops[i].send = (size_t)PyInt_AS_LONG(item);
    item = PyTuple_GET_ITEM(tuple, 3);
    if (!PyInt_Check(item)) {
      free(bops);
      return NULL;
    }
    bops[i].dbeg = (size_t)PyInt_AS_LONG(item);
    item = PyTuple_GET_ITEM(tuple, 4);
    if (!PyInt_Check(item)) {
      free(bops);
      return NULL;
    }
    bops[i].dend = (size_t)PyInt_AS_LONG(item);
  }
  return bops;
}

static PyObject*
editops_to_tuple_list(size_t n, LevEditOp *ops)
{
  PyObject *list;
  size_t i;

  list = PyList_New(n);
  for (i = 0; i < n; i++, ops++) {
    PyObject *tuple = PyTuple_New(3);
    PyObject *is = opcode_names[ops->type].pystring;
    Py_INCREF(is);
    PyTuple_SET_ITEM(tuple, 0, is);
    PyTuple_SET_ITEM(tuple, 1, PyInt_FromLong((long)ops->spos));
    PyTuple_SET_ITEM(tuple, 2, PyInt_FromLong((long)ops->dpos));
    PyList_SET_ITEM(list, i, tuple);
  }

  return list;
}

static PyObject*
matching_blocks_to_tuple_list(size_t len1, size_t len2,
                              size_t nmb, LevMatchingBlock *mblocks)
{
  PyObject *list, *tuple;
  size_t i;

  list = PyList_New(nmb + 1);
  for (i = 0; i < nmb; i++, mblocks++) {
    tuple = PyTuple_New(3);
    PyTuple_SET_ITEM(tuple, 0, PyInt_FromLong((long)mblocks->spos));
    PyTuple_SET_ITEM(tuple, 1, PyInt_FromLong((long)mblocks->dpos));
    PyTuple_SET_ITEM(tuple, 2, PyInt_FromLong((long)mblocks->len));
    PyList_SET_ITEM(list, i, tuple);
  }
  tuple = PyTuple_New(3);
  PyTuple_SET_ITEM(tuple, 0, PyInt_FromLong((long)len1));
  PyTuple_SET_ITEM(tuple, 1, PyInt_FromLong((long)len2));
  PyTuple_SET_ITEM(tuple, 2, PyInt_FromLong((long)0));
  PyList_SET_ITEM(list, nmb, tuple);

  return list;
}

static size_t
get_length_of_anything(PyObject *object)
{
  if (PyInt_Check(object)) {
    long int len = PyInt_AS_LONG(object);
    if (len < 0)
      len = -1;
    return (size_t)len;
  }
  if (PySequence_Check(object))
    return PySequence_Length(object);
  return (size_t)-1;
}

static PyObject*
editops_py(PyObject *self, PyObject *args)
{
  PyObject *arg1, *arg2, *arg3 = NULL;
  PyObject *oplist;
  size_t len1, len2, n;
  LevEditOp *ops;
  LevOpCode *bops;

  if (!PyArg_UnpackTuple(args, "editops", 2, 3, &arg1, &arg2, &arg3)) {
    PyErr_Format(PyExc_ValueError,
                 "editops expected two Strings or two Unicodes");
    return NULL;
  }

  /* convert: we were called (bops, s1, s2) */
  if (arg3) {
    if (!PyList_Check(arg1)) {
      PyErr_Format(PyExc_ValueError,
                  "editops first argument must be a List of edit operations");
      return NULL;
    }
    n = PyList_GET_SIZE(arg1);
    if (!n) {
      Py_INCREF(arg1);
      return arg1;
    }
    len1 = get_length_of_anything(arg2);
    len2 = get_length_of_anything(arg3);
    if (len1 == (size_t)-1 || len2 == (size_t)-1) {
      PyErr_Format(PyExc_ValueError,
                  "editops second and third argument must specify sizes");
      return NULL;
    }

    if ((bops = extract_opcodes(arg1)) != NULL) {
      if (lev_opcodes_check_errors(len1, len2, n, bops)) {
        PyErr_Format(PyExc_ValueError,
                    "editops edit operation list is invalid");
        free(bops);
        return NULL;
      }
      ops = lev_opcodes_to_editops(n, bops, &n, 0); /* XXX: different n's! */
      if (!ops && n) {
        free(bops);
        return PyErr_NoMemory();
      }
      oplist = editops_to_tuple_list(n, ops);
      free(ops);
      free(bops);
      return oplist;
    }
    if ((ops = extract_editops(arg1)) != NULL) {
      if (lev_editops_check_errors(len1, len2, n, ops)) {
        PyErr_Format(PyExc_ValueError,
                    "editops edit operation list is invalid");
        free(ops);
        return NULL;
      }
      free(ops);
      Py_INCREF(arg1);  /* editops -> editops is identity */
      return arg1;
    }
    if (!PyErr_Occurred())
      PyErr_Format(PyExc_ValueError,
                  "editops first argument must be a List of edit operations");
    return NULL;
  }

  /* find editops: we were called (s1, s2) */
  if (PyObject_TypeCheck(arg1, &PyString_Type)
      && PyObject_TypeCheck(arg2, &PyString_Type)) {
    lev_byte *string1, *string2;

    len1 = PyString_GET_SIZE(arg1);
    len2 = PyString_GET_SIZE(arg2);
    string1 = PyString_AS_STRING(arg1);
    string2 = PyString_AS_STRING(arg2);
    ops = lev_editops_find(len1, string1, len2, string2, &n);
  }
  else if (PyObject_TypeCheck(arg1, &PyUnicode_Type)
      && PyObject_TypeCheck(arg2, &PyUnicode_Type)) {
    Py_UNICODE *string1, *string2;

    len1 = PyUnicode_GET_SIZE(arg1);
    len2 = PyUnicode_GET_SIZE(arg2);
    string1 = PyUnicode_AS_UNICODE(arg1);
    string2 = PyUnicode_AS_UNICODE(arg2);
    ops = lev_u_editops_find(len1, string1, len2, string2, &n);
  }
  else {
    PyErr_Format(PyExc_ValueError,
                 "editops expected two Strings or two Unicodes");
    return NULL;
  }
  if (!ops && n)
    return PyErr_NoMemory();
  oplist = editops_to_tuple_list(n, ops);
  free(ops);
  return oplist;
}

static PyObject*
opcodes_to_tuple_list(size_t nb, LevOpCode *bops)
{
  PyObject *list;
  size_t i;

  list = PyList_New(nb);
  for (i = 0; i < nb; i++, bops++) {
    PyObject *tuple = PyTuple_New(5);
    PyObject *is = opcode_names[bops->type].pystring;
    Py_INCREF(is);
    PyTuple_SET_ITEM(tuple, 0, is);
    PyTuple_SET_ITEM(tuple, 1, PyInt_FromLong((long)bops->sbeg));
    PyTuple_SET_ITEM(tuple, 2, PyInt_FromLong((long)bops->send));
    PyTuple_SET_ITEM(tuple, 3, PyInt_FromLong((long)bops->dbeg));
    PyTuple_SET_ITEM(tuple, 4, PyInt_FromLong((long)bops->dend));
    PyList_SET_ITEM(list, i, tuple);
  }

  return list;
}

static PyObject*
opcodes_py(PyObject *self, PyObject *args)
{
  PyObject *arg1, *arg2, *arg3 = NULL;
  PyObject *oplist;
  size_t len1, len2, n, nb;
  LevEditOp *ops;
  LevOpCode *bops;

  if (!PyArg_UnpackTuple(args, "opcodes", 2, 3, &arg1, &arg2, &arg3)) {
    PyErr_Format(PyExc_ValueError,
                 "opcodes expected two Strings or two Unicodes");
    return NULL;
  }

  /* convert: we were called (ops, s1, s2) */
  if (arg3) {
    if (!PyList_Check(arg1)) {
      PyErr_Format(PyExc_ValueError,
                  "opcodes first argument must be a List of edit operations");
      return NULL;
    }
    n = PyList_GET_SIZE(arg1);
    len1 = get_length_of_anything(arg2);
    len2 = get_length_of_anything(arg3);
    if (len1 == (size_t)-1 || len2 == (size_t)-1) {
      PyErr_Format(PyExc_ValueError,
                  "opcodes second and third argument must specify sizes");
      return NULL;
    }

    if ((ops = extract_editops(arg1)) != NULL) {
      if (lev_editops_check_errors(len1, len2, n, ops)) {
        PyErr_Format(PyExc_ValueError,
                    "opcodes edit operation list is invalid");
        free(ops);
        return NULL;
      }
      bops = lev_editops_to_opcodes(n, ops, &n, len1, len2);  /* XXX: n != n */
      if (!bops && n) {
        free(ops);
        return PyErr_NoMemory();
      }
      oplist = opcodes_to_tuple_list(n, bops);
      free(bops);
      free(ops);
      return oplist;
    }
    if ((bops = extract_opcodes(arg1)) != NULL) {
      if (lev_opcodes_check_errors(len1, len2, n, bops)) {
        PyErr_Format(PyExc_ValueError,
                    "opcodes edit operation list is invalid");
        free(bops);
        return NULL;
      }
      free(bops);
      Py_INCREF(arg1);  /* opcodes -> opcodes is identity */
      return arg1;
    }
    if (!PyErr_Occurred())
      PyErr_Format(PyExc_ValueError,
                  "opcodes first argument must be a List of edit operations");
    return NULL;
  }

  /* find opcodes: we were called (s1, s2) */
  if (PyObject_TypeCheck(arg1, &PyString_Type)
      && PyObject_TypeCheck(arg2, &PyString_Type)) {
    lev_byte *string1, *string2;

    len1 = PyString_GET_SIZE(arg1);
    len2 = PyString_GET_SIZE(arg2);
    string1 = PyString_AS_STRING(arg1);
    string2 = PyString_AS_STRING(arg2);
    ops = lev_editops_find(len1, string1, len2, string2, &n);
  }
  else if (PyObject_TypeCheck(arg1, &PyUnicode_Type)
      && PyObject_TypeCheck(arg2, &PyUnicode_Type)) {
    Py_UNICODE *string1, *string2;

    len1 = PyUnicode_GET_SIZE(arg1);
    len2 = PyUnicode_GET_SIZE(arg2);
    string1 = PyUnicode_AS_UNICODE(arg1);
    string2 = PyUnicode_AS_UNICODE(arg2);
    ops = lev_u_editops_find(len1, string1, len2, string2, &n);
  }
  else {
    PyErr_Format(PyExc_ValueError,
                 "opcodes expected two Strings or two Unicodes");
    return NULL;
  }
  if (!ops && n)
    return PyErr_NoMemory();
  bops = lev_editops_to_opcodes(n, ops, &nb, len1, len2);
  free(ops);
  if (!bops && nb)
    return PyErr_NoMemory();
  oplist = opcodes_to_tuple_list(nb, bops);
  free(bops);
  return oplist;
}

static PyObject*
inverse_py(PyObject *self, PyObject *args)
{
  PyObject *list, *result;
  size_t n;
  LevEditOp *ops;
  LevOpCode *bops;

  if (!PyArg_UnpackTuple(args, "inverse", 1, 1, &list)
      || !PyList_Check(list)) {
    PyErr_Format(PyExc_ValueError,
                 "inverse expected a List of edit operations");
    return NULL;
  }

  n = PyList_GET_SIZE(list);
  if (!n) {
    Py_INCREF(list);
    return list;
  }
  if ((ops = extract_editops(list)) != NULL) {
    lev_editops_inverse(n, ops);
    result = editops_to_tuple_list(n, ops);
    free(ops);
    return result;
  }
  if ((bops = extract_opcodes(list)) != NULL) {
    lev_opcodes_inverse(n, bops);
    result = opcodes_to_tuple_list(n, bops);
    free(bops);
    return result;
  }

  if (!PyErr_Occurred())
    PyErr_Format(PyExc_ValueError,
                "inverse expected a list of edit operations");
  return NULL;
}

static PyObject*
apply_edit_py(PyObject *self, PyObject *args)
{
  PyObject *list, *result, *arg1, *arg2;
  size_t n, len, len1, len2;
  LevEditOp *ops;
  LevOpCode *bops;

  if (!PyArg_UnpackTuple(args, "apply_edit", 3, 3, &list, &arg1, &arg2)) {
    PyErr_Format(PyExc_ValueError,
                 "apply_edit expected a three arguments");
    return NULL;
  }
  if (!PyList_Check(list)) {
    PyErr_Format(PyExc_ValueError,
                 "apply_edit first argument must be a List of edit operations");
    return NULL;
  }
  n = PyList_GET_SIZE(list);

  if (PyObject_TypeCheck(arg1, &PyString_Type)
      && PyObject_TypeCheck(arg2, &PyString_Type)) {
    lev_byte *string1, *string2, *s;

    if (!n) {
      Py_INCREF(arg1);
      return arg1;
    }
    len1 = PyString_GET_SIZE(arg1);
    len2 = PyString_GET_SIZE(arg2);
    string1 = PyString_AS_STRING(arg1);
    string2 = PyString_AS_STRING(arg2);

    if ((ops = extract_editops(list)) != NULL) {
      if (lev_editops_check_errors(len1, len2, n, ops)) {
        PyErr_Format(PyExc_ValueError,
                     "apply_edit edit oprations are invalid or inapplicable");
        free(ops);
        return NULL;
      }
      s = lev_editops_apply(len1, string1, len2, string2,
                            n, ops, &len);
      free(ops);
      if (!s && len)
        return PyErr_NoMemory();
      result = PyString_FromStringAndSize(s, len);
      free(s);
      return result;
    }
    if ((bops = extract_opcodes(list)) != NULL) {
      if (lev_opcodes_check_errors(len1, len2, n, bops)) {
        PyErr_Format(PyExc_ValueError,
                     "apply_edit edit oprations are invalid or inapplicable");
        free(bops);
        return NULL;
      }
      s = lev_opcodes_apply(len1, string1, len2, string2,
                            n, bops, &len);
      free(bops);
      if (!s && len)
        return PyErr_NoMemory();
      result = PyString_FromStringAndSize(s, len);
      free(s);
      return result;
    }

    if (!PyErr_Occurred())
      PyErr_Format(PyExc_ValueError,
                  "apply_edit first argument must be "
                  "a List of edit operations");
    return NULL;
  }
  if (PyObject_TypeCheck(arg1, &PyUnicode_Type)
      && PyObject_TypeCheck(arg2, &PyUnicode_Type)) {
    Py_UNICODE *string1, *string2, *s;

    if (!n) {
      Py_INCREF(arg1);
      return arg1;
    }
    len1 = PyUnicode_GET_SIZE(arg1);
    len2 = PyUnicode_GET_SIZE(arg2);
    string1 = PyUnicode_AS_UNICODE(arg1);
    string2 = PyUnicode_AS_UNICODE(arg2);

    if ((ops = extract_editops(list)) != NULL) {
      if (lev_editops_check_errors(len1, len2, n, ops)) {
        PyErr_Format(PyExc_ValueError,
                     "apply_edit edit oprations are invalid or inapplicable");
        free(ops);
        return NULL;
      }
      s = lev_u_editops_apply(len1, string1, len2, string2,
                              n, ops, &len);
      free(ops);
      if (!s && len)
        return PyErr_NoMemory();
      result = PyUnicode_FromUnicode(s, len);
      free(s);
      return result;
    }
    if ((bops = extract_opcodes(list)) != NULL) {
      if (lev_opcodes_check_errors(len1, len2, n, bops)) {
        PyErr_Format(PyExc_ValueError,
                     "apply_edit edit oprations are invalid or inapplicable");
        free(bops);
        return NULL;
      }
      s = lev_u_opcodes_apply(len1, string1, len2, string2,
                              n, bops, &len);
      free(bops);
      if (!s && len)
        return PyErr_NoMemory();
      result = PyUnicode_FromUnicode(s, len);
      free(s);
      return result;
    }

    if (!PyErr_Occurred())
      PyErr_Format(PyExc_ValueError,
                   "apply_edit first argument must be "
                   "a List of edit operations");
    return NULL;
  }

  PyErr_Format(PyExc_ValueError,
               "apply_edit expected two Strings or two Unicodes");
  return NULL;
}

static PyObject*
matching_blocks_py(PyObject *self, PyObject *args)
{
  PyObject *list, *arg1, *arg2, *result;
  size_t n, nmb, len1, len2;
  LevEditOp *ops;
  LevOpCode *bops;
  LevMatchingBlock *mblocks;

  if (!PyArg_UnpackTuple(args, "matching_blocks", 3, 3, &list, &arg1, &arg2)
      || !PyList_Check(list)) {
    PyErr_Format(PyExc_ValueError,
                 "matching_blocks expected a List of edit operations "
                 "and two sizes");
    return NULL;
  }

  if (!PyList_Check(list)) {
    PyErr_Format(PyExc_ValueError,
                 "matching_blocks first argument must be "
                 "a List of edit operations");
    return NULL;
  }
  n = PyList_GET_SIZE(list);
  len1 = get_length_of_anything(arg1);
  len2 = get_length_of_anything(arg2);
  if (len1 == (size_t)-1 || len2 == (size_t)-1) {
    PyErr_Format(PyExc_ValueError,
                 "matching_blocks second and third argument "
                 "must specify sizes");
    return NULL;
  }

  if ((ops = extract_editops(list)) != NULL) {
    if (lev_editops_check_errors(len1, len2, n, ops)) {
      PyErr_Format(PyExc_ValueError,
                   "apply_edit edit oprations are invalid or inapplicable");
      free(ops);
      return NULL;
    }
    mblocks = lev_editops_matching_blocks(len1, len2, n, ops, &nmb);
    free(ops);
    if (!mblocks && nmb)
      return PyErr_NoMemory();
    result = matching_blocks_to_tuple_list(len1, len2, nmb, mblocks);
    free(mblocks);
    return result;
  }
  if ((bops = extract_opcodes(list)) != NULL) {
    if (lev_opcodes_check_errors(len1, len2, n, bops)) {
      PyErr_Format(PyExc_ValueError,
                   "apply_edit edit oprations are invalid or inapplicable");
      free(bops);
      return NULL;
    }
    mblocks = lev_opcodes_matching_blocks(len1, len2, n, bops, &nmb);
    free(bops);
    if (!mblocks && nmb)
      return PyErr_NoMemory();
    result = matching_blocks_to_tuple_list(len1, len2, nmb, mblocks);
    free(mblocks);
    return result;
  }

  if (!PyErr_Occurred())
    PyErr_Format(PyExc_ValueError,
                "inverse expected a list of edit operations");
  return NULL;
}


void
initLevenshtein(void)
{
  PyObject *module;
  size_t i;

  module = Py_InitModule3("Levenshtein", methods, Levenshtein_DESC);
  /* create intern strings for edit operation names */
  if (opcode_names[0].pystring)
    abort();
  for (i = 0; i < N_OPCODE_NAMES; i++) {
    opcode_names[i].pystring
      = PyString_InternFromString(opcode_names[i].cstring);
    opcode_names[i].len = strlen(opcode_names[i].cstring);
  }
}
#endif /* not NO_PYTHON */

/****************************************************************************
 *
 * C (i.e. executive) part
 *
 ****************************************************************************/

/****************************************************************************
 *
 * Basic stuff, Levenshtein distance
 *
 ****************************************************************************/

#define EPSILON 1e-14

/*
 * Levenshtein distance between string1 and string2.
 *
 * Replace cost is normally 1, and 2 with nonzero xcost.
 */
LEV_STATIC_PY size_t
lev_edit_distance(size_t len1, const lev_byte *string1,
                  size_t len2, const lev_byte *string2,
                  size_t xcost)
{
  size_t i;
  size_t *row;  /* we only need to keep one row of costs */
  size_t *end;
  size_t half;

  /* strip common prefix */
  while (len1 > 0 && len2 > 0 && *string1 == *string2) {
    len1--;
    len2--;
    string1++;
    string2++;
  }

  /* strip common suffix */
  while (len1 > 0 && len2 > 0 && string1[len1-1] == string2[len2-1]) {
    len1--;
    len2--;
  }

  /* catch trivial cases */
  if (len1 == 0)
    return len2;
  if (len2 == 0)
    return len1;

  /* make the inner cycle (i.e. string2) the longer one */
  if (len1 > len2) {
    size_t nx = len1;
    const lev_byte *sx = string1;
    len1 = len2;
    len2 = nx;
    string1 = string2;
    string2 = sx;
  }
  /* check len1 == 1 separately */
  if (len1 == 1) {
    if (xcost)
      return len2 + 1 - 2*(memchr(string2, *string1, len2) != NULL);
    else
      return len2 - (memchr(string2, *string1, len2) != NULL);
  }
  len1++;
  len2++;
  half = len1 >> 1;

  /* initalize first row */
  row = (size_t*)malloc(len2*sizeof(size_t));
  if (!row)
    return (size_t)(-1);
  end = row + len2 - 1;
  for (i = 0; i < len2 - (xcost ? 0 : half); i++)
    row[i] = i;

  /* go through the matrix and compute the costs.  yes, this is an extremely
   * obfuscated version, but also extremely memory-conservative and relatively
   * fast.  */
  if (xcost) {
    for (i = 1; i < len1; i++) {
      size_t *p = row + 1;
      const lev_byte char1 = string1[i - 1];
      const lev_byte *char2p = string2;
      size_t D = i;
      size_t x = i;
      while (p <= end) {
        if (char1 == *(char2p++))
          x = --D;
        else
          x++;
        D = *p;
        D++;
        if (x > D)
          x = D;
        *(p++) = x;
      }
    }
  }
  else {
    /* in this case we don't have to scan two corner triangles (of size len1/2)
     * in the matrix because no best path can go throught them. note this
     * breaks when len1 == len2 == 2 so the memchr() special case above is
     * necessary */
    row[0] = len1 - half - 1;
    for (i = 1; i < len1; i++) {
      size_t *p;
      const lev_byte char1 = string1[i - 1];
      const lev_byte *char2p;
      size_t D, x;
      /* skip the upper triangle */
      if (i >= len1 - half) {
        size_t offset = i - (len1 - half);
        size_t c3;

        char2p = string2 + offset;
        p = row + offset;
        c3 = *(p++) + (char1 != *(char2p++));
        x = *p;
        x++;
        D = x;
        if (x > c3)
          x = c3;
        *(p++) = x;
      }
      else {
        p = row + 1;
        char2p = string2;
        D = x = i;
      }
      /* skip the lower triangle */
      if (i <= half + 1)
        end = row + len2 + i - half - 2;
      /* main */
      while (p <= end) {
        size_t c3 = --D + (char1 != *(char2p++));
        x++;
        if (x > c3)
          x = c3;
        D = *p;
        D++;
        if (x > D)
          x = D;
        *(p++) = x;
      }
      /* lower triangle sentinel */
      if (i <= half) {
        size_t c3 = --D + (char1 != *char2p);
        x++;
        if (x > c3)
          x = c3;
        *p = x;
      }
    }
  }

  i = *end;
  free(row);
  return i;
}

/*
 * Levenshtein distance between string1 and string2 (Unicode).
 *
 * Replace cost is normally 1, and 2 with nonzero xcost.
 */
LEV_STATIC_PY size_t
lev_u_edit_distance(size_t len1, const lev_wchar *string1,
                    size_t len2, const lev_wchar *string2,
                    size_t xcost)
{
  size_t i;
  size_t *row;  /* we only need to keep one row of costs */
  size_t *end;
  size_t half;

  /* strip common prefix */
  while (len1 > 0 && len2 > 0 && *string1 == *string2) {
    len1--;
    len2--;
    string1++;
    string2++;
  }

  /* strip common suffix */
  while (len1 > 0 && len2 > 0 && string1[len1-1] == string2[len2-1]) {
    len1--;
    len2--;
  }

  /* catch trivial cases */
  if (len1 == 0)
    return len2;
  if (len2 == 0)
    return len1;

  /* make the inner cycle (i.e. string2) the longer one */
  if (len1 > len2) {
    size_t nx = len1;
    const lev_wchar *sx = string1;
    len1 = len2;
    len2 = nx;
    string1 = string2;
    string2 = sx;
  }
  /* check len1 == 1 separately */
  if (len1 == 1) {
    lev_wchar z = *string1;
    const lev_wchar *p = string2;
    for (i = len2; i; i--) {
      if (*(p++) == z)
        return len2 - 1;
    }
    return len2 + (xcost != 0);
  }
  len1++;
  len2++;
  half = len1 >> 1;

  /* initalize first row */
  row = (size_t*)malloc(len2*sizeof(size_t));
  if (!row)
    return (size_t)(-1);
  end = row + len2 - 1;
  for (i = 0; i < len2 - (xcost ? 0 : half); i++)
    row[i] = i;

  /* go through the matrix and compute the costs.  yes, this is an extremely
   * obfuscated version, but also extremely memory-conservative and relatively
   * fast.  */
  if (xcost) {
    for (i = 1; i < len1; i++) {
      size_t *p = row + 1;
      const lev_wchar char1 = string1[i - 1];
      const lev_wchar *char2p = string2;
      size_t D = i - 1;
      size_t x = i;
      while (p <= end) {
        if (char1 == *(char2p++))
          x = D;
        else
          x++;
        D = *p;
        if (x > D + 1)
          x = D + 1;
        *(p++) = x;
      }
    }
  }
  else {
    /* in this case we don't have to scan two corner triangles (of size len1/2)
     * in the matrix because no best path can go throught them. note this
     * breaks when len1 == len2 == 2 so the memchr() special case above is
     * necessary */
    row[0] = len1 - half - 1;
    for (i = 1; i < len1; i++) {
      size_t *p;
      const lev_wchar char1 = string1[i - 1];
      const lev_wchar *char2p;
      size_t D, x;
      /* skip the upper triangle */
      if (i >= len1 - half) {
        size_t offset = i - (len1 - half);
        size_t c3;

        char2p = string2 + offset;
        p = row + offset;
        c3 = *(p++) + (char1 != *(char2p++));
        x = *p;
        x++;
        D = x;
        if (x > c3)
          x = c3;
        *(p++) = x;
      }
      else {
        p = row + 1;
        char2p = string2;
        D = x = i;
      }
      /* skip the lower triangle */
      if (i <= half + 1)
        end = row + len2 + i - half - 2;
      /* main */
      while (p <= end) {
        size_t c3 = --D + (char1 != *(char2p++));
        x++;
        if (x > c3)
          x = c3;
        D = *p;
        D++;
        if (x > D)
          x = D;
        *(p++) = x;
      }
      /* lower triangle sentinel */
      if (i <= half) {
        size_t c3 = --D + (char1 != *char2p);
        x++;
        if (x > c3)
          x = c3;
        *p = x;
      }
    }
  }

  i = *end;
  free(row);
  return i;
}

/****************************************************************************
 *
 * Medians (greedy, set, ...)
 *
 ****************************************************************************/

/* compute the sets of symbols each string contains, and the set of symbols
  * in any of them (symset).  meanwhile, count how many different symbols
  * there are (used below for symlist). */
static lev_byte*
make_symlist(size_t n, const size_t *lengths,
             const lev_byte *strings[], size_t *symlistlen)
{
  short int *symset;  /* indexed by ALL symbols, contains 1 for symbols
                         present in the strings, zero for others */
  size_t i, j;
  lev_byte *symlist;

  symset = calloc(0x100, sizeof(short int));
  if (!symset) {
    *symlistlen = (size_t)(-1);
    return NULL;
  }
  *symlistlen = 0;
  for (i = 0; i < n; i++) {
    const lev_byte *stri = strings[i];
    for (j = 0; j < lengths[i]; j++) {
      int c = stri[j];
      if (!symset[c])
        (*symlistlen)++;
      symset[c] = 1;
    }
  }
  if (!*symlistlen) {
    free(symset);
    return NULL;
  }

  /* create dense symbol table, so we can easily iterate over only characters
   * present in the strings */
  {
    size_t pos = 0;
    symlist = (lev_byte*)malloc((*symlistlen)*sizeof(lev_byte));
    if (!symlist) {
      *symlistlen = (size_t)(-1);
      free(symset);
      return NULL;
    }
    for (j = 0; j < 0x100; j++) {
      if (symset[j])
        symlist[pos++] = (lev_byte)j;
    }
  }
  free(symset);

  return symlist;
}

/*
 * Generalized set median, using greedy algorithm for string set strings[].
 *
 * Returns a newly allocated string.
 */
LEV_STATIC_PY lev_byte*
lev_greedy_median(size_t n, const size_t *lengths,
                  const lev_byte *strings[],
                  const double *weights,
                  size_t *medlength)
{
  size_t i;  /* usually iterates over strings (n) */
  size_t j;  /* usually iterates over characters */
  size_t len;  /* usually iterates over the approximate median string */
  lev_byte *symlist;  /* list of symbols present in the strings,
                              we iterate over it insead of set of all
                              existing symbols */
  size_t symlistlen;  /* length of symlist */
  size_t maxlen;  /* maximum input string length */
  size_t stoplen;  /* maximum tried median string length -- this is slightly
                      higher than maxlen, because the median string may be
                      longer than any of the input strings */
  size_t **rows;  /* Levenshtein matrix rows for each string, we need to keep
                     only one previous row to construct the current one */
  size_t *row;  /* a scratch buffer for new Levenshtein matrix row computation,
                   shared among all strings */
  lev_byte *median;  /* the resulting approximate median string */
  double *mediandist;  /* the total distance of the best median string of
                          given length.  warning!  mediandist[0] is total
                          distance for empty string, while median[] itself
                          is normally zero-based */
  size_t bestlen;  /* the best approximate median string length */

  /* find all symbols */
  symlist = make_symlist(n, lengths, strings, &symlistlen);
  if (!symlist) {
    *medlength = 0;
    if (symlistlen != 0)
      return NULL;
    else
      return calloc(1, sizeof(lev_byte));
  }

  /* allocate and initialize per-string matrix rows and a common work buffer */
  rows = (size_t**)malloc(n*sizeof(size_t*));
  if (!rows) {
    free(symlist);
    return NULL;
  }
  maxlen = 0;
  for (i = 0; i < n; i++) {
    size_t *ri;
    size_t leni = lengths[i];
    if (leni > maxlen)
      maxlen = leni;
    ri = rows[i] = (size_t*)malloc((leni + 1)*sizeof(size_t));
    if (!ri) {
      for (j = 0; j < i; j++)
        free(rows[j]);
      free(rows);
      free(symlist);
      return NULL;
    }
    for (j = 0; j <= leni; j++)
      ri[j] = j;
  }
  stoplen = 2*maxlen + 1;
  row = (size_t*)malloc((stoplen + 1)*sizeof(size_t));
  if (!row) {
    for (j = 0; j < n; j++)
      free(rows[j]);
    free(rows);
    free(symlist);
    return NULL;
  }

  /* compute final cost of string of length 0 (empty string may be also
   * a valid answer) */
  median = (lev_byte*)malloc(stoplen*sizeof(lev_byte));
  if (!median) {
    for (j = 0; j < n; j++)
      free(rows[j]);
    free(rows);
    free(row);
    free(symlist);
    return NULL;
  }
  mediandist = (double*)malloc((stoplen + 1)*sizeof(double));
  if (!mediandist) {
    for (j = 0; j < n; j++)
      free(rows[j]);
    free(rows);
    free(row);
    free(symlist);
    free(median);
    return NULL;
  }
  mediandist[0] = 0.0;
  for (i = 0; i < n; i++)
    mediandist[0] += lengths[i]*weights[i];

  /* build up the approximate median string symbol by symbol
   * XXX: we actually exit on break below, but on the same condition */
  for (len = 1; len <= stoplen; len++) {
    lev_byte symbol;
    double minminsum = 1e100;
    row[0] = len;
    /* iterate over all symbols we may want to add */
    for (j = 0; j < symlistlen; j++) {
      double totaldist = 0.0;
      double minsum = 0.0;
      symbol = symlist[j];
      /* sum Levenshtein distances from all the strings, with given weights */
      for (i = 0; i < n; i++) {
        const lev_byte *stri = strings[i];
        size_t *p = rows[i];
        size_t leni = lengths[i];
        size_t *end = rows[i] + leni;
        size_t min = len;
        size_t x = len; /* == row[0] */
        /* compute how another row of Levenshtein matrix would look for median
         * string with this symbol added */
        while (p < end) {
          size_t D = *(p++) + (symbol != *(stri++));
          x++;
          if (x > D)
            x = D;
          if (x > *p + 1)
            x = *p + 1;
          if (x < min)
            min = x;
        }
        minsum += min*weights[i];
        totaldist += x*weights[i];
      }
      /* is this symbol better than all the others? */
      if (minsum < minminsum) {
        minminsum = minsum;
        mediandist[len] = totaldist;
        median[len - 1] = symbol;
      }
    }
    /* stop the iteration if we no longer need to recompute the matrix rows
     * or when we are over maxlen and adding more characters doesn't seem
     * useful */
    if (len == stoplen
        || (len > maxlen && mediandist[len] > mediandist[len - 1])) {
      stoplen = len;
      break;
    }
    /* now the best symbol is known, so recompute all matrix rows for this
     * one */
    symbol = median[len - 1];
    for (i = 0; i < n; i++) {
      const lev_byte *stri = strings[i];
      size_t *oldrow = rows[i];
      size_t leni = lengths[i];
      size_t k;
      /* compute a row of Levenshtein matrix */
      for (k = 1; k <= leni; k++) {
        size_t c1 = oldrow[k] + 1;
        size_t c2 = row[k - 1] + 1;
        size_t c3 = oldrow[k - 1] + (symbol != stri[k - 1]);
        row[k] = c2 > c3 ? c3 : c2;
        if (row[k] > c1)
          row[k] = c1;
      }
      memcpy(oldrow, row, (leni + 1)*sizeof(size_t));
    }
  }

  /* find the string with minimum total distance */
  bestlen = 0;
  for (len = 1; len <= stoplen; len++) {
    if (mediandist[len] < mediandist[bestlen])
      bestlen = len;
  }

  /* clean up */
  for (i = 0; i < n; i++)
    free(rows[i]);
  free(rows);
  free(row);
  free(symlist);
  free(mediandist);

  /* return result */
  {
    lev_byte *result = (lev_byte*)malloc(bestlen*sizeof(lev_byte));
    if (!result) {
      free(median);
      return NULL;
    }
    memcpy(result, median, bestlen*sizeof(lev_byte));
    free(median);
    *medlength = bestlen;
    return result;
  }
}

/*
 * Knowing the distance matrices up to some row, finish the distance
 * computations.
 *
 * string1, len1 are already shortened.
 */
static double
finish_distance_computations(size_t len1, lev_byte *string1,
                             size_t n, const size_t *lenghts,
                             const lev_byte **strings,
                             const double *weights, size_t **rows,
                             size_t *row)
{
  size_t *end;
  size_t i, j;
  size_t offset;  /* row[0]; offset + len1 give together real len of string1 */
  double distsum = 0.0;  /* sum of distances */

  /* catch trivia case */
  if (len1 == 0) {
    for (j = 0; j < n; j++)
      distsum += rows[j][lenghts[j]]*weights[j];
    return distsum;
  }

  /* iterate through the strings and sum the distances */
  for (j = 0; j < n; j++) {
    size_t *rowi = rows[j];  /* current row */
    size_t leni = lenghts[j];  /* current length */
    size_t len = len1;  /* temporary len1 for suffix stripping */
    const lev_byte *stringi = strings[j];  /* current string */

    /* strip common suffix (prefix CAN'T be stripped) */
    while (len && leni && stringi[leni-1] == string1[len-1]) {
      len--;
      leni--;
    }

    /* catch trivial cases */
    if (len == 0) {
      distsum += rowi[leni]*weights[j];
      continue;
    }
    offset = rowi[0];
    if (leni == 0) {
      distsum += (offset + len)*weights[j];
      continue;
    }

    /* complete the matrix */
    memcpy(row, rowi, (leni + 1)*sizeof(size_t));
    end = row + leni;

    for (i = 1; i <= len; i++) {
      size_t *p = row + 1;
      const lev_byte char1 = string1[i - 1];
      const lev_byte *char2p = stringi;
      size_t D, x;

      D = x = i + offset;
      while (p <= end) {
        size_t c3 = --D + (char1 != *(char2p++));
        x++;
        if (x > c3)
          x = c3;
        D = *p;
        D++;
        if (x > D)
          x = D;
        *(p++) = x;
      }
    }
    distsum += weights[j]*(*end);
  }

  return distsum;
}

/*
 * Try to improve a generalized set median with one-character perturbations.
 *
 * Returns a newly allocated string.
 */
LEV_STATIC_PY lev_byte*
lev_median_improve(size_t len, const lev_byte *s,
                   size_t n, const size_t *lengths,
                   const lev_byte *strings[],
                   const double *weights,
                   size_t *medlength)
{
  size_t i;  /* usually iterates over strings (n) */
  size_t j;  /* usually iterates over characters */
  size_t pos;  /* the position in the approximate median string we are
                  trying to change */
  lev_byte *symlist;  /* list of symbols present in the strings,
                              we iterate over it insead of set of all
                              existing symbols */
  size_t symlistlen;  /* length of symlist */
  size_t maxlen;  /* maximum input string length */
  size_t stoplen;  /* maximum tried median string length -- this is slightly
                      higher than maxlen, because the median string may be
                      longer than any of the input strings */
  size_t **rows;  /* Levenshtein matrix rows for each string, we need to keep
                     only one previous row to construct the current one */
  size_t *row;  /* a scratch buffer for new Levenshtein matrix row computation,
                   shared among all strings */
  lev_byte *median;  /* the resulting approximate median string */
  size_t medlen;  /* the current approximate median string length */
  double minminsum;  /* the current total distance sum */

  /* find all symbols */
  symlist = make_symlist(n, lengths, strings, &symlistlen);
  if (!symlist) {
    *medlength = 0;
    if (symlistlen != 0)
      return NULL;
    else
      return calloc(1, sizeof(lev_byte));
  }

  /* allocate and initialize per-string matrix rows and a common work buffer */
  rows = (size_t**)malloc(n*sizeof(size_t*));
  if (!rows) {
    free(symlist);
    return NULL;
  }
  maxlen = 0;
  for (i = 0; i < n; i++) {
    size_t *ri;
    size_t leni = lengths[i];
    if (leni > maxlen)
      maxlen = leni;
    ri = rows[i] = (size_t*)malloc((leni + 1)*sizeof(size_t));
    if (!ri) {
      for (j = 0; j < i; j++)
        free(rows[j]);
      free(rows);
      free(symlist);
      return NULL;
    }
    for (j = 0; j <= leni; j++)
      ri[j] = j;
  }
  stoplen = 2*maxlen + 1;
  row = (size_t*)malloc((stoplen + 2)*sizeof(size_t));
  if (!row) {
    for (j = 0; j < n; j++)
      free(rows[j]);
    free(rows);
    free(symlist);
    return NULL;
  }

  /* initialize median to given string */
  median = (lev_byte*)malloc((stoplen+1)*sizeof(lev_byte));
  if (!median) {
    for (j = 0; j < n; j++)
      free(rows[j]);
    free(rows);
    free(row);
    free(symlist);
    return NULL;
  }
  median++;  /* we need -1st element for insertions a pos 0 */
  medlen = len;
  memcpy(median, s, (medlen)*sizeof(lev_byte));
  minminsum = finish_distance_computations(medlen, median,
                                           n, lengths, strings,
                                           weights, rows, row);

  /* sequentially try perturbations on all positions */
  for (pos = 0; pos <= medlen; ) {
    lev_byte orig_symbol, symbol;
    LevEditType operation;
    double sum;

    symbol = median[pos];
    operation = LEV_EDIT_KEEP;
    /* IF pos < medlength: FOREACH symbol: try to replace the symbol
     * at pos, if some lower the total distance, chooste the best */
    if (pos < medlen) {
      orig_symbol = median[pos];
      for (j = 0; j < symlistlen; j++) {
        if (symlist[j] == orig_symbol)
          continue;
        median[pos] = symlist[j];
        sum = finish_distance_computations(medlen - pos, median + pos,
                                           n, lengths, strings,
                                           weights, rows, row);
        if (sum < minminsum) {
          minminsum = sum;
          symbol = symlist[j];
          operation = LEV_EDIT_REPLACE;
        }
      }
      median[pos] = orig_symbol;
    }
    /* FOREACH symbol: try to add it at pos, if some lower the total
     * distance, chooste the best (increase medlength)
     * We simulate insertion by replacing the character at pos-1 */
    orig_symbol = *(median + pos - 1);
    for (j = 0; j < symlistlen; j++) {
      *(median + pos - 1) = symlist[j];
      sum = finish_distance_computations(medlen - pos + 1, median + pos - 1,
                                          n, lengths, strings,
                                         weights, rows, row);
      if (sum < minminsum) {
        minminsum = sum;
        symbol = symlist[j];
        operation = LEV_EDIT_INSERT;
      }
    }
    *(median + pos - 1) = orig_symbol;
    /* IF pos < medlength: try to delete the symbol at pos, if it lowers
     * the total distance remember it (decrease medlength) */
    if (pos < medlen) {
      sum = finish_distance_computations(medlen - pos - 1, median + pos + 1,
                                         n, lengths, strings,
                                         weights, rows, row);
      if (sum < minminsum) {
        minminsum = sum;
        operation = LEV_EDIT_DELETE;
      }
    }
    /* actually perform the operation */
    switch (operation) {
      case LEV_EDIT_REPLACE:
      median[pos] = symbol;
      break;

      case LEV_EDIT_INSERT:
      memmove(median+pos+1, median+pos,
              (medlen - pos)*sizeof(lev_byte));
      median[pos] = symbol;
      medlen++;
      break;

      case LEV_EDIT_DELETE:
      memmove(median+pos, median + pos+1,
              (medlen - pos-1)*sizeof(lev_byte));
      medlen--;
      break;

      default:
      break;
    }
    assert(medlen <= stoplen);
    /* now the result is known, so recompute all matrix rows and move on */
    if (operation != LEV_EDIT_DELETE) {
      symbol = median[pos];
      row[0] = pos + 1;
      for (i = 0; i < n; i++) {
        const lev_byte *stri = strings[i];
        size_t *oldrow = rows[i];
        size_t leni = lengths[i];
        size_t k;
        /* compute a row of Levenshtein matrix */
        for (k = 1; k <= leni; k++) {
          size_t c1 = oldrow[k] + 1;
          size_t c2 = row[k - 1] + 1;
          size_t c3 = oldrow[k - 1] + (symbol != stri[k - 1]);
          row[k] = c2 > c3 ? c3 : c2;
          if (row[k] > c1)
            row[k] = c1;
        }
        memcpy(oldrow, row, (leni + 1)*sizeof(size_t));
      }
      pos++;
    }
  }

  /* clean up */
  for (i = 0; i < n; i++)
    free(rows[i]);
  free(rows);
  free(row);
  free(symlist);

  /* return result */
  {
    lev_byte *result = (lev_byte*)malloc(medlen*sizeof(lev_byte));
    if (!result) {
      free(median);
      return NULL;
    }
    *medlength = medlen;
    memcpy(result, median, medlen*sizeof(lev_byte));
    median--;
    free(median);
    return result;
  }
}

/* used internally in make_usymlist */
typedef struct _HItem HItem;
struct _HItem {
  lev_wchar c;
  HItem *n;
};

/* free usmylist hash
 * this is a separate function because we need it in more than one place */
static void
free_usymlist_hash(HItem *symmap)
{
  size_t j;

  for (j = 0; j < 0x100; j++) {
    HItem *p = symmap + j;
    if (p->n == symmap || p->n == NULL)
      continue;
    p = p->n;
    while (p) {
      HItem *q = p;
      p = p->n;
      free(q);
    }
  }
  free(symmap);
}

/* compute the sets of symbols each string contains, and the set of symbols
  * in any of them (symset).  meanwhile, count how many different symbols
  * there are (used below for symlist). */
static lev_wchar*
make_usymlist(size_t n, const size_t *lengths,
              const lev_wchar *strings[], size_t *symlistlen)
{
  lev_wchar *symlist;
  size_t i, j;
  HItem *symmap;

  j = 0;
  for (i = 0; i < n; i++)
    j += lengths[i];

  *symlistlen = 0;
  if (j == 0)
    return NULL;

  /* find all symbols, use a kind of hash for storage */
  symmap = (HItem*)malloc(0x100*sizeof(HItem));
  if (!symmap) {
    *symlistlen = (size_t)(-1);
    return NULL;
  }
  /* this is an ugly memory allocation avoiding hack: most hash elements
   * will probably contain none or one symbols only so, when p->n is equal
   * to symmap, it means there're no symbols yet, afters insterting the
   * first one, p->n becomes normally NULL and then it behaves like an
   * usual singly linked list */
  for (i = 0; i < 0x100; i++)
    symmap[i].n = symmap;
  for (i = 0; i < n; i++) {
    const lev_wchar *stri = strings[i];
    for (j = 0; j < lengths[i]; j++) {
      int c = stri[j];
      int key = (c + (c >> 7)) & 0xff;
      HItem *p = symmap + key;
      if (p->n == symmap) {
        p->c = c;
        p->n = NULL;
        (*symlistlen)++;
        continue;
      }
      while (p->c != c && p->n != NULL)
        p = p->n;
      if (p->c != c) {
        p->n = (HItem*)malloc(sizeof(HItem));
        if (!p->n) {
          free_usymlist_hash(symmap);
          *symlistlen = (size_t)(-1);
          return NULL;
        }
        p = p->n;
        p->n = NULL;
        p->c = c;
        (*symlistlen)++;
      }
    }
  }
  /* create dense symbol table, so we can easily iterate over only characters
   * present in the strings */
  {
    size_t pos = 0;
    symlist = (lev_wchar*)malloc((*symlistlen)*sizeof(lev_wchar));
    if (!symlist) {
      free_usymlist_hash(symmap);
      *symlistlen = (size_t)(-1);
      return NULL;
    }
    for (j = 0; j < 0x100; j++) {
      HItem *p = symmap + j;
      while (p != NULL && p->n != symmap) {
        symlist[pos++] = p->c;
        p = p->n;
      }
    }
  }

  /* free memory */
  free_usymlist_hash(symmap);

  return symlist;
}

/*
 * Generalized set median, using greedy algorithm for string set strings[]
 * (Unicode)
 *
 * Returns a newly allocated string.
 */
LEV_STATIC_PY lev_wchar*
lev_u_greedy_median(size_t n, const size_t *lengths,
                    const lev_wchar *strings[],
                    const double *weights,
                    size_t *medlength)
{
  size_t i;  /* usually iterates over strings (n) */
  size_t j;  /* usually iterates over characters */
  size_t len;  /* usually iterates over the approximate median string */
  lev_wchar *symlist;  /* list of symbols present in the strings,
                              we iterate over it insead of set of all
                              existing symbols */
  size_t symlistlen;  /* length of symlistle */
  size_t maxlen;  /* maximum input string length */
  size_t stoplen;  /* maximum tried median string length -- this is slightly
                      higher than maxlen, because the median string may be
                      longer than any of the input strings */
  size_t **rows;  /* Levenshtein matrix rows for each string, we need to keep
                     only one previous row to construct the current one */
  size_t *row;  /* a scratch buffer for new Levenshtein matrix row computation,
                   shared among all strings */
  lev_wchar *median;  /* the resulting approximate median string */
  double *mediandist;  /* the total distance of the best median string of
                          given length.  warning!  mediandist[0] is total
                          distance for empty string, while median[] itself
                          is normally zero-based */
  size_t bestlen;  /* the best approximate median string length */

  /* find all symbols */
  symlist = make_usymlist(n, lengths, strings, &symlistlen);
  if (!symlist) {
    *medlength = 0;
    if (symlistlen != 0)
      return NULL;
    else
      return calloc(1, sizeof(lev_wchar));
  }

  /* allocate and initialize per-string matrix rows and a common work buffer */
  rows = (size_t**)malloc(n*sizeof(size_t*));
  if (!rows) {
    free(symlist);
    return NULL;
  }
  maxlen = 0;
  for (i = 0; i < n; i++) {
    size_t *ri;
    size_t leni = lengths[i];
    if (leni > maxlen)
      maxlen = leni;
    ri = rows[i] = (size_t*)malloc((leni + 1)*sizeof(size_t));
    if (!ri) {
      for (j = 0; j < i; j++)
        free(rows[j]);
      free(rows);
      free(symlist);
      return NULL;
    }
    for (j = 0; j <= leni; j++)
      ri[j] = j;
  }
  stoplen = 2*maxlen + 1;
  row = (size_t*)malloc((stoplen + 1)*sizeof(size_t));
  if (!row) {
    for (j = 0; j < n; j++)
      free(rows[j]);
    free(rows);
    free(symlist);
    return NULL;
  }

  /* compute final cost of string of length 0 (empty string may be also
   * a valid answer) */
  median = (lev_wchar*)malloc(stoplen*sizeof(lev_wchar));
  if (!median) {
    for (j = 0; j < n; j++)
      free(rows[j]);
    free(rows);
    free(row);
    free(symlist);
    return NULL;
  }
  mediandist = (double*)malloc((stoplen + 1)*sizeof(double));
  if (!mediandist) {
    for (j = 0; j < n; j++)
      free(rows[j]);
    free(rows);
    free(row);
    free(symlist);
    free(median);
    return NULL;
  }
  mediandist[0] = 0.0;
  for (i = 0; i < n; i++)
    mediandist[0] += lengths[i]*weights[i];

  /* build up the approximate median string symbol by symbol
   * XXX: we actually exit on break below, but on the same condition */
  for (len = 1; len <= stoplen; len++) {
    lev_wchar symbol;
    double minminsum = 1e100;
    row[0] = len;
    /* iterate over all symbols we may want to add */
    for (j = 0; j < symlistlen; j++) {
      double totaldist = 0.0;
      double minsum = 0.0;
      symbol = symlist[j];
      /* sum Levenshtein distances from all the strings, with given weights */
      for (i = 0; i < n; i++) {
        const lev_wchar *stri = strings[i];
        size_t *p = rows[i];
        size_t leni = lengths[i];
        size_t *end = rows[i] + leni;
        size_t min = len;
        size_t x = len; /* == row[0] */
        /* compute how another row of Levenshtein matrix would look for median
         * string with this symbol added */
        while (p < end) {
          size_t D = *(p++) + (symbol != *(stri++));
          x++;
          if (x > D)
            x = D;
          if (x > *p + 1)
            x = *p + 1;
          if (x < min)
            min = x;
        }
        minsum += min*weights[i];
        totaldist += x*weights[i];
      }
      /* is this symbol better than all the others? */
      if (minsum < minminsum) {
        minminsum = minsum;
        mediandist[len] = totaldist;
        median[len - 1] = symbol;
      }
    }
    /* stop the iteration if we no longer need to recompute the matrix rows
     * or when we are over maxlen and adding more characters doesn't seem
     * useful */
    if (len == stoplen
        || (len > maxlen && mediandist[len] > mediandist[len - 1])) {
      stoplen = len;
      break;
    }
    /* now the best symbol is known, so recompute all matrix rows for this
     * one */
    symbol = median[len - 1];
    for (i = 0; i < n; i++) {
      const lev_wchar *stri = strings[i];
      size_t *oldrow = rows[i];
      size_t leni = lengths[i];
      size_t k;
      /* compute a row of Levenshtein matrix */
      for (k = 1; k <= leni; k++) {
        size_t c1 = oldrow[k] + 1;
        size_t c2 = row[k - 1] + 1;
        size_t c3 = oldrow[k - 1] + (symbol != stri[k - 1]);
        row[k] = c2 > c3 ? c3 : c2;
        if (row[k] > c1)
          row[k] = c1;
      }
      memcpy(oldrow, row, (leni + 1)*sizeof(size_t));
    }
  }

  /* find the string with minimum total distance */
  bestlen = 0;
  for (len = 1; len <= stoplen; len++) {
    if (mediandist[len] < mediandist[bestlen])
      bestlen = len;
  }

  /* clean up */
  for (i = 0; i < n; i++)
    free(rows[i]);
  free(rows);
  free(row);
  free(symlist);
  free(mediandist);

  /* return result */
  {
    lev_wchar *result = (lev_wchar*)malloc(bestlen*sizeof(lev_wchar));
    if (!result) {
      free(median);
      return NULL;
    }
    memcpy(result, median, bestlen*sizeof(lev_wchar));
    free(median);
    *medlength = bestlen;
    return result;
  }
}

/*
 * Knowing the distance matrices up to some row, finish the distance
 * computations.
 *
 * string1, len1 are already shortened.
 */
static double
finish_udistance_computations(size_t len1, lev_wchar *string1,
                             size_t n, const size_t *lenghts,
                             const lev_wchar **strings,
                             const double *weights, size_t **rows,
                             size_t *row)
{
  size_t *end;
  size_t i, j;
  size_t offset;  /* row[0]; offset + len1 give together real len of string1 */
  double distsum = 0.0;  /* sum of distances */

  /* catch trivia case */
  if (len1 == 0) {
    for (j = 0; j < n; j++)
      distsum += rows[j][lenghts[j]]*weights[j];
    return distsum;
  }

  /* iterate through the strings and sum the distances */
  for (j = 0; j < n; j++) {
    size_t *rowi = rows[j];  /* current row */
    size_t leni = lenghts[j];  /* current length */
    size_t len = len1;  /* temporary len1 for suffix stripping */
    const lev_wchar *stringi = strings[j];  /* current string */

    /* strip common suffix (prefix CAN'T be stripped) */
    while (len && leni && stringi[leni-1] == string1[len-1]) {
      len--;
      leni--;
    }

    /* catch trivial cases */
    if (len == 0) {
      distsum += rowi[leni]*weights[j];
      continue;
    }
    offset = rowi[0];
    if (leni == 0) {
      distsum += (offset + len)*weights[j];
      continue;
    }

    /* complete the matrix */
    memcpy(row, rowi, (leni + 1)*sizeof(size_t));
    end = row + leni;

    for (i = 1; i <= len; i++) {
      size_t *p = row + 1;
      const lev_wchar char1 = string1[i - 1];
      const lev_wchar *char2p = stringi;
      size_t D, x;

      D = x = i + offset;
      while (p <= end) {
        size_t c3 = --D + (char1 != *(char2p++));
        x++;
        if (x > c3)
          x = c3;
        D = *p;
        D++;
        if (x > D)
          x = D;
        *(p++) = x;
      }
    }
    distsum += weights[j]*(*end);
  }

  return distsum;
}

/*
 * Try to improve a generalized set median with one-character perturbations.
 *
 * Returns a newly allocated string.
 */
LEV_STATIC_PY lev_wchar*
lev_u_median_improve(size_t len, const lev_wchar *s,
                     size_t n, const size_t *lengths,
                     const lev_wchar *strings[],
                     const double *weights,
                     size_t *medlength)
{
  size_t i;  /* usually iterates over strings (n) */
  size_t j;  /* usually iterates over characters */
  size_t pos;  /* the position in the approximate median string we are
                  trying to change */
  lev_wchar *symlist;  /* list of symbols present in the strings,
                              we iterate over it insead of set of all
                              existing symbols */
  size_t symlistlen;  /* length of symlist */
  size_t maxlen;  /* maximum input string length */
  size_t stoplen;  /* maximum tried median string length -- this is slightly
                      higher than maxlen, because the median string may be
                      longer than any of the input strings */
  size_t **rows;  /* Levenshtein matrix rows for each string, we need to keep
                     only one previous row to construct the current one */
  size_t *row;  /* a scratch buffer for new Levenshtein matrix row computation,
                   shared among all strings */
  lev_wchar *median;  /* the resulting approximate median string */
  size_t medlen;  /* the current approximate median string length */
  double minminsum;  /* the current total distance sum */

  /* find all symbols */
  symlist = make_usymlist(n, lengths, strings, &symlistlen);
  if (!symlist) {
    *medlength = 0;
    if (symlistlen != 0)
      return NULL;
    else
      return calloc(1, sizeof(lev_wchar));
  }

  /* allocate and initialize per-string matrix rows and a common work buffer */
  rows = (size_t**)malloc(n*sizeof(size_t*));
  if (!rows) {
    free(symlist);
    return NULL;
  }
  maxlen = 0;
  for (i = 0; i < n; i++) {
    size_t *ri;
    size_t leni = lengths[i];
    if (leni > maxlen)
      maxlen = leni;
    ri = rows[i] = (size_t*)malloc((leni + 1)*sizeof(size_t));
    if (!ri) {
      for (j = 0; j < i; j++)
        free(rows[j]);
      free(rows);
      free(symlist);
      return NULL;
    }
    for (j = 0; j <= leni; j++)
      ri[j] = j;
  }
  stoplen = 2*maxlen + 1;
  row = (size_t*)malloc((stoplen + 2)*sizeof(size_t));
  if (!row) {
    for (j = 0; j < n; j++)
      free(rows[j]);
    free(rows);
    free(symlist);
    return NULL;
  }

  /* initialize median to given string */
  median = (lev_wchar*)malloc((stoplen+1)*sizeof(lev_wchar));
  if (!median) {
    for (j = 0; j < n; j++)
      free(rows[j]);
    free(rows);
    free(row);
    free(symlist);
    return NULL;
  }
  median++;  /* we need -1st element for insertions a pos 0 */
  medlen = len;
  memcpy(median, s, (medlen)*sizeof(lev_wchar));
  minminsum = finish_udistance_computations(medlen, median,
                                            n, lengths, strings,
                                            weights, rows, row);

  /* sequentially try perturbations on all positions */
  for (pos = 0; pos <= medlen; ) {
    lev_wchar orig_symbol, symbol;
    LevEditType operation;
    double sum;

    symbol = median[pos];
    operation = LEV_EDIT_KEEP;
    /* IF pos < medlength: FOREACH symbol: try to replace the symbol
     * at pos, if some lower the total distance, chooste the best */
    if (pos < medlen) {
      orig_symbol = median[pos];
      for (j = 0; j < symlistlen; j++) {
        if (symlist[j] == orig_symbol)
          continue;
        median[pos] = symlist[j];
        sum = finish_udistance_computations(medlen - pos, median + pos,
                                            n, lengths, strings,
                                            weights, rows, row);
        if (sum < minminsum) {
          minminsum = sum;
          symbol = symlist[j];
          operation = LEV_EDIT_REPLACE;
        }
      }
      median[pos] = orig_symbol;
    }
    /* FOREACH symbol: try to add it at pos, if some lower the total
     * distance, chooste the best (increase medlength)
     * We simulate insertion by replacing the character at pos-1 */
    orig_symbol = *(median + pos - 1);
    for (j = 0; j < symlistlen; j++) {
      *(median + pos - 1) = symlist[j];
      sum = finish_udistance_computations(medlen - pos + 1, median + pos - 1,
                                          n, lengths, strings,
                                          weights, rows, row);
      if (sum < minminsum) {
        minminsum = sum;
        symbol = symlist[j];
        operation = LEV_EDIT_INSERT;
      }
    }
    *(median + pos - 1) = orig_symbol;
    /* IF pos < medlength: try to delete the symbol at pos, if it lowers
     * the total distance remember it (decrease medlength) */
    if (pos < medlen) {
      sum = finish_udistance_computations(medlen - pos - 1, median + pos + 1,
                                          n, lengths, strings,
                                          weights, rows, row);
      if (sum < minminsum) {
        minminsum = sum;
        operation = LEV_EDIT_DELETE;
      }
    }
    /* actually perform the operation */
    switch (operation) {
      case LEV_EDIT_REPLACE:
      median[pos] = symbol;
      break;

      case LEV_EDIT_INSERT:
      memmove(median+pos+1, median+pos,
              (medlen - pos)*sizeof(lev_wchar));
      median[pos] = symbol;
      medlen++;
      break;

      case LEV_EDIT_DELETE:
      memmove(median+pos, median + pos+1,
              (medlen - pos-1)*sizeof(lev_wchar));
      medlen--;
      break;

      default:
      break;
    }
    assert(medlen <= stoplen);
    /* now the result is known, so recompute all matrix rows and move on */
    if (operation != LEV_EDIT_DELETE) {
      symbol = median[pos];
      row[0] = pos + 1;
      for (i = 0; i < n; i++) {
        const lev_wchar *stri = strings[i];
        size_t *oldrow = rows[i];
        size_t leni = lengths[i];
        size_t k;
        /* compute a row of Levenshtein matrix */
        for (k = 1; k <= leni; k++) {
          size_t c1 = oldrow[k] + 1;
          size_t c2 = row[k - 1] + 1;
          size_t c3 = oldrow[k - 1] + (symbol != stri[k - 1]);
          row[k] = c2 > c3 ? c3 : c2;
          if (row[k] > c1)
            row[k] = c1;
        }
        memcpy(oldrow, row, (leni + 1)*sizeof(size_t));
      }
      pos++;
    }
  }

  /* clean up */
  for (i = 0; i < n; i++)
    free(rows[i]);
  free(rows);
  free(row);
  free(symlist);

  /* return result */
  {
    lev_wchar *result = (lev_wchar*)malloc(medlen*sizeof(lev_wchar));
    if (!result) {
      free(median);
      return NULL;
    }
    *medlength = medlen;
    memcpy(result, median, medlen*sizeof(lev_wchar));
    median--;
    free(median);
    return result;
  }
}

/*
 * Plain set median.
 *
 * Returns a newly allocated string.
 */
LEV_STATIC_PY lev_byte*
lev_set_median(size_t n, const size_t *lengths,
               const lev_byte *strings[],
               const double *weights,
               size_t *medlength)
{
  size_t minidx = 0;
  double mindist = 1e100;
  size_t i;
  long int *distances;

  distances = (long int*)malloc((n*(n - 1)/2)*sizeof(long int));
  if (!distances)
    return NULL;
  memset(distances, 0xff, (n*(n - 1)/2)*sizeof(long int)); /* XXX */
  for (i = 0; i < n; i++) {
    size_t j = 0;
    double dist = 0.0;
    const lev_byte *stri = strings[i];
    size_t leni = lengths[i];
    /* below diagonal */
    while (j < i && dist < mindist) {
      size_t dindex = (i - 1)*(i - 2)/2 + j;
      long int d;
      if (distances[dindex] >= 0)
        d = distances[dindex];
      else {
        d = lev_edit_distance(lengths[j], strings[j], leni, stri, 0);
        if (d < 0) {
          free(distances);
          return NULL;
        }
      }
      dist += weights[j]*d;
      j++;
    }
    j++;  /* no need to compare item with itself */
    /* above diagonal */
    while (j < n && dist < mindist) {
      size_t dindex = (j - 1)*(j - 2)/2 + i;
      distances[dindex] = lev_edit_distance(lengths[j], strings[j],
                                            leni, stri, 0);
      if (distances[dindex] < 0) {
        free(distances);
        return NULL;
      }
      dist += weights[j]*distances[dindex];
      j++;
    }

    if (dist < mindist) {
      mindist = dist;
      minidx = i;
    }
  }

  free(distances);
  *medlength = lengths[minidx];
  if (!lengths[minidx])
    return (lev_byte*)calloc(1, sizeof(lev_byte));
  {
    lev_byte *result = (lev_byte*)malloc(lengths[minidx]*sizeof(lev_byte));
    if (!result)
      return NULL;
    return memcpy(result, strings[minidx], lengths[minidx]*sizeof(lev_byte));
  }
}

/*
 * Plain set median (Unicode).
 *
 * Returns a newly allocated string.
 */
LEV_STATIC_PY lev_wchar*
lev_u_set_median(size_t n, const size_t *lengths,
                 const lev_wchar *strings[],
                 const double *weights,
                 size_t *medlength)
{
  size_t minidx = 0;
  double mindist = 1e100;
  size_t i;
  long int *distances;

  distances = (long int*)malloc((n*(n - 1)/2)*sizeof(long int));
  if (!distances)
    return NULL;
  memset(distances, 0xff, (n*(n - 1)/2)*sizeof(long int)); /* XXX */
  for (i = 0; i < n; i++) {
    size_t j = 0;
    double dist = 0.0;
    const lev_wchar *stri = strings[i];
    size_t leni = lengths[i];
    /* below diagonal */
    while (j < i && dist < mindist) {
      size_t dindex = (i - 1)*(i - 2)/2 + j;
      long int d;
      if (distances[dindex] >= 0)
        d = distances[dindex];
      else {
        d = lev_u_edit_distance(lengths[j], strings[j], leni, stri, 0);
        if (d < 0) {
          free(distances);
          return NULL;
        }
      }
      dist += weights[j]*d;
      j++;
    }
    j++;  /* no need to compare item with itself */
    /* above diagonal */
    while (j < n && dist < mindist) {
      size_t dindex = (j - 1)*(j - 2)/2 + i;
      distances[dindex] = lev_u_edit_distance(lengths[j], strings[j],
                                              leni, stri, 0);
      if (distances[dindex] < 0) {
        free(distances);
        return NULL;
      }
      dist += weights[j]*distances[dindex];
      j++;
    }

    if (dist < mindist) {
      mindist = dist;
      minidx = i;
    }
  }

  free(distances);
  *medlength = lengths[minidx];
  if (!lengths[minidx])
    return (lev_wchar*)calloc(1, sizeof(lev_wchar));
  {
    lev_wchar *result = (lev_wchar*)malloc(lengths[minidx]*sizeof(lev_wchar));
    if (!result)
      return NULL;
    return memcpy(result, strings[minidx], lengths[minidx]*sizeof(lev_wchar));
  }
}

/****************************************************************************
 *
 * Set, sequence distances
 *
 ****************************************************************************/

/*
 * Levenshtein distance of two string sequences.
 *
 * I.e. double-Levenshtein.
 */
LEV_STATIC_PY double
lev_edit_seq_distance(size_t n1, const size_t *lengths1,
                      const lev_byte *strings1[],
                      size_t n2, const size_t *lengths2,
                      const lev_byte *strings2[])
{
  size_t i;
  double *row;  /* we only need to keep one row of costs */
  double *end;

  /* strip common prefix */
  while (n1 > 0 && n2 > 0
         && *lengths1 == *lengths2
         && memcmp(*strings1, *strings2,
                   *lengths1*sizeof(lev_byte)) == 0) {
    n1--;
    n2--;
    strings1++;
    strings2++;
    lengths1++;
    lengths2++;
  }

  /* strip common suffix */
  while (n1 > 0 && n2 > 0
         && lengths1[n1-1] == lengths2[n2-1]
         && memcmp(strings1[n1-1], strings2[n2-1],
                   lengths1[n1-1]*sizeof(lev_byte)) == 0) {
    n1--;
    n2--;
  }

  /* catch trivial cases */
  if (n1 == 0)
    return n2;
  if (n2 == 0)
    return n1;

  /* make the inner cycle (i.e. strings2) the longer one */
  if (n1 > n2) {
    size_t nx = n1;
    const size_t *lx = lengths1;
    const lev_byte **sx = strings1;
    n1 = n2;
    n2 = nx;
    lengths1 = lengths2;
    lengths2 = lx;
    strings1 = strings2;
    strings2 = sx;
  }
  n1++;
  n2++;

  /* initalize first row */
  row = (double*)malloc(n2*sizeof(double));
  if (!row)
    return -1.0;
  end = row + n2 - 1;
  for (i = 0; i < n2; i++)
    row[i] = (double)i;

  /* go through the matrix and compute the costs.  yes, this is an extremely
   * obfuscated version, but also extremely memory-conservative and relatively
   * fast.  */
  for (i = 1; i < n1; i++) {
    double *p = row + 1;
    const lev_byte *str1 = strings1[i - 1];
    const size_t len1 = lengths1[i - 1];
    const lev_byte **str2p = strings2;
    const size_t *len2p = lengths2;
    double D = i - 1.0;
    double x = i;
    while (p <= end) {
      size_t l = len1 + *len2p;
      double q;
      if (l == 0)
        q = D;
      else {
        size_t d = lev_edit_distance(len1, str1, *(len2p++), *(str2p++), 1);
        if (d == (size_t)(-1)) {
          free(row);
          return -1.0;
        }
        q = D + 2.0/l*d;
      }
      x += 1.0;
      if (x > q)
        x = q;
      D = *p;
      if (x > D + 1.0)
        x = D + 1.0;
      *(p++) = x;
    }
  }

  {
    double q = *end;
    free(row);
    return q;
  }
}

/*
 * Levenshtein distance of two string sequences. (Unicode)
 *
 * I.e. double-Levenshtein.
 */
LEV_STATIC_PY double
lev_u_edit_seq_distance(size_t n1, const size_t *lengths1,
                        const lev_wchar *strings1[],
                        size_t n2, const size_t *lengths2,
                        const lev_wchar *strings2[])
{
  size_t i;
  double *row;  /* we only need to keep one row of costs */
  double *end;

  /* strip common prefix */
  while (n1 > 0 && n2 > 0
         && *lengths1 == *lengths2
         && memcmp(*strings1, *strings2,
                   *lengths1*sizeof(lev_wchar)) == 0) {
    n1--;
    n2--;
    strings1++;
    strings2++;
    lengths1++;
    lengths2++;
  }

  /* strip common suffix */
  while (n1 > 0 && n2 > 0
         && lengths1[n1-1] == lengths2[n2-1]
         && memcmp(strings1[n1-1], strings2[n2-1],
                   lengths1[n1-1]*sizeof(lev_wchar)) == 0) {
    n1--;
    n2--;
  }

  /* catch trivial cases */
  if (n1 == 0)
    return (double)n2;
  if (n2 == 0)
    return (double)n1;

  /* make the inner cycle (i.e. strings2) the longer one */
  if (n1 > n2) {
    size_t nx = n1;
    const size_t *lx = lengths1;
    const lev_wchar **sx = strings1;
    n1 = n2;
    n2 = nx;
    lengths1 = lengths2;
    lengths2 = lx;
    strings1 = strings2;
    strings2 = sx;
  }
  n1++;
  n2++;

  /* initalize first row */
  row = (double*)malloc(n2*sizeof(double));
  if (!row)
    return -1.0;
  end = row + n2 - 1;
  for (i = 0; i < n2; i++)
    row[i] = (double)i;

  /* go through the matrix and compute the costs.  yes, this is an extremely
   * obfuscated version, but also extremely memory-conservative and relatively
   * fast.  */
  for (i = 1; i < n1; i++) {
    double *p = row + 1;
    const lev_wchar *str1 = strings1[i - 1];
    const size_t len1 = lengths1[i - 1];
    const lev_wchar **str2p = strings2;
    const size_t *len2p = lengths2;
    double D = i - 1.0;
    double x = i;
    while (p <= end) {
      size_t l = len1 + *len2p;
      double q;
      if (l == 0)
        q = D;
      else {
        size_t d = lev_u_edit_distance(len1, str1, *(len2p++), *(str2p++), 1);
        if (d == (size_t)(-1)) {
          free(row);
          return -1.0;
        }
        q = D + 2.0/l*d;
      }
      x += 1.0;
      if (x > q)
        x = q;
      D = *p;
      if (x > D + 1.0)
        x = D + 1.0;
      *(p++) = x;
    }
  }

  {
    double q = *end;
    free(row);
    return q;
  }
}

/*
 * Distance of two string sets.
 *
 * Uses sequential Munkers-Blackman algorithm.
 */
LEV_STATIC_PY double
lev_set_distance(size_t n1, const size_t *lengths1,
                 const lev_byte *strings1[],
                 size_t n2, const size_t *lengths2,
                 const lev_byte *strings2[])
{
  double *dists;  /* the (modified) distance matrix, indexed [row*n1 + col] */
  double *r;
  size_t i, j;
  size_t *map;
  double sum;

  /* catch trivial cases */
  if (n1 == 0)
    return (double)n2;
  if (n2 == 0)
    return (double)n1;

  /* make the number of columns (n1) smaller than the number of rows */
  if (n1 > n2) {
    size_t nx = n1;
    const size_t *lx = lengths1;
    const lev_byte **sx = strings1;
    n1 = n2;
    n2 = nx;
    lengths1 = lengths2;
    lengths2 = lx;
    strings1 = strings2;
    strings2 = sx;
  }

  /* compute distances from each to each */
  r = dists = (double*)malloc(n1*n2*sizeof(double));
  if (!r)
    return -1.0;
  for (i = 0; i < n2; i++) {
    size_t len2 = lengths2[i];
    const lev_byte *str2 = strings2[i];
    const size_t *len1p = lengths1;
    const lev_byte **str1p = strings1;
    for (j = 0; j < n1; j++) {
      size_t l = len2 + *len1p;
      if (l == 0)
        *(r++) = 0.0;
      else {
        size_t d = lev_edit_distance(len2, str2, *(len1p++), *(str1p)++, 1);
        if (d == (size_t)(-1)) {
          free(r);
          return -1.0;
        }
        *(r++) = (double)d/l;
      }
    }
  }

  /* find the optimal mapping between the two sets */
  map = munkers_blackman(n1, n2, dists);
  if (!map)
    return -1.0;

  /* sum the set distance */
  sum = n2 - n1;
  for (j = 0; j < n1; j++) {
    size_t l;
    i = map[j];
    l = lengths1[j] + lengths2[i];
    if (l > 0) {
      size_t d = lev_edit_distance(lengths1[j], strings1[j],
                                   lengths2[i], strings2[i], 1);
      if (d == (size_t)(-1)) {
        free(map);
        return -1.0;
      }
      sum += 2.0*d/l;
    }
  }
  free(map);

  return sum;
}

/*
 * Distance of two string sets. (Unicode)
 *
 * Uses sequential Munkers-Blackman algorithm.
 */
LEV_STATIC_PY double
lev_u_set_distance(size_t n1, const size_t *lengths1,
                   const lev_wchar *strings1[],
                   size_t n2, const size_t *lengths2,
                   const lev_wchar *strings2[])
{
  double *dists;  /* the (modified) distance matrix, indexed [row*n1 + col] */
  double *r;
  size_t i, j;
  size_t *map;
  double sum;

  /* catch trivial cases */
  if (n1 == 0)
    return (double)n2;
  if (n2 == 0)
    return (double)n1;

  /* make the number of columns (n1) smaller than the number of rows */
  if (n1 > n2) {
    size_t nx = n1;
    const size_t *lx = lengths1;
    const lev_wchar **sx = strings1;
    n1 = n2;
    n2 = nx;
    lengths1 = lengths2;
    lengths2 = lx;
    strings1 = strings2;
    strings2 = sx;
  }

  /* compute distances from each to each */
  r = dists = (double*)malloc(n1*n2*sizeof(double));
  if (!r)
    return -1.0;
  for (i = 0; i < n2; i++) {
    size_t len2 = lengths2[i];
    const lev_wchar *str2 = strings2[i];
    const size_t *len1p = lengths1;
    const lev_wchar **str1p = strings1;
    for (j = 0; j < n1; j++) {
      size_t l = len2 + *len1p;
      if (l == 0)
        *(r++) = 0.0;
      else {
        size_t d = lev_u_edit_distance(len2, str2, *(len1p++), *(str1p)++, 1);
        if (d == (size_t)(-1)) {
          free(r);
          return -1.0;
        }
        *(r++) = (double)d/l;
      }
    }
  }

  /* find the optimal mapping between the two sets */
  map = munkers_blackman(n1, n2, dists);
  if (!map)
    return -1.0;

  /* sum the set distance */
  sum = n2 - n1;
  for (j = 0; j < n1; j++) {
    size_t l;
    i = map[j];
    l = lengths1[j] + lengths2[i];
    if (l > 0) {
      size_t d = lev_u_edit_distance(lengths1[j], strings1[j],
                                     lengths2[i], strings2[i], 1);
      if (d == (size_t)(-1)) {
        free(map);
        return -1.0;
      }
      sum += 2.0*d/l;
    }
  }
  free(map);

  return sum;
}

/*
 * Munkers-Blackman algorithm.
 */
static size_t*
munkers_blackman(size_t n1, size_t n2, double *dists)
{
  size_t i, j;
  size_t *covc, *covr;  /* 1 if column/row is covered */
  /* these contain 1-based indices, so we can use zero as `none'
   * zstarr: column of a z* in given row
   * zstarc: row of a z* in given column
   * zprimer: column of a z' in given row */
  size_t *zstarr, *zstarc, *zprimer;

  /* allocate memory */
  covc = calloc(n1, sizeof(size_t));
  if (!covc)
    return NULL;
  zstarc = calloc(n1, sizeof(size_t));
  if (!zstarc) {
    free(covc);
    return NULL;
  }
  covr = calloc(n2, sizeof(size_t));
  if (!covr) {
    free(zstarc);
    free(covc);
    return NULL;
  }
  zstarr = calloc(n2, sizeof(size_t));
  if (!zstarr) {
    free(covr);
    free(zstarc);
    free(covc);
    return NULL;
  }
  zprimer = calloc(n2, sizeof(size_t));
  if (!zprimer) {
    free(zstarr);
    free(covr);
    free(zstarc);
    free(covc);
    return NULL;
  }

  /* step 0 (substract minimal distance) and step 1 (find zeroes) */
  for (j = 0; j < n1; j++) {
    size_t minidx = 0;
    double *col = dists + j;
    double min = *col;
    double *p = col + n1;
    for (i = 1; i < n2; i++) {
      if (min > *p) {
        minidx = i;
        min = *p;
      }
      p += n1;
    }
    /* substract */
    p = col;
    for (i = 0; i < n2; i++) {
      *p -= min;
      if (*p < EPSILON)
        *p = 0.0;
      p += n1;
    }
    /* star the zero, if possible */
    if (!zstarc[j] && !zstarr[minidx]) {
      zstarc[j] = minidx + 1;
      zstarr[minidx] = j + 1;
    }
    else {
      /* otherwise try to find some other */
      p = col;
      for (i = 0; i < n2; i++) {
        if (i != minidx && *p == 0.0
            && !zstarc[j] && !zstarr[i]) {
          zstarc[j] = i + 1;
          zstarr[i] = j + 1;
          break;
        }
        p += n1;
      }
    }
  }

  /* main */
  while (1) {
    /* step 2 (cover columns containing z*) */
    {
      size_t nc = 0;
      for (j = 0; j < n1; j++) {
        if (zstarc[j]) {
          covc[j] = 1;
          nc++;
        }
      }
      if (nc == n1)
        break;
    }

    /* step 3 (find uncovered zeroes) */
    while (1) {
      step_3:
      /* search uncovered matrix entries */
      for (j = 0; j < n1; j++) {
        double *p = dists + j;
        if (covc[j])
          continue;
        for (i = 0; i < n2; i++) {
          if (!covr[i] && *p == 0.0) {
            /* when a zero is found, prime it */
            zprimer[i] = j + 1;
            if (zstarr[i]) {
              /* if there's a z* in the same row,
               * uncover the column, cover the row and redo */
              covr[i] = 1;
              covc[zstarr[i] - 1] = 0;
              goto step_3;
            }
            /* if there's no z*,
             * we are at the end of our path an can convert z'
             * to z* */
            goto step_4;
          }
          p += n1;
        }
      }

      /* step 5 (new zero manufacturer)
       * we can't get here, unless no zero is found at all */
      {
        /* find the smallest uncovered entry */
        double min = 1e100;
        for (j = 0; j < n1; j++) {
          double *p = dists + j;
          if (covc[j])
            continue;
          for (i = 0; i < n2; i++) {
            if (!covr[i] && min > *p) {
              min = *p;
            }
            p += n1;
          }
        }
        /* add it to all covered rows */
        for (i = 0; i < n2; i++) {
          double *p = dists + i*n1;
          if (!covr[i])
            continue;
          for (j = 0; j < n1; j++)
            *(p++) += min;
        }
        /* substract if from all uncovered columns */
        for (j = 0; j < n1; j++) {
          double *p = dists + j;
          if (covc[j])
            continue;
          for (i = 0; i < n2; i++) {
            *p -= min;
            if (*p < EPSILON)
              *p = 0.0;
            p += n1;
          }
        }
      }
    }

    /* step 4 (increment the number of z*)
     * i is the row number (we get it from step 3) */
    step_4:
    i++;
    do {
      size_t x = i;

      i--;
      j = zprimer[i] - 1;  /* move to z' in the same row */
      zstarr[i] = j + 1;   /* mark it as z* in row buffer */
      i = zstarc[j];       /* move to z* in the same column */
      zstarc[j] = x;       /* mark the z' as being new z* */
    } while (i);
    memset(zprimer, 0, n2*sizeof(size_t));
    memset(covr, 0, n2*sizeof(size_t));
    memset(covc, 0, n1*sizeof(size_t));
  }

  free(dists);
  free(covc);
  free(covr);
  /*free(zstarc);  this is the result */
  free(zstarr);
  free(zprimer);

  for (j = 0; j < n1; j++)
    zstarc[j]--;
  return zstarc;
}

/****************************************************************************
 *
 * Editops and other difflib-like stuff.
 *
 ****************************************************************************/

/*
 * Returns zero if ops are applicable as len1 -> len2 partial edit.
 */
LEV_STATIC_PY int
lev_editops_check_errors(size_t len1, size_t len2,
                          size_t n, const LevEditOp *ops)
{
  const LevEditOp *o;
  size_t i;

  if (!n)
    return LEV_EDIT_ERR_OK;

  /* check bounds */
  o = ops;
  for (i = n; i; i--, o++) {
    if (o->type >= LEV_EDIT_LAST)
      return LEV_EDIT_ERR_TYPE;
    if (o->spos > len1 || o->dpos > len2)
      return LEV_EDIT_ERR_OUT;
    if (o->spos == len1 && o->type != LEV_EDIT_INSERT)
      return LEV_EDIT_ERR_OUT;
    if (o->dpos == len2 && o->type != LEV_EDIT_DELETE)
      return LEV_EDIT_ERR_OUT;
  }

  /* check ordering */
  o = ops + 1;
  for (i = n - 1; i; i--, o++, ops++) {
    if (o->spos < ops->spos || o->dpos < ops->dpos)
      return LEV_EDIT_ERR_ORDER;
  }

  return LEV_EDIT_ERR_OK;
}

/*
 * Returns zero if block ops are applicable as len1 -> len2 partial edit.
 */
LEV_STATIC_PY int
lev_opcodes_check_errors(size_t len1, size_t len2,
                         size_t nb, const LevOpCode *bops)
{
  const LevOpCode *b;
  size_t i;

  if (!nb)
    return 1;

  /* check completenes */
  if (bops->sbeg || bops->dbeg
      || bops[nb - 1].send != len1 || bops[nb - 1].dend != len2)
    return LEV_EDIT_ERR_SPAN;

  /* check bounds and block consistency */
  b = bops;
  for (i = nb; i; i--, b++) {
    if (b->send > len1 || b->dend > len2)
      return LEV_EDIT_ERR_OUT;
    switch (b->type) {
      case LEV_EDIT_KEEP:
      case LEV_EDIT_REPLACE:
      if (b->dend - b->dbeg != b->send - b->sbeg || b->dend == b->dbeg)
        return LEV_EDIT_ERR_BLOCK;
      break;

      case LEV_EDIT_INSERT:
      if (b->dend - b->dbeg == 0 || b->send - b->sbeg != 0)
        return LEV_EDIT_ERR_BLOCK;
      break;

      case LEV_EDIT_DELETE:
      if (b->send - b->sbeg == 0 || b->dend - b->dbeg != 0)
        return LEV_EDIT_ERR_BLOCK;
      break;

      default:
      return LEV_EDIT_ERR_TYPE;
      break;
    }
  }

  /* check ordering */
  b = bops + 1;
  for (i = nb - 1; i; i--, b++, bops++) {
    if (b->sbeg != bops->send || b->dbeg != bops->dend)
      return LEV_EDIT_ERR_ORDER;
  }

  return LEV_EDIT_ERR_OK;
}

/*
 * Invert the sense of ops (src <-> dest).
 *
 * Changed in place.
 */
LEV_STATIC_PY void
lev_editops_inverse(size_t n, LevEditOp *ops)
{
  size_t i;

  for (i = n; i; i--, ops++) {
    size_t z;

    z = ops->dpos;
    ops->dpos = ops->spos;
    ops->spos = z;
    if (ops->type & 2)
      ops->type ^= 1;
  }
}

/*
 * Apply a subsequence of editing operations to a string.
 *
 * Returns a newly allocated string.  ops is not checked!
 */
LEV_STATIC_PY lev_byte*
lev_editops_apply(size_t len1, const lev_byte *string1,
                  __attribute__((unused)) size_t len2, const lev_byte *string2,
                  size_t n, const LevEditOp *ops,
                  size_t *len)
{
  lev_byte *dst, *dpos;  /* destination string */
  const lev_byte *spos;  /* source string position */
  size_t i, j;

  /* this looks too complex for such a simple task, but note ops is not
   * a complete edit sequence, we have to be able to apply anything anywhere */
  dpos = dst = (lev_byte*)malloc((n + len1)*sizeof(lev_byte));
  if (!dst) {
    *len = (size_t)(-1);
    return NULL;
  }
  spos = string1;
  for (i = n; i; i--, ops++) {
    /* XXX: this fine with gcc internal memcpy, but when memcpy is
     * actually a function, it may be pretty slow */
    j = ops->spos - (spos - string1) + (ops->type == LEV_EDIT_KEEP);
    if (j) {
      memcpy(dpos, spos, j*sizeof(lev_byte));
      spos += j;
      dpos += j;
    }
    switch (ops->type) {
      case LEV_EDIT_DELETE:
      spos++;
      break;

      case LEV_EDIT_REPLACE:
      spos++;
      case LEV_EDIT_INSERT:
      *(dpos++) = string2[ops->dpos];
      break;

      default:
      break;
    }
  }
  j = len1 - (spos - string1);
  if (j) {
    memcpy(dpos, spos, j*sizeof(lev_byte));
    spos += j;
    dpos += j;
  }

  *len = dpos - dst;
  /* possible realloc failure is detected with *len != 0 */
  return realloc(dst, *len*sizeof(lev_byte));
}

/*
 * Apply a subsequence of editing operations to a string. (Unicode)
 *
 * Returns a newly allocated string.  ops is not checked!
 */
LEV_STATIC_PY lev_wchar*
lev_u_editops_apply(size_t len1, const lev_wchar *string1,
                    __attribute__((unused)) size_t len2,
                    const lev_wchar *string2,
                    size_t n, const LevEditOp *ops,
                    size_t *len)
{
  lev_wchar *dst, *dpos;  /* destination string */
  const lev_wchar *spos;  /* source string position */
  size_t i, j;

  /* this looks too complex for such a simple task, but note ops is not
   * a complete edit sequence, we have to be able to apply anything anywhere */
  dpos = dst = (lev_wchar*)malloc((n + len1)*sizeof(lev_wchar));
  if (!dst) {
    *len = (size_t)(-1);
    return NULL;
  }
  spos = string1;
  for (i = n; i; i--, ops++) {
    /* XXX: this is fine with gcc internal memcpy, but when memcpy is
     * actually a function, it may be pretty slow */
    j = ops->spos - (spos - string1) + (ops->type == LEV_EDIT_KEEP);
    if (j) {
      memcpy(dpos, spos, j*sizeof(lev_wchar));
      spos += j;
      dpos += j;
    }
    switch (ops->type) {
      case LEV_EDIT_DELETE:
      spos++;
      break;

      case LEV_EDIT_REPLACE:
      spos++;
      case LEV_EDIT_INSERT:
      *(dpos++) = string2[ops->dpos];
      break;

      default:
      break;
    }
  }

  *len = dpos - dst;
  /* possible realloc failure is detected with *len != 0 */
  return realloc(dst, *len*sizeof(lev_wchar));
}

/*
 * Get the editops from the cost matrix.
 *
 * Frees the matrix after use.
 */
static LevEditOp*
editops_from_cost_matrix(size_t len1, const lev_byte *string1, size_t o1,
                         size_t len2, const lev_byte *string2, size_t o2,
                         size_t *matrix, size_t *n)
{
  size_t *p;
  size_t i, j, pos;
  LevEditOp *ops;
  int dir = 0;

  pos = *n = matrix[len1*len2 - 1];
  if (!*n) {
    free(matrix);
    return NULL;
  }
  ops = (LevEditOp*)malloc((*n)*sizeof(LevEditOp));
  if (!ops) {
    free(matrix);
    *n = (size_t)(-1);
    return NULL;
  }
  i = len1 - 1;
  j = len2 - 1;
  p = matrix + len1*len2 - 1;
  while (i || j) {
    /* prefer contiuning in the same direction */
    if (dir < 0 && j && *p == *(p - 1) + 1) {
      pos--;
      ops[pos].type = LEV_EDIT_INSERT;
      ops[pos].spos = i + o1;
      ops[pos].dpos = --j + o2;
      p--;
      continue;
    }
    if (dir > 0 && i && *p == *(p - len2) + 1) {
      pos--;
      ops[pos].type = LEV_EDIT_DELETE;
      ops[pos].spos = --i + o1;
      ops[pos].dpos = j + o2;
      p -= len2;
      continue;
    }
    if (i && j && *p == *(p - len2 - 1)
        && string1[i - 1] == string2[j - 1]) {
      /* don't be stupid like difflib, don't store LEV_EDIT_KEEP */
      i--;
      j--;
      p -= len2 + 1;
      dir = 0;
      continue;
    }
    if (i && j && *p == *(p - len2 - 1) + 1) {
      pos--;
      ops[pos].type = LEV_EDIT_REPLACE;
      ops[pos].spos = --i + o1;
      ops[pos].dpos = --j + o2;
      p -= len2 + 1;
      dir = 0;
      continue;
    }
    /* we cant't turn directly from -1 to 1, in this case it would be better
     * to go diagonally, but check it (dir == 0) */
    if (dir == 0 && j && *p == *(p - 1) + 1) {
      pos--;
      ops[pos].type = LEV_EDIT_INSERT;
      ops[pos].spos = i + o1;
      ops[pos].dpos = --j + o2;
      p--;
      dir = -1;
      continue;
    }
    if (dir == 0 && i && *p == *(p - len2) + 1) {
      pos--;
      ops[pos].type = LEV_EDIT_DELETE;
      ops[pos].spos = --i + o1;
      ops[pos].dpos = j + o2;
      p -= len2;
      dir = 1;
      continue;
    }
    /* coredump right now, later might be too late ;-) */
    assert("lost in the cost matrix" == NULL);
  }
  free(matrix);

  return ops;
}

/*
 * Find (some) edit sequence from string1 to string2.
 */
LEV_STATIC_PY LevEditOp*
lev_editops_find(size_t len1, const lev_byte *string1,
                 size_t len2, const lev_byte *string2,
                 size_t *n)
{
  size_t len1o, len2o;
  size_t i;
  size_t *matrix; /* cost matrix */

  /* strip common prefix */
  len1o = 0;
  while (len1 > 0 && len2 > 0 && *string1 == *string2) {
    len1--;
    len2--;
    string1++;
    string2++;
    len1o++;
  }
  len2o = len1o;

  /* strip common suffix */
  while (len1 > 0 && len2 > 0 && string1[len1-1] == string2[len2-1]) {
    len1--;
    len2--;
  }
  len1++;
  len2++;

  /* initalize first row and column */
  matrix = (size_t*)malloc(len1*len2*sizeof(size_t));
  if (!matrix) {
    *n = (size_t)(-1);
    return NULL;
  }
  for (i = 0; i < len2; i++)
    matrix[i] = i;
  for (i = 1; i < len1; i++)
    matrix[len2*i] = i;

  /* find the costs and fill the matrix */
  for (i = 1; i < len1; i++) {
    size_t *prev = matrix + (i - 1)*len2;
    size_t *p = matrix + i*len2;
    size_t *end = p + len2 - 1;
    const lev_byte char1 = string1[i - 1];
    const lev_byte *char2p = string2;
    size_t x = i;
    p++;
    while (p <= end) {
      size_t c3 = *(prev++) + (char1 != *(char2p++));
      x++;
      if (x > c3)
        x = c3;
      c3 = *prev + 1;
      if (x > c3)
        x = c3;
      *(p++) = x;
    }
  }

  /* find the way back */
  return editops_from_cost_matrix(len1, string1, len1o,
                                  len2, string2, len2o,
                                  matrix, n);
}

/*
 * Get the editops from the cost matrix. (Unicode)
 *
 * Frees the matrix after use.
 */
static LevEditOp*
ueditops_from_cost_matrix(size_t len1, const lev_wchar *string1, size_t o1,
                          size_t len2, const lev_wchar *string2, size_t o2,
                          size_t *matrix, size_t *n)
{
  size_t *p;
  size_t i, j, pos;
  LevEditOp *ops;
  int dir = 0;

  pos = *n = matrix[len1*len2 - 1];
  if (!*n) {
    free(matrix);
    return NULL;
  }
  ops = (LevEditOp*)malloc((*n)*sizeof(LevEditOp));
  if (!ops) {
    free(matrix);
    *n = (size_t)(-1);
    return NULL;
  }
  i = len1 - 1;
  j = len2 - 1;
  p = matrix + len1*len2 - 1;
  while (i || j) {
    /* prefer contiuning in the same direction */
    if (dir < 0 && j && *p == *(p - 1) + 1) {
      pos--;
      ops[pos].type = LEV_EDIT_INSERT;
      ops[pos].spos = i + o1;
      ops[pos].dpos = --j + o2;
      p--;
      continue;
    }
    if (dir > 0 && i && *p == *(p - len2) + 1) {
      pos--;
      ops[pos].type = LEV_EDIT_DELETE;
      ops[pos].spos = --i + o1;
      ops[pos].dpos = j + o2;
      p -= len2;
      continue;
    }
    if (i && j && *p == *(p - len2 - 1)
        && string1[i - 1] == string2[j - 1]) {
      /* don't be stupid like difflib, don't store LEV_EDIT_KEEP */
      i--;
      j--;
      p -= len2 + 1;
      dir = 0;
      continue;
    }
    if (i && j && *p == *(p - len2 - 1) + 1) {
      pos--;
      ops[pos].type = LEV_EDIT_REPLACE;
      ops[pos].spos = --i + o1;
      ops[pos].dpos = --j + o2;
      p -= len2 + 1;
      dir = 0;
      continue;
    }
    /* we cant't turn directly from -1 to 1, in this case it would be better
     * to go diagonally, but check it (dir == 0) */
    if (dir == 0 && j && *p == *(p - 1) + 1) {
      pos--;
      ops[pos].type = LEV_EDIT_INSERT;
      ops[pos].spos = i + o1;
      ops[pos].dpos = --j + o2;
      p--;
      dir = -1;
      continue;
    }
    if (dir == 0 && i && *p == *(p - len2) + 1) {
      pos--;
      ops[pos].type = LEV_EDIT_DELETE;
      ops[pos].spos = --i + o1;
      ops[pos].dpos = j + o2;
      p -= len2;
      dir = 1;
      continue;
    }
    /* coredump right now, later might be too late ;-) */
    assert("lost in the cost matrix" == NULL);
  }
  free(matrix);

  return ops;
}

/*
 * Find (some) edit sequence from string1 to string2. (Unicode)
 */
LEV_STATIC_PY LevEditOp*
lev_u_editops_find(size_t len1, const lev_wchar *string1,
                   size_t len2, const lev_wchar *string2,
                   size_t *n)
{
  size_t len1o, len2o;
  size_t i;
  size_t *matrix; /* cost matrix */

  /* strip common prefix */
  len1o = 0;
  while (len1 > 0 && len2 > 0 && *string1 == *string2) {
    len1--;
    len2--;
    string1++;
    string2++;
    len1o++;
  }
  len2o = len1o;

  /* strip common suffix */
  while (len1 > 0 && len2 > 0 && string1[len1-1] == string2[len2-1]) {
    len1--;
    len2--;
  }
  len1++;
  len2++;

  /* initalize first row and column */
  matrix = (size_t*)malloc(len1*len2*sizeof(size_t));
  if (!matrix) {
    *n = (size_t)(-1);
    return NULL;
  }
  for (i = 0; i < len2; i++)
    matrix[i] = i;
  for (i = 1; i < len1; i++)
    matrix[len2*i] = i;

  /* find the costs and fill the matrix */
  for (i = 1; i < len1; i++) {
    size_t *prev = matrix + (i - 1)*len2;
    size_t *p = matrix + i*len2;
    size_t *end = p + len2 - 1;
    const lev_wchar char1 = string1[i - 1];
    const lev_wchar *char2p = string2;
    size_t x = i;
    p++;
    while (p <= end) {
      size_t c3 = *(prev++) + (char1 != *(char2p++));
      x++;
      if (x > c3)
        x = c3;
      c3 = *prev + 1;
      if (x > c3)
        x = c3;
      *(p++) = x;
    }
  }

  /* find the way back */
  return ueditops_from_cost_matrix(len1, string1, len1o,
                                   len2, string2, len2o,
                                   matrix, n);
}

/*
 * Convert an array of difflib-like block ops to atomic ops.
 *
 * Returns a newly allocated array.
 */
LEV_STATIC_PY LevEditOp*
lev_opcodes_to_editops(size_t nb, const LevOpCode *bops,
                       size_t *n, int keepkeep)
{
  size_t i;
  const LevOpCode *b;
  LevEditOp *ops, *o;

  /* compute the number of atomic operations */
  *n = 0;
  if (!nb)
    return NULL;

  b = bops;
  if (keepkeep) {
    for (i = nb; i; i--, b++) {
      size_t sd = b->send - b->sbeg;
      size_t dd = b->dend - b->dbeg;
      *n += (sd > dd ? sd : dd);
    }
  }
  else {
    for (i = nb; i; i--, b++) {
      size_t sd = b->send - b->sbeg;
      size_t dd = b->dend - b->dbeg;
      *n += (b->type != LEV_EDIT_KEEP ? (sd > dd ? sd : dd) : 0);
    }
  }

  /* convert */
  o = ops = (LevEditOp*)malloc((*n)*sizeof(LevEditOp));
  if (!ops) {
    *n = (size_t)(-1);
    return NULL;
  }
  b = bops;
  for (i = nb; i; i--, b++) {
    size_t j;

    switch (b->type) {
      case LEV_EDIT_KEEP:
      if (keepkeep) {
        for (j = 0; j < b->send - b->sbeg; j++, o++) {
          o->type = LEV_EDIT_KEEP;
          o->spos = b->sbeg + j;
          o->dpos = b->dbeg + j;
        }
      }
      break;

      case LEV_EDIT_REPLACE:
      for (j = 0; j < b->send - b->sbeg; j++, o++) {
        o->type = LEV_EDIT_REPLACE;
        o->spos = b->sbeg + j;
        o->dpos = b->dbeg + j;
      }
      break;

      case LEV_EDIT_DELETE:
      for (j = 0; j < b->send - b->sbeg; j++, o++) {
        o->type = LEV_EDIT_DELETE;
        o->spos = b->sbeg + j;
        o->dpos = b->dbeg;
      }
      break;

      case LEV_EDIT_INSERT:
      for (j = 0; j < b->dend - b->dbeg; j++, o++) {
        o->type = LEV_EDIT_INSERT;
        o->spos = b->sbeg;
        o->dpos = b->dbeg + j;
      }
      break;

      default:
      break;
    }
  }
  assert((size_t)(o - ops) == *n);

  return ops;
}

/*
 * Convert an array of atomic ops to difflib-like blocks.
 *
 * Returns a newly allocated array.  len1, len2 are string lengths (needed for
 * the last KEEP block).
 */
LEV_STATIC_PY LevOpCode*
lev_editops_to_opcodes(size_t n, const LevEditOp *ops, size_t *nblocks,
                       size_t len1, size_t len2)
{
  size_t nb, i, spos, dpos;
  const LevEditOp *o;
  LevOpCode *bops, *b;
  LevEditType type;

  /* compute the number of blocks */
  nb = 0;
  o = ops;
  spos = dpos = 0;
  type = LEV_EDIT_KEEP;
  for (i = n; i; ) {
    /* simply pretend there are no keep blocks */
    while (o->type == LEV_EDIT_KEEP && --i)
      o++;
    if (!i)
      break;
    if (spos < o->spos || dpos < o->dpos) {
      nb++;
      spos = o->spos;
      dpos = o->dpos;
    }
    nb++;
    type = o->type;
    switch (type) {
      case LEV_EDIT_REPLACE:
      do {
        spos++;
        dpos++;
        i--;
        o++;
      } while (i && o->type == type && spos == o->spos && dpos == o->dpos);
      break;

      case LEV_EDIT_DELETE:
      do {
        spos++;
        i--;
        o++;
      } while (i && o->type == type && spos == o->spos && dpos == o->dpos);
      break;

      case LEV_EDIT_INSERT:
      do {
        dpos++;
        i--;
        o++;
      } while (i && o->type == type && spos == o->spos && dpos == o->dpos);
      break;

      default:
      break;
    }
  }
  if (spos < len1 || dpos < len2)
    nb++;

  /* convert */
  b = bops = (LevOpCode*)malloc(nb*sizeof(LevOpCode));
  if (!bops) {
    *nblocks = (size_t)(-1);
    return NULL;
  }
  o = ops;
  spos = dpos = 0;
  type = LEV_EDIT_KEEP;
  for (i = n; i; ) {
    /* simply pretend there are no keep blocks */
    while (o->type == LEV_EDIT_KEEP && --i)
      o++;
    if (!i)
      break;
    b->sbeg = spos;
    b->dbeg = dpos;
    if (spos < o->spos || dpos < o->dpos) {
      b->type = LEV_EDIT_KEEP;
      spos = b->send = o->spos;
      dpos = b->dend = o->dpos;
      b++;
      b->sbeg = spos;
      b->dbeg = dpos;
    }
    type = o->type;
    switch (type) {
      case LEV_EDIT_REPLACE:
      do {
        spos++;
        dpos++;
        i--;
        o++;
      } while (i && o->type == type && spos == o->spos && dpos == o->dpos);
      break;

      case LEV_EDIT_DELETE:
      do {
        spos++;
        i--;
        o++;
      } while (i && o->type == type && spos == o->spos && dpos == o->dpos);
      break;

      case LEV_EDIT_INSERT:
      do {
        dpos++;
        i--;
        o++;
      } while (i && o->type == type && spos == o->spos && dpos == o->dpos);
      break;

      default:
      break;
    }
    b->type = type;
    b->send = spos;
    b->dend = dpos;
    b++;
  }
  if (spos < len1 || dpos < len2) {
    assert(len1 - spos == len2 - dpos);
    b->type = LEV_EDIT_KEEP;
    b->sbeg = spos;
    b->dbeg = dpos;
    b->send = len1;
    b->dend = len2;
    b++;
  }
  assert((size_t)(b - bops) == nb);

  *nblocks = nb;
  return bops;
}

/*
 * Apply a sequence of block editing operations to a string.
 *
 * Returns a newly allocated string.  bops is not checked!
 */
LEV_STATIC_PY lev_byte*
lev_opcodes_apply(size_t len1, const lev_byte *string1,
                  size_t len2, const lev_byte *string2,
                  size_t nb, const LevOpCode *bops,
                  size_t *len)
{
  lev_byte *dst, *dpos;  /* destination string */
  const lev_byte *spos;  /* source string position */
  size_t i;

  /* this looks too complex for such a simple task, but note ops is not
   * a complete edit sequence, we have to be able to apply anything anywhere */
  dpos = dst = (lev_byte*)malloc((len1 + len2)*sizeof(lev_byte));
  if (!dst) {
    *len = (size_t)(-1);
    return NULL;
  }
  spos = string1;
  for (i = nb; i; i--, bops++) {
    switch (bops->type) {
      case LEV_EDIT_INSERT:
      case LEV_EDIT_REPLACE:
      memcpy(dpos, string2 + bops->dbeg,
             (bops->dend - bops->dbeg)*sizeof(lev_byte));
      break;

      case LEV_EDIT_KEEP:
      memcpy(dpos, string1 + bops->sbeg,
             (bops->send - bops->sbeg)*sizeof(lev_byte));
      break;

      default:
      break;
    }
    spos += bops->send - bops->sbeg;
    dpos += bops->dend - bops->dbeg;
  }

  *len = dpos - dst;
  /* possible realloc failure is detected with *len != 0 */
  return realloc(dst, *len*sizeof(lev_byte));
}

/*
 * Apply a sequence of block editing operations to a string. (Unicode)
 *
 * Returns a newly allocated string.  bops is not checked!
 */
LEV_STATIC_PY lev_wchar*
lev_u_opcodes_apply(size_t len1, const lev_wchar *string1,
                    size_t len2, const lev_wchar *string2,
                    size_t nb, const LevOpCode *bops,
                    size_t *len)
{
  lev_wchar *dst, *dpos;  /* destination string */
  const lev_wchar *spos;  /* source string position */
  size_t i;

  /* this looks too complex for such a simple task, but note ops is not
   * a complete edit sequence, we have to be able to apply anything anywhere */
  dpos = dst = (lev_wchar*)malloc((len1 + len2)*sizeof(lev_wchar));
  if (!dst) {
    *len = (size_t)(-1);
    return NULL;
  }
  spos = string1;
  for (i = nb; i; i--, bops++) {
    switch (bops->type) {
      case LEV_EDIT_INSERT:
      case LEV_EDIT_REPLACE:
      memcpy(dpos, string2 + bops->dbeg,
             (bops->dend - bops->dbeg)*sizeof(lev_wchar));
      break;

      case LEV_EDIT_KEEP:
      memcpy(dpos, string1 + bops->sbeg,
             (bops->send - bops->sbeg)*sizeof(lev_wchar));
      break;

      default:
      break;
    }
    spos += bops->send - bops->sbeg;
    dpos += bops->dend - bops->dbeg;
  }

  *len = dpos - dst;
  /* possible realloc failure is detected with *len != 0 */
  return realloc(dst, *len*sizeof(lev_wchar));
}

/*
 * Invert the sense of block ops (src <-> dest).
 *
 * Changed in place.
 */
LEV_STATIC_PY void
lev_opcodes_inverse(size_t n, LevOpCode *bops)
{
  size_t i;

  for (i = n; i; i--, bops++) {
    size_t z;

    z = bops->dbeg;
    bops->dbeg = bops->sbeg;
    bops->sbeg = z;
    z = bops->dend;
    bops->dend = bops->send;
    bops->send = z;
    if (bops->type & 2)
      bops->type ^= 1;
  }
}

/*
 * Find matching blocks.
 *
 * Returns a newly allocated array.  len1, len2 are string lengths.
 */
LEV_STATIC_PY LevMatchingBlock*
lev_editops_matching_blocks(size_t len1,
                            size_t len2,
                            size_t n,
                            const LevEditOp *ops,
                            size_t *nmblocks)
{
  size_t nmb, i, spos, dpos;
  LevEditType type;
  const LevEditOp *o;
  LevMatchingBlock *mblocks, *mb;

  /* compute the number of matching blocks */
  nmb = 0;
  o = ops;
  spos = dpos = 0;
  type = LEV_EDIT_KEEP;
  for (i = n; i; ) {
    /* simply pretend there are no keep blocks */
    while (o->type == LEV_EDIT_KEEP && --i)
      o++;
    if (!i)
      break;
    if (spos < o->spos || dpos < o->dpos) {
      nmb++;
      spos = o->spos;
      dpos = o->dpos;
    }
    type = o->type;
    switch (type) {
      case LEV_EDIT_REPLACE:
      do {
        spos++;
        dpos++;
        i--;
        o++;
      } while (i && o->type == type && spos == o->spos && dpos == o->dpos);
      break;

      case LEV_EDIT_DELETE:
      do {
        spos++;
        i--;
        o++;
      } while (i && o->type == type && spos == o->spos && dpos == o->dpos);
      break;

      case LEV_EDIT_INSERT:
      do {
        dpos++;
        i--;
        o++;
      } while (i && o->type == type && spos == o->spos && dpos == o->dpos);
      break;

      default:
      break;
    }
  }
  if (spos < len1 || dpos < len2)
    nmb++;

  /* fill the info */
  mb = mblocks = (LevMatchingBlock*)malloc(nmb*sizeof(LevOpCode));
  if (!mblocks) {
    *nmblocks = (size_t)(-1);
    return NULL;
  }
  o = ops;
  spos = dpos = 0;
  type = LEV_EDIT_KEEP;
  for (i = n; i; ) {
    /* simply pretend there are no keep blocks */
    while (o->type == LEV_EDIT_KEEP && --i)
      o++;
    if (!i)
      break;
    if (spos < o->spos || dpos < o->dpos) {
      mb->spos = spos;
      mb->dpos = dpos;
      mb->len = o->spos - spos;
      spos = o->spos;
      dpos = o->dpos;
      mb++;
    }
    type = o->type;
    switch (type) {
      case LEV_EDIT_REPLACE:
      do {
        spos++;
        dpos++;
        i--;
        o++;
      } while (i && o->type == type && spos == o->spos && dpos == o->dpos);
      break;

      case LEV_EDIT_DELETE:
      do {
        spos++;
        i--;
        o++;
      } while (i && o->type == type && spos == o->spos && dpos == o->dpos);
      break;

      case LEV_EDIT_INSERT:
      do {
        dpos++;
        i--;
        o++;
      } while (i && o->type == type && spos == o->spos && dpos == o->dpos);
      break;

      default:
      break;
    }
  }
  if (spos < len1 || dpos < len2) {
    assert(len1 - spos == len2 - dpos);
    mb->spos = spos;
    mb->dpos = dpos;
    mb->len = len1 - spos;
    mb++;
  }
  assert((size_t)(mb - mblocks) == nmb);

  *nmblocks = nmb;
  return mblocks;
}

/*
 * Find matching blocks.
 *
 * Returns a newly allocated array.  len1, len2 are string lengths.
 */
LEV_STATIC_PY LevMatchingBlock*
lev_opcodes_matching_blocks(size_t len1,
                            __attribute__((unused)) size_t len2,
                            size_t nb,
                            const LevOpCode *bops,
                            size_t *nmblocks)
{
  size_t nmb, i;
  const LevOpCode *b;
  LevMatchingBlock *mblocks, *mb;

  /* compute the number of matching blocks */
  nmb = 0;
  b = bops;
  for (i = nb; i; i--, b++) {
    if (b->type == LEV_EDIT_KEEP) {
      nmb++;
      /* adjacent KEEP blocks -- we never produce it, but... */
      while (i && b->type == LEV_EDIT_KEEP) {
        i--;
        b++;
      }
      if (!i)
        break;
    }
  }

  /* convert */
  mb = mblocks = (LevMatchingBlock*)malloc(nmb*sizeof(LevOpCode));
  if (!mblocks) {
    *nmblocks = (size_t)(-1);
    return NULL;
  }
  b = bops;
  for (i = nb; i; i--, b++) {
    if (b->type == LEV_EDIT_KEEP) {
      mb->spos = b->sbeg;
      mb->dpos = b->dbeg;
      /* adjacent KEEP blocks -- we never produce it, but... */
      while (i && b->type == LEV_EDIT_KEEP) {
        i--;
        b++;
      }
      if (!i) {
        mb->len = len1 - mb->spos;
        mb++;
        break;
      }
      mb->len = b->sbeg - mb->spos;
      mb++;
    }
  }
  assert((size_t)(mb - mblocks) == nmb);

  *nmblocks = nmb;
  return mblocks;
}
