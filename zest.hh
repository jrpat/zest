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
**  Define a test and run assertions:
**
**      TEST(GroupName, "description")
**      {
**        is_eq(expected, actual);  // actual == expected
**        is_ne(expected, actual);  // actual != expected
**        is_gt(expected, actual);  // actual >  expected
**        is_lt(expected, actual);  // actual <  expected
**        is_ge(expected, actual);  // actual >= expected
**        is_le(expected, actual);  // actual <= expected
**
**        // assertions return booleans
**        bool ok = is_eq(expected, actual);
**      }
**
**  If the assertion syntax seems backward,
**  think of each assertion as a named function:
**
**      is_gt(3, 4) → is_gt_3(4);  // true
**      is_gt(4, 3) → is_gt_4(3);  // false
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
**  Get a reference to the current TestCase:
**
**      zest::current()  // TestCase& / throws if no current test
**
**
**
**  COLORS
**  ------
**
**  By default, zest tries to be smart about whether to output ANSI
**  color codes. It checks for a tty, non-dumb $TERM, and $NO_COLOR.
**  You can also instruct it explicitly:
**
**      zest::color(true)                // enable color output
**      zest::color(false)               // disable color output
**      zest::color(zest::autocolor())   // figure out color output
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
**  The only restriction on TestCase subclasses is that they must be
**  default-constructible.
**
**  From within either hook, you can fail the test using fail().
**  To print a simple failure message, just pass it a string:
**
**      fail("message");
**
**  With no argument, fail() returns the output stream:
**
**      fail() << a << b << c << std::endl;  // (don't forget std::endl)
**
**  See zest::TestCase for other member functions and variables.
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
**            if (count < 0) fail("Count too low!");
**          }
**      };
**
**  Then use the `ZEST_TEST` macro to define a syntax for your new type:
**
**      #define COUNTER_TEST(group, title) \
**        ZEST_TEST(CounterTestCase, group, title)
**
**  Inside your tests, you can use `zest::current` to get
**  a reference to the current test:
**
**      zest::current<CounterTestCase>()  // CounterTestCase&
**
**  Then define counter tests like you'd expect:
**
**      COUNTER_TEST(MyGroup, "passing test")
**      {
**        ZPRN("-- test --");
**        zest::current<CounterTest>().count = 99;
**      }
**
**      COUNTER_TEST(MyGroup, "failing test")
**      {
**        zest::current<CounterTest>().count = -1;
**      }
**
**  "passing test" will output:
**
**      before count = 0
**      -- test --
**      after count = 99
**
**  and "failing test" will fail with a standard failure output:
**
**      /path/to/file:137: FAIL: Count too low!
*/

#include <exception>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>
#include <unistd.h>

namespace zest
{

#define ZOUT(x) do{ std::cout << x << std::flush; }while(0)
#define ZPRN(x) do{ std::cout << x << std::endl; }while(0)
#define ZLOG(x) ZPRN(#x " = " << (x))

static inline const char *cRED, *cGRN, *cDIM, *cOFF=0;
inline bool autocolor() {
  return isatty(fileno(stdout)) && !getenv("NO_COLOR") &&
    (getenv("TERM") && (getenv("TERM") != std::string_view("dumb")));
}
inline void color(bool enabled) {
  cRED = !enabled ? "" : "\033[31m";
  cGRN = !enabled ? "" : "\033[32m";
  cDIM = !enabled ? "" : "\033[38;5;8m";
  cOFF = !enabled ? "" : "\033[m";
}

template <class T>
concept Printable = requires(std::ostream& os, T x) { os << x; };

template <Printable T>
std::ostream& operator<<(std::ostream& os, std::optional<T> opt)
{ if (opt){ os << *opt; } else { os << "(none)"; } return os; }

inline std::ostream& operator<<(std::ostream& os, const std::exception& e)
{ os << e.what(); return os; }

using Str = std::string;
using Run = void();


struct TestCase {
  Run* run;
  Str title;
  Str file;
  int line;
  int failed = 0;
  bool done = false;

  std::ostream& output() {
    if (!done && (failed == 1)) ZPRN(cRED << " ✗ " << title << cOFF);
    else if (done && (!failed)) ZPRN(cGRN << " ✓ " << title << cOFF);
    return std::cout;
  }

  inline std::ostream& fail(const Str& file, int line)  {
    ++failed;
    return output() << file << ":" << line << ": " << "FAIL: ";
  }

  inline std::ostream& fail(const Str& msg = "") {
    return fail(file,line) << msg << (msg.empty() ? "" : "\n");
  }

  inline virtual void before() {}
  inline virtual void after() {}
  virtual ~TestCase() {}
};

class Test { public: template <class T> static void run(); };
struct Flag { static inline TestCase skip, only; };

struct Runner {
  static inline std::map<Str, std::vector<TestCase*>> groups;
  static inline TestCase* current = nullptr;
  static inline bool only_mode = false;

  static bool add(TestCase& c, Str g, Str t, Run* r, Str f, int l) {
    c.run=r; c.title=t; c.file=f; c.line=l;
    groups[g].push_back(&c);
    return true;
  }

  static void skip(Str group) {
    auto& g = groups[group];
    g.insert(g.begin(), &Flag::skip);
  }

  static void only(Str group) {
    auto& g = groups[group];
    g.insert(g.begin(), &Flag::only);
    only_mode = true;
  }

  static inline int run() {
    int nfail=0, nskip=0;
    if (!cOFF) { color(autocolor()); }
    for (auto& [group, tests] : groups) {
      if ((tests[0] == &Flag::skip) ||
          ((tests[0] != &Flag::only) && only_mode)) {
        nskip += tests.size() - 1;
        continue;
      }
      ZPRN("\n[" << group << "]");
      for (auto t : tests) {
        if (!t->run) { continue; }
        current = t;
        t->before();
        try {
          t->run();
        } catch (...) {
          t->fail("Uncaught exception");
          std::rethrow_exception(std::current_exception());
        }
        t->after();
        t->done = true;
        t->output();
        nfail += t->failed ? 1 : 0;
        current = nullptr;
      }
    }
    auto C = nfail ? cRED : cGRN;
    printf("%s\n┌──────┐", C);
    printf("%s\n│ %-4s │", C, (nfail ? "FAIL" : " OK "));
    if (nskip) printf("%s (%d skipped)", cDIM, nskip);
    printf("%s\n└──────┘", C);
    printf("%s\n", cOFF);
    return nfail ? 1 : 0;
  }
};


#define ZEST_IS_FN(NAME, COMP)                                         \
  template <class LHS, class RHS>                                      \
  bool is_##NAME##_(Str f, size_t l,                                   \
                    Str lhs_str, const LHS& lhs,                       \
                    Str rhs_str, const RHS& rhs) {                     \
    TestCase* test = Runner::current;                                  \
    if (!test) { throw "Called is_" #NAME " while no current test"; }  \
    if (test->done) { throw "Called is_" #NAME " in finished test"; }  \
    if ((rhs COMP lhs)) { return true; }                               \
    auto& out = test->fail(f,l) << rhs_str << " " #COMP " " << lhs_str;\
    if constexpr (Printable<LHS> && Printable<RHS>)                    \
      out << "  (" << rhs << " " #COMP " " << lhs << ")";              \
    out << "\n"; return false; }

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
  template <> void ZEST_FUN(g)();                                      \
  bool ZEST_CLS(g)::b = zest::Runner::add(                             \
    ZEST_CLS(g)::c, #g, t, ZEST_FUN(g), __FILE__, __LINE__);           \
  template <> void ZEST_FUN(g)()


static auto& run = Runner::run;
static auto& skip = Runner::skip;
static auto& only = Runner::only;

template <class T = TestCase>
static inline T& current() {
  if (!Runner::current)
    throw "Called zest::current() while no current test";
  return dynamic_cast<T&>(*Runner::current);
}

#define TEST(Group, Title) ZEST_TEST(zest::TestCase, Group, Title)
#define is_eq(e,a) zest::is_eq_(__FILE__,__LINE__, #e,(e), #a,(a))
#define is_ne(e,a) zest::is_ne_(__FILE__,__LINE__, #e,(e), #a,(a))
#define is_gt(e,a) zest::is_gt_(__FILE__,__LINE__, #e,(e), #a,(a))
#define is_lt(e,a) zest::is_lt_(__FILE__,__LINE__, #e,(e), #a,(a))
#define is_ge(e,a) zest::is_ge_(__FILE__,__LINE__, #e,(e), #a,(a))
#define is_le(e,a) zest::is_le_(__FILE__,__LINE__, #e,(e), #a,(a))

} // namespace zest

