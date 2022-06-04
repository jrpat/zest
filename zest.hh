#pragma once

/*
**  zest
**  ====
**
**  A `z`epto-scale t`est`ing library for C++20
**
**
**  USAGE
**  -----
**
**  Define a test:
**
**      TEST(GroupName, "description")
**      {
**        is_eq(expected, actual);  // expected == actual
**        is_ne(expected, actual);  // expected != actual
**        is_gt(expected, actual);  // expected >  actual
**        is_lt(expected, actual);  // expected <  actual
**        is_ge(expected, actual);  // expected >= actual
**        is_le(expected, actual);  // expected <= actual
**      }
**
**
**  Run all the tests:
**
**      int exit_status = zest::run();  // 0=pass / 1=fail
**
**
**  Skip certain groups:
**
**      zest::skip("GroupName");   // call this before zest::run()
**      zest::skip("OtherGroup");
**
**
**  Only run certain groups:
**
**      zest::only("JustThisGroup");  // call this before zest::run()
**      zest::only("OhAlsoThisOne");
**
**
**  Allow test cases to access private members of a class:
**
**      class MyClass
**      {
**        private:
**          friend class ::zest::Test;
**      };
**
**
**  Small helpers for debugging:
**
**      ZOUT(x << y << z)  // print with no newline
**      ZPRN(x << y << z)  // print with a newline
**      ZLOG(x)            // ZPRN(#x " = " << x)
**
**
**  Get a reference to the current test from inside a TEST:
**
**      THIS_TEST()  // zest::TestCase&
**
**
**
**  LAMBDAS
**  -------
**
**  Assertions require access to the current test pointer, which is a
**  local variable inside functions. That means that lambdas in TESTs
**  must capture, either by value or reference.
**
**      TEST(MyGroup, "lambda test")
**      {
**        auto good_by_val = [=](int x){ is_eq(99, x); }
**        auto good_by_ref = [&](int x){ is_eq(99, x); }
**        auto good_manual = [THIS_TEST()](int x){ is_eq(99, x); }
**        auto ERROR = [](int x){ is_eq(99, x); }
**      }
**
**
**
**  CONFIGURATION
**  -------------
**
**  Zest tries to be smart about whether to output ANSI colors
**  with its output. You shouldn't ever have to think about it.
**  But if you want to disable color output entirely,
**  define `ZEST_NO_COLOR` before #include'ing zest.hh:
**
**      c++ ... -DZEST_NO_COLOR
**
**
**
**  CUSTOM TEST TYPES
**  -----------------
**
**  You can create custom test types by inheriting from zest::TestCase.
**  Custom test types can implement hooks `before()` and `after()`
**  which run before and after the test function, respectively.
**
**  This feature has been designed to have minimal boilerplate, so it's
**  useful in a variety of situations from simple shared setup/teardown
**  for a handful of tests, to more advanced uses like automatically
**  awaiting futures or doing custom reporting.
**
**  Look at zest::TestCase for available member functions and variables.
**
**  The only restriction on TestCase subclasses is that they must be
**  default-constructible.
**
**  Example:
**
**      class CounterTestCase : public zest::TestCase
**      {
**        public: // test cases do not have access to private members
**          int count;
**          void before() override {
**            std::cout << "before count = " << count;
**          }
**          void after() override {
**            std::cout << "after count = " << count;
**            if (count < 0) { fail() << "Count too low!"; }
**          }
**      };
**
**  Inside your tests, you can use the `THIS_TEST_AS` macro to get
**  a reference to the current subclass test:
**
**      THIS_TEST_AS(CounterTestCase)  // CounterTestCase&
**
**  As an additional convenience, you can use the `ZEST_TEST` macro to
**  provide a "native-feeling" syntax for your new test type:
**
**      #define COUNTER_TEST(group, title) \
**        ZEST_TEST(CounterTestCase, group, title)
**
**  Then define counter tests like you'd expect:
**
**      COUNTER_TEST(MyGroup, "passing test")
**      {
**        ZPRN("-- test --");
**        THIS_TEST_AS(CounterTest).count = 99;
**      }
**
**      COUNTER_TEST(MyGroup, "failing test")
**      {
**        THIS_TEST_AS(CounterTest).count = -1;
**      }
**
**
**  "passing test" will output:
**
**      before count = 0
**      -- test --
**      after count = 99
**
**  and "failing test" will fail with a "Count too low!" message.
*/


#include <iostream>
#include <optional>
#include <string>
#include <map>
#include <set>
#include <vector>

#if !defined(ZEST_NO_COLOR)
# include <unistd.h>
# include <cstdlib>
#endif


#define ZOUT(x) do{ std::cout << x << std::flush; }while(0)
#define ZPRN(x) do{ std::cout << x << std::endl << std::flush; }while(0)
#define ZLOG(x) ZPRN(#x " = " << (x))


namespace zest
{

struct TestCase;
using Str = std::string;
using RunFunc = void(TestCase&);


#if defined(ZEST_NO_COLOR)
static inline bool DOCOLOR = false;
#else
static inline bool DOCOLOR = isatty(fileno(stdout)) && getenv("TERM")
                          && (getenv("TERM") != std::string_view("dumb"));
#endif

static inline auto CLR_RED = !DOCOLOR ? "" : "\033[31m";
static inline auto CLR_GRN = !DOCOLOR ? "" : "\033[32m";
static inline auto CLR_DIM = !DOCOLOR ? "" : "\033[38;5;8m";
static inline auto CLR_OFF = !DOCOLOR ? "" : "\033[m";


template <class T>
concept Printable = requires(std::ostream& os, T x) { os << x; };

template <Printable T>
std::ostream& operator<<(std::ostream& os, std::optional<T> opt)
{
  if (opt){ os << *opt; } else { os << "(none)"; }
  return os;
}


struct Runner
{
    static inline std::map<Str, std::vector<TestCase*>> groups;
    static inline bool only_mode = false;
    static inline void skip(Str);
    static inline void only(Str);
    static inline int run();
    static inline bool add(TestCase&, Str, Str, RunFunc*, Str, int);
};

struct TestCase
{
    RunFunc* run;
    Str title;
    Str file;
    int line;
    int failed = 0;
    bool done = false;

    std::ostream& output() {
      if (!done && (failed == 1)) {
        ZPRN(CLR_RED << " ✗ " << title << CLR_OFF);
      } else if (done && (failed == 0)) {
        ZPRN(CLR_GRN << " ✔ " << title << CLR_OFF);
      }
      return std::cout;
    }

    inline std::ostream& fail(const Str& file, int line)  {
      ++failed;
      output() << file << ":" << line << ": " << "FAIL: ";
      return std::cout;
    }

    inline std::ostream& fail() { return fail(file, line); }

    inline virtual void before() {}
    inline virtual void after() {}
};

class Test { public: template <class T> static void run(TestCase&); };
struct Flag { static inline TestCase skip, only; };

bool Runner::add(TestCase& c, Str g, Str t, RunFunc* r, Str f, int l) {
  c.run=r; c.title=t; c.file=f; c.line=l;
  groups[g].push_back(&c);
  return true;
}

int Runner::run() {
  int nfail = 0;
  int nskip = 0;
  for (auto& [group, tests] : groups) {
    ZPRN("\n[" << group << "]");
    if ((tests[0] == &Flag::skip) ||
        ((tests[0] != &Flag::only) && only_mode)) {
      nskip += tests.size() - 1;
      ZPRN(CLR_DIM << "  …skipping…" << CLR_OFF);
      continue;
    }
    for (auto test : tests) {
      if (!test->run) { continue; }
      test->before();
      test->run(*test);
      test->after();
      test->done = true;
      test->output();
      nfail += test->failed ? 1 : 0;
    }
  }
  auto CLR = nfail ? CLR_RED : CLR_GRN;
  printf("\n");
  printf("%s\n┌──────┐", CLR);
  printf("%s\n│ %-4s │", CLR, (nfail ? "FAIL" : "PASS"));
  if (nskip) { printf("%s (%d skipped)", CLR_DIM, nskip); }
  printf("%s\n└──────┘\n", CLR);
  printf("\n\n%s", CLR_OFF);
  return nfail ? 1 : 0;
}

void Runner::skip(Str group) {
  auto& g = groups[group];
  g.insert(g.begin(), &Flag::skip);
}

void Runner::only(Str group) {
  auto& g = groups[group];
  g.insert(g.begin(), &Flag::only);
  only_mode = true;
}

#define ZEST_IS_FN(NAME, COMP)                                         \
  template <class LHS, class RHS>                                      \
  bool is_##NAME##_(                                                   \
    zest::TestCase& test,                                              \
    zest::Str lhs_str, const LHS& lhs,                                 \
    zest::Str rhs_str, const RHS& rhs,                                 \
    zest::Str file, size_t line)                                       \
  {                                                                    \
    if (test.done) { throw "is_" #NAME " in finished test"; }          \
    if ((lhs COMP rhs)) { return true; }                               \
    auto& out = test.fail(file, line)                                  \
      << lhs_str << " " #COMP " " << rhs_str;                          \
    if constexpr (zest::Printable<RHS>) out << " (got " << rhs << ")"; \
    out << "\n";                                                       \
    return false;                                                      \
  }                                                                    \

ZEST_IS_FN(eq, ==)
ZEST_IS_FN(ne, !=)
ZEST_IS_FN(gt, >)
ZEST_IS_FN(lt, <)
ZEST_IS_FN(ge, >=)
ZEST_IS_FN(le, <=)


#define ZEST_FULLNAME_(pre, grp, uniq) pre##_##grp##_##uniq
#define ZEST_FULLNAME(pre, grp, uniq) ZEST_FULLNAME_(pre, grp, uniq)

#define ZEST_CLS(grp) ZEST_FULLNAME(_testcls, grp, __LINE__)
#define ZEST_FUN(grp) zest::Test::run<ZEST_CLS(grp)>

#define ZEST_TEST(C, g, t)                                             \
  struct ZEST_CLS(g) {static inline C c; static bool b;};              \
  template <> void ZEST_FUN(g)(zest::TestCase&);                       \
  bool ZEST_CLS(g)::b = zest::Runner::add(                             \
    ZEST_CLS(g)::c, #g, t, ZEST_FUN(g), __FILE__, __LINE__);           \
  template <> void ZEST_FUN(g)(zest::TestCase& _z_t)                   \

} // namespace zest


////////////////////////////////////////////////////////////////////////
// Public API

#define THIS_TEST() (_z_t)
#define THIS_TEST_AS(T) (dynamic_cast<T&>(_z_t))
#define TEST(Group, Title) ZEST_TEST(zest::TestCase, Group, Title)
#define is_eq(e,a) zest::is_eq_(_z_t, #e,(e), #a,(a), __FILE__,__LINE__)
#define is_ne(e,a) zest::is_ne_(_z_t, #e,(e), #a,(a), __FILE__,__LINE__)
#define is_gt(e,a) zest::is_gt_(_z_t, #e,(e), #a,(a), __FILE__,__LINE__)
#define is_lt(e,a) zest::is_lt_(_z_t, #e,(e), #a,(a), __FILE__,__LINE__)
#define is_ge(e,a) zest::is_ge_(_z_t, #e,(e), #a,(a), __FILE__,__LINE__)
#define is_le(e,a) zest::is_le_(_z_t, #e,(e), #a,(a), __FILE__,__LINE__)
namespace zest {
static auto& run = Runner::run;
static auto& skip = Runner::skip;
static auto& only = Runner::only;
}

////////////////////////////////////////////////////////////////////////

