# Coding Style Guide

This library and framework is designed to make your life easier. As such, we want to extend the same services to those that develop the framework. Following are the coding style guides, to ensure that you can learn from the last, and make very clean, well documented code that can be easily maintainable.


## Variable Naming Declarations

The variable names have a prefix that defines the type. This helps us not lose time in searching back to the declaration of said object, keeping the coding process efficient and precise.

### Primitive Types

* (n)Variable ex. nVar - (n) denotes an integer (signed / unsigned).
* (d)Variable ex. dVar - (d) denotes a floating point number.
* (f)Variable ex. fVar - (f) denotes a boolean flag.
* (p)Variable ex. pVar - (p) denotes the variable is a pointer.
* (s)Variable ex. sVar - (s) denotes the variable is static.

### STL Types

* (v)Variable ex. vArr     - (v)   denotes the type is a std::vector.
* (map)Variable ex. mapVar - (map) denotes the type is a std::map
* (set)Variable ex. setVar - (set) denotes the type is a std::set


## Namespaces

Always use namespaces to keep the code well ordered and organized. The guide to using namespaces is based around the folder the source files are in. Always declare a new namespace every time you increment a folder, to ensure it is easy to find objects, and there are no duplicate declarations.


## Indentation and Formatting

This section involes how to format the code due to carriage return

* Avoid K&R style brackets. Put opening and closing brackets on their own lines.

* Use tab length 4 and replace tabs with spaces


## Types to avoid

There are certain types that cause more problems than they solve. Following is a list of types to be warned of using.

* Avoid floating points in objects when possible to avoid floating point precision errors that can occur on certain hardware. One can easily convert from an integer into float by setting the significant figures. ex. unsigned int n = 1000000; printf("%f", n / 1000000.0);


## Security Precautions

* memcpy - this is known to have buffer overflow attack vulnerabilities. Use std::copy instead of memcpy in all instances.
