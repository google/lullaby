To support the option of multiple backends for a given interface, we do a bit of
manual "devirtualization" using thunks. The idea is basically:

1) Declare the class API (ie. public function declarations) in the main package.
This class should not have any virtual functions _except_ it's destructor. It
should have a protected default constructor.

2) Define an implementation class that inherits from the API class. The derived
class should declare the _same_ functions as the API class, but remember these
are not virtuals or overrides. (In other words, these functions will shadow the
base class functions.)

3) Implement an Upcast function that takes a pointer to the base class and
returns the derived class.

4) Include the various thunks header files.

5) Provide a factory function that will create an instance of the derived class
and return it as a pointer to the base class.

The way it works is that, for users of the module, they only get a pointer to
the base API class. When calling any function, it will call the "thunk"
implementation for that class which will "upcast" the object to the derived type
and invoke the function with the same name/signature in the derived class. The
advantage to this (over virtual functions) is that there is no dynamic dispatch;
the "thunk" functions can be inlined and so it will be like calling the derived
class directly using the base class pointer.
