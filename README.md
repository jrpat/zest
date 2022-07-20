> This readme is automatically generated from `zest.hh` and `t/test`. See `Makefile`.

zest
====

A `z`epto-scale t`est`ing library for C++20


USAGE
-----

Define a test and run assertions:

    TEST(GroupName, "description")
    {
      is_eq(expected, actual);  // actual == expected
      is_ne(expected, actual);  // actual != expected
      is_gt(expected, actual);  // actual >  expected
      is_lt(expected, actual);  // actual <  expected
      is_ge(expected, actual);  // actual >= expected
      is_le(expected, actual);  // actual <= expected

      is_thrown(type, body);    // expect body to throw type

      // assertions return booleans
      bool ok = is_eq(expected, actual);
    }

If the assertion syntax seems backward,
think of each assertion as a named function:

    is_gt(3, 4) → is_gt_3(4);  // true
    is_gt(4, 3) → is_gt_4(3);  // false

You can check that a certain type of exception is thrown:

    is_thrown(expected_error_type, body)


Run all the tests:

    int exit_status = zest::run();  // 0=pass / 1=fail


Skip certain groups:

    zest::skip("GroupName");   // call this before zest::run()
    zest::skip("OtherGroup");


Only run certain groups:

    zest::only("JustThisGroup");  // call this before zest::run()
    zest::only("OhAlsoThisOne");


Allow test cases to access private members of a class:

    class MyClass
    {
      private:
        friend class ::zest::Test;
    };


Small helpers for debugging:

    ZOUT(x << y << z)  // print with no newline
    ZPRN(x << y << z)  // print with a newline
    ZLOG(x)            // ZPRN(#x " = " << x)


Get a reference to the current TestCase:

    zest::current()  // TestCase& / throws if no current test



COLORS
------

By default, zest tries to be smart about whether to output ANSI
color codes. It checks for a tty, non-dumb $TERM, and $NO_COLOR.
You can also instruct it explicitly:

    zest::color(true)                // enable color output
    zest::color(false)               // disable color output
    zest::color(zest::autocolor())   // figure out color output



CUSTOM TEST TYPES
-----------------

You can create custom test types by inheriting from zest::TestCase.
Custom test types can implement hooks `before()` and `after()`
which run before and after the test function, respectively.

This feature has been designed to have minimal boilerplate, so it's
useful in a variety of situations from simple shared setup/teardown
for a handful of tests, to more advanced uses like automatically
awaiting futures or doing custom reporting.

The only restriction on TestCase subclasses is that they must be
default-constructible.

From within either hook, you can fail the test using fail().
To print a simple failure message, just pass it a string:

    fail("message");

With no argument, fail() returns the output stream:

    fail() << a << b << c << std::endl;  // (don't forget std::endl)

See zest::TestCase for other member functions and variables.

Example:

    class CounterTestCase : public zest::TestCase
    {
      public: // test cases do not have access to private members
        int count;
        void before() override {
          std::cout << "before count = " << count;
        }
        void after() override {
          std::cout << "after count = " << count;
          if (count < 0) fail("Count too low!");
        }
    };

Then use the `ZEST_TEST` macro to define a syntax for your new type:

    #define COUNTER_TEST(group, title) \
      ZEST_TEST(CounterTestCase, group, title)

Inside your tests, you can use `zest::current` to get
a reference to the current test:

    zest::current<CounterTestCase>()  // CounterTestCase&

Then define counter tests like you'd expect:

    COUNTER_TEST(MyGroup, "passing test")
    {
      ZPRN("-- test --");
      zest::current<CounterTest>().count = 99;
    }

    COUNTER_TEST(MyGroup, "failing test")
    {
      zest::current<CounterTest>().count = -1;
    }

"passing test" will output:

    before count = 0
    -- test --
    after count = 99

and "failing test" will fail with a standard failure output:

    /path/to/file:137: FAIL: Count too low!


SAMPLE OUTPUT
-------------

The output of `make test-pass`:
```

[FirstGroup]
 ✔ is_ok
 ✔ make_ok
 ✔ final_digits
 ✔ incr_final_digits

[SecondGroup]
 ✔ Mutually Exclusive Types
 ✔ Empty
 ✔ Root Only
 ✔ Absolute Foobars
 ✔ Relative Foobars
 ✔ Absolute Bazquxs
 ✔ Relative Bazquxs
 ✔ Parent
 ✔ Trailing Slash is Ignored
 ✔ Lexically Normalized
 ✔ Various Pathological
 ✔ Comparisons

[ThirdGroup]
 ✔ Delegate
 ✔ Foobar
 ✔ Bazqux
 ✔ Output

[FourthGroup]
 ✔ Basic
 ✔ Construct w/ various types
 ✔ Copy affects refcount
 ✔ Move does not affect refcount
 ✔ Registry auto add/remove
 ✔ std::set can hold
 ✔ std::unordered_set can hold
 ✔ gensym

[FifthGroup]
 ✔ MAKE_x & FIND_x are inverses
 ✔ Components
 ✔ Sortable
 ✔ Equality
 ✔ Next

[SixthGroup]
 ✔ Constructors

[SeventhGroup]
  …skipping…

┌──────┐
│ PASS │ (1 skipped)
└──────┘
```

The output of `make test-fail`:
```

[FirstGroup]
 ✔ is_ok
 ✔ make_ok
 ✔ final_digits
 ✔ incr_final_digits

[SecondGroup]
 ✔ Mutually Exclusive Types
 ✔ Empty
 ✔ Root Only
 ✔ Absolute Foobars
 ✔ Relative Foobars
 ✔ Absolute Bazquxs
 ✔ Relative Bazquxs
 ✔ Parent
 ✔ Trailing Slash is Ignored
 ✔ Lexically Normalized
 ✔ Various Pathological
 ✔ Comparisons

[ThirdGroup]
 ✔ Delegate
 ✔ Foobar
 ✔ Bazqux
 ✔ Output

[FourthGroup]
 ✔ Basic
 ✗ Construct w/ various types
/Users/me/src/project/test/test_fourth.cc:18: FAIL: "z" == s2.name() (got a)
 ✔ Copy affects refcount
 ✔ Move does not affect refcount
 ✔ Registry auto add/remove
 ✔ std::set can hold
 ✔ std::unordered_set can hold
 ✔ gensym

[FifthGroup]
 ✔ MAKE_x & FIND_x are inverses
 ✔ Components
 ✔ Sortable
 ✔ Equality
 ✔ Next

[SixthGroup]
 ✔ Constructors

[SeventhGroup]
  …skipping…

┌──────┐
│ FAIL │ (1 skipped)
└──────┘
```
