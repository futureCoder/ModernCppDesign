#include <exception>
#include <iostream>

template <class T> struct SingleThreaded;

template <class T, template <class> class CheckingPolicy,
          template <class> class ThreadingModel = SingleThreaded >
class SmartPtr : public CheckingPolicy<T>,
                 public ThreadingModel<T> { /* in the book it's: , public
                                               ThreadingModel<SmartPtr> but it
                                               doesn't work */
                                            // ...
public:
  SmartPtr(T* pointee) : pointee_(pointee ) {}
  
  // 1.11. Compatible and Incompatible Policies
  template< class T1,
	    template <class> class CP1,
	    template <class> class TM1 >
  SmartPtr(const SmartPtr<T1, CP1, TM1>& other)
    : pointee_(other.pointee_), CheckingPolicy<T>(other)
  {}

  ~SmartPtr() { delete pointee_; }
  T *operator->() {
    // typename needed otherwise it could be a static Lock variable in ThreadingModel<SmartPtr>
    // see http://pages.cs.wisc.edu/~driscoll/typename.html
    typename ThreadingModel<SmartPtr>::Lock guard(*this);
    CheckingPolicy<T>::Check(pointee_);
    return pointee_;
  }

protected:
  T *pointee_;
};

// checking policies
//
template <class T> struct NoChecking {
  static void Check(T *) {}
};

template <class T> struct EnforceNotNull {
  class NullPointerException : public std::exception { /* ... */
  };
  static void Check(T * ptr) {
    		if (!ptr) throw NullPointerException();
  }
};

template <class T> struct EnsureNotNull {

public:
  // from the book: "You can even initialize the pointer with a default value by
  // accepting a reference to a pointer" (1.9. Combining Policy Classes)
  static void Check(T *&ptr) { // why reference to pointer?
    if (!ptr) {
      ptr = GetDefaultValue();
    }
  }

private:
  static T s_defaultValue;
  static T *GetDefaultValue() { return &s_defaultValue; }
};
template <class T> T EnsureNotNull<T>::s_defaultValue = T("default");

// Threading policies
//
template <class T> struct SingleThreaded {
  class Lock {
  public:
    Lock(T &obj) { (void)obj; /* ... */ }
  };
};

class Widget {
public:
	Widget(std::string name) : name_(name) {}
  void hello() {
    std::cout << "Hello "
              << name_
              << "\n";
  }
private:
  std::string name_;
};

class ExtendedWidget : public Widget {
  public:
  ExtendedWidget(std::string name) : Widget(name) {}

  void helloExtended() {
	  std::cout << "Extended: \n";
	  Widget::hello();
  }
};


typedef SmartPtr<Widget, NoChecking, SingleThreaded> WidgetPtr;
typedef SmartPtr<Widget, EnsureNotNull, SingleThreaded> EnsuredWidgetPtr;

int main(void) {
  WidgetPtr widgetPtr(new Widget("one"));
	// WidgetPtr widgetPtr(NULL); // this would create a segfault
	widgetPtr->hello();

  EnsuredWidgetPtr ensuredPtr(new Widget("two"));
	//EnsuredWidgetPtr ensuredPtr(NULL); // this would get an error when deleting default value
  ensuredPtr->hello();

  // 1.11. Compatible and Incompatible Policies
  SmartPtr<ExtendedWidget, NoChecking> noCheckExtendedWidget(new ExtendedWidget("three"));
  // this is not working but is explained in the book SmartPtr<Widget, NoChecking> noCheckWidget(noCheckExtendedWidget);
  
  return 0;
}
