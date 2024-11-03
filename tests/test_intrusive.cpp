#include "../src/intrusive/intrusive.h"

#define REQUIRE(b)                                                             \
  {                                                                            \
    if (!(b)) {                                                                \
      std::cout << "WRONG" << std::endl;                                       \
    };                                                                         \
  }

////////////////////////////////////////////////////////////////////////////////

struct MyInt : public SimpleRefCounted<MyInt> {
  MyInt(int value) : value{value} {}

  int value = 0;
};

struct MyString : public SimpleRefCounted<MyString>, public std::string {
  using std::string::basic_string;
};

void TestEmpty() {
  // "Sizeof"
  { REQUIRE(sizeof(IntrusivePtr<MyInt>) == sizeof(void *)); }

  // "Empty state"
  {
    IntrusivePtr<MyInt> a, b;

    b = a;
    IntrusivePtr c(a);
    b = std::move(c);

    REQUIRE(a.Get() == nullptr);
    REQUIRE(b.Get() == nullptr);
    REQUIRE(c.Get() == nullptr);
  }
}

void TestCopyMove() {
  //"Constructors"
  {
    IntrusivePtr<MyString> a{new MyString{"abacaba"}};
    IntrusivePtr<MyString> b = a;
    IntrusivePtr<MyString> c = std::move(a);
    IntrusivePtr<MyString> d = b;
    REQUIRE(c.UseCount() == 3);
    REQUIRE(!a);
    REQUIRE(*b == "abacaba");
    REQUIRE(*c == "abacaba");
    REQUIRE(*d == "abacaba");
  }

  // "Assignment"
  {
    IntrusivePtr<MyString> a{new MyString{"abracadabra"}};
    IntrusivePtr<MyString> b{new MyString{"karabas"}};
    IntrusivePtr<MyString> c = b;
    b = a;
    a = b;
    b = a;
    a = b;
    c = c;            // NOLINT
    c = std::move(c); // NOLINT
    b = std::move(b); // NOLINT
    REQUIRE(*a == "abracadabra");
    REQUIRE(*b == "abracadabra");
    REQUIRE(*c == "karabas");
  }

  // "Mulitple copies"
  {
    std::vector<IntrusivePtr<MyString>> ptrs;
    ptrs.emplace_back(new MyString{"hehe"});

    constexpr int kNumIters = 1000;
    for (size_t i = kNumIters; i-- > 0;) {
      ptrs.emplace_back(ptrs.back());
    }
    for (auto &&ptr : ptrs) {
      REQUIRE(ptr.UseCount() == 1 + kNumIters);
      REQUIRE(ptr.Get() == ptrs.back().Get());
    }

    ptrs.resize(5);
    for (auto &&ptr : ptrs) {
      REQUIRE(ptr.UseCount() == 5);
      REQUIRE(ptr.Get() == ptrs.back().Get());
    }
  }
}

void TestConversions() {
  struct Foo : SimpleRefCounted<Foo> {
    virtual ~Foo() = default;
    virtual int Kek() = 0;
  };

  struct Boo : Foo {
    int value;

    Boo(int value) : value{value} {}

    int Kek() override { return value; }
  };

  IntrusivePtr<Boo> boo = MakeIntrusive<Boo>(123);
  IntrusivePtr<Foo> foo = boo;
  foo = MakeIntrusive<Boo>(42);
  REQUIRE(foo->Kek() == 42);
}

template <typename T> class ObjectCounters {
public:
  ObjectCounters() {
    ++created;
    ++alive;
  }

  ~ObjectCounters() { --alive; }

  static size_t NumAlive() { return alive; }

  static size_t NumCreated() { return created; }

  static void ResetCounters() {
    created = 0;
    alive = 0;
  }

private:
  static inline size_t created = 0;
  static inline size_t alive = 0;
};

struct CountedString : std::string,
                       ObjectCounters<CountedString>,
                       SimpleRefCounted<CountedString> {
  using std::string::basic_string;
};

void TestModifiers() {
  // "Reset()"
  {
    IntrusivePtr<CountedString> p{new CountedString{}};
    REQUIRE(CountedString::NumAlive() == 1);

    p.Reset();
    REQUIRE(CountedString::NumAlive() == 0);
    REQUIRE(p.UseCount() == 0);
    REQUIRE(p.Get() == nullptr);

    p.Reset();
    REQUIRE(CountedString::NumAlive() == 0);
    REQUIRE(p.UseCount() == 0);
    REQUIRE(p.Get() == nullptr);
  }

  CountedString::ResetCounters();

  // "Reset(T*)"
  {
    IntrusivePtr<CountedString> p{new CountedString{"boo"}};
    REQUIRE(CountedString::NumCreated() == 1);
    p.Reset(new CountedString{"foo"});
    REQUIRE(CountedString::NumCreated() == 2);
    REQUIRE(CountedString::NumAlive() == 1);
    REQUIRE(*p == "foo");
  }

  CountedString::ResetCounters();

  // "Swap"
  {
    IntrusivePtr<CountedString> p{new CountedString{"first"}};
    IntrusivePtr<CountedString> q{new CountedString{"second"}};
    REQUIRE(CountedString::NumCreated() == 2);
    REQUIRE(CountedString::NumAlive() == 2);

    p.Swap(p);
    REQUIRE(CountedString::NumCreated() == 2);
    REQUIRE(CountedString::NumAlive() == 2);
    REQUIRE(*p == "first");
    REQUIRE(*q == "second");

    p.Swap(q);
    REQUIRE(CountedString::NumCreated() == 2);
    REQUIRE(CountedString::NumAlive() == 2);
    REQUIRE(*p == "second");
    REQUIRE(*q == "first");
  }
}

void TestObservers() {
  struct IntrusivePair : SimpleRefCounted<IntrusivePair> {
    int first;
    int second;
  };

  // "operator->"
  {
    const IntrusivePtr<IntrusivePair> p(
        new IntrusivePair{.first = 3, .second = 4});
    REQUIRE(p->first == 3);
    REQUIRE(p->second == 4);
    p->first = 5;
    p->second = 6;
    REQUIRE(p->first == 5);
    REQUIRE(p->second == 6);
  }

  // "Dereference"
  {
    const IntrusivePtr<MyInt> p(new MyInt(32));
    REQUIRE((*p).value == 32);
    *p = 3;
    REQUIRE((*p).value == 3);
  }

  // "Dereference 2"
  {
    const IntrusivePtr<MyInt> p(new MyInt(24));
    REQUIRE((*p).value == 24);
    *p = 1;
    IntrusivePtr<MyInt> ptr1(p);
    IntrusivePtr<MyInt> ptr2(p);
    IntrusivePtr<MyInt> ptr3(p);
    ptr2 = std::move(ptr3);
    REQUIRE((*p).value == 1);
  }

  // "operator bool"
  {
    static_assert(std::is_constructible<bool, IntrusivePtr<MyString>>::value,
                  "");
    static_assert(!std::is_convertible<IntrusivePtr<MyString>, bool>::value,
                  "");
    {
      const IntrusivePtr<MyString> p(new MyString("kek"));
      REQUIRE(p);
    }
    {
      const IntrusivePtr<MyString> p;
      REQUIRE(!p);
    }
  }
}

void TestFromRawPointer() {
  MyString *str = new MyString{"Molodoy Krakodil khochet zavesti sebe druzey"};
  IntrusivePtr<MyString> a{str};
  IntrusivePtr<MyString> b{str};
  IntrusivePtr<MyString> c{a};
  IntrusivePtr<MyString> d{c.Get()};
  REQUIRE(str->RefCount() == 4);
}

struct Pinned : SimpleRefCounted<Pinned> {
  Pinned(int tag) : tag_(tag) {}

  Pinned(const Pinned &a) = delete;
  Pinned(Pinned &&a) = delete;

  Pinned &operator=(const Pinned &a) = delete;
  Pinned &operator=(Pinned &&a) = delete;

  ~Pinned() = default;

  int GetTag() const { return tag_; }

private:
  int tag_;
};

void TestNoCopies() { IntrusivePtr<Pinned> p(new Pinned(1)); }

template <typename T> class ObjectInPool;

template <typename T> class ObjectPool {
  static_assert(std::is_base_of_v<ObjectInPool<T>, T>, "Unsupported type");

public:
  template <typename... Args> IntrusivePtr<T> Allocate(Args &&...args) {
    if (!objects_.empty()) {
      std::unique_ptr<T> ptr = std::move(objects_.back());
      objects_.pop_back();
      return IntrusivePtr<T>(ptr.release());
    }
    return DoAllocate(std::forward<Args>(args)...);
  }

  void Release(T *ptr) { objects_.emplace_back(ptr); }

  size_t NumAvailable() const { return objects_.size(); }

  size_t NumInUse() const { return allocated_ - NumAvailable(); }

private:
  template <typename... Args> IntrusivePtr<T> DoAllocate(Args &&...args) {
    ++allocated_;
    std::unique_ptr<T> object =
        std::make_unique<T>(std::forward<Args>(args)...);
    object->SetHome(this);
    return IntrusivePtr<T>(object.release());
  }

private:
  std::vector<std::unique_ptr<T>> objects_;
  size_t allocated_ = 0;
};

template <typename Derived> class ObjectInPool {
public:
  void IncRef() { count_++; }

  void DecRef() {
    if (--count_ == 0) {
      TakeMeHome();
    }
  }

  size_t RefCount() const { return count_; }

  void SetHome(ObjectPool<Derived> *pool) { home_ = pool; }

private:
  void TakeMeHome() { home_->Release(static_cast<Derived *>(this)); }

private:
  size_t count_ = 0;
  ObjectPool<Derived> *home_;
};

struct PoolableString : ObjectInPool<PoolableString>, std::string {
  using std::string::basic_string;
};

void TestObjectPool() {
  ObjectPool<PoolableString> strs;

  // "Simple"
  {
    strs.Allocate("first");
    REQUIRE(*strs.Allocate("second") == "first");
    REQUIRE(*strs.Allocate("third") == "first");
    REQUIRE(strs.NumAvailable() == 1);
    REQUIRE(strs.NumInUse() == 0);
  }

  // "Reuse"
  {
    {
      auto a = strs.Allocate("first");
      auto b = strs.Allocate("second");
      auto c = strs.Allocate("third");
      REQUIRE(strs.NumAvailable() == 0);
      REQUIRE(strs.NumInUse() == 3);
    }
    REQUIRE(strs.NumAvailable() == 3);
    REQUIRE(strs.NumInUse() == 0);

    {
      auto a = strs.Allocate("aa");
      auto b = strs.Allocate("bb");
      auto c = strs.Allocate("cc");
      REQUIRE(*a == "first");
      REQUIRE(*b == "second");
      REQUIRE(*c == "third");
    }

    REQUIRE(strs.NumAvailable() == 4);
    REQUIRE(strs.NumInUse() == 0);
    auto a = strs.Allocate("aa");
    REQUIRE(strs.NumAvailable() == 3);
    REQUIRE(strs.NumInUse() == 1);
  }
}
