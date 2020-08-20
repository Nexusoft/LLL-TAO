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

* (str)Variable ex. strVar - (str) denotes the type is a std::string.
* (v)Variable ex. vArr     - (v)   denotes the type is a std::vector.
* (map)Variable ex. mapVar - (map) denotes the type is a std::map
* (set)Variable ex. setVar - (set) denotes the type is a std::set


## Namespaces

Always use namespaces to keep the code well ordered and organized. The guide to using namespaces is based around the folder the source files are in. Always declare a new namespace every time you increment a folder, to ensure it is easy to find objects, and there are no duplicate declarations.


## Indentation and Formatting

This section involves how to format the code due to carriage return

* Avoid K&R style brackets. Put opening and closing brackets on their own lines.

* Use tab length 4 and replace tabs with spaces

* We use a maximum line length of 132 characters


## Types to avoid

There are certain types that cause more problems than they solve. Following is a list of types to be warned of using.

* NEVER USE 'double' or 'float' in consensus critical code, only for outputting to the console ex. debug::log(0, "The value is ", double(dValue));

* ALWAYS USE 'cv::softdouble' or 'cv::softfloat' which conform to the IEEE 754 standards using integer arithmetic if floating points
are necessary. These include math functions 'cv::log', 'cv::abs', among others that operate only using integers for hardware portability.


## Security Precautions

* memcpy - this is known to have buffer overflow attack vulnerabilities. Use std::copy instead of memcpy in all instances.


## Pass by Reference

When passing values into functions, ALWAYS pass by reference if the datatype is over 8 bytes long. The reference symbol '&' must
always be on the LEFT HAND SIDE for Pass By Reference, and must ALWAYS use a const specifier.

```
/** Function
 *
 *  This function is responsible for...
 *
 *  @param[in] data The input parameter
 *
 **/
void Function(const Type& data)
{

}
```

## Return by Reference

When returning values by reference, always put the reference symbol '&' on the RIGHT HAND SIDE. This way it is easy to deduce the
intended use of the function parameter. The return by references must NEVER use a const specifier.

```
/** Function
 *
 *  This function is responsible for...
 *
 *  @param[in] data The input parameter
 *  @param[out] return The return value
 *
 **/
void Function(const Type& data, Type &return)
{

}
```


## Public and Private methods

Public methods must always follow 'CamelCase' with the first letter ALWAYS capitalized.

```
class Test
{
public:

    void MethodIsPublic()
    {
    }
}
```

Private and Protected methods must always be lowercase and use underscores between words:

```
class Test
{
private:

    void method_is_private()
    {
    }

protected:

    void method_is_protected()
    {
    }
}
```

This way any developer reading your code will know by the method name whether it is public or private.
