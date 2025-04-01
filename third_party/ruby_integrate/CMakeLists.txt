# Integrate CRuby Interpreter
cmake_minimum_required(VERSION 3.20.1)

#--------------------------------------------------------------------------------
# Source of Interpreter
#--------------------------------------------------------------------------------

set(INTEGRATE_RUBY_SOURCE
  addr2line.c
  array.c
  bignum.c
  class.c
  compar.c
  compile.c
  complex.c
  cont.c
  debug.c
  dir.c
  dln.c
  dln_find.c
  dmydln.c
  dmyencoding.c
  dmyext.c
  dmyversion.c
  encoding.c
  enum.c
  enumerator.c
  error.c
  eval.c
  file.c
  gc.c
  goruby.c
  hash.c
  id.c
  inits.c
  io.c
  iseq.c
  lex.c
  load.c
  main.c
  marshal.c
  math.c
  miniprelude.c
  newline.c
  node.c
  numeric.c
  object.c
  pack.c
  parse.c
  proc.c
  process.c
  random.c
  range.c
  rational.c
  re.c
  regcomp.c
  regenc.c
  regerror.c
  regexec.c
  regparse.c
  regsyntax.c
  ruby.c
  safe.c
  signal.c
  siphash.c
  sparc.c
  sprintf.c
  st.c
  strftime.c
  string.c
  struct.c
  thread.c
  thread_pthread.c
  thread_win32.c
  time.c
  transcode.c
  util.c
  variable.c
  version.c
  vm.c
  vm_dump.c
  vsnprintf.c
)

#--------------------------------------------------------------------------------
# Define Static Library
#--------------------------------------------------------------------------------

add_library(integrate_ruby ${INTEGRATE_RUBY_SOURCE})
target_include_directories(integrate_ruby PUBLIC include)
