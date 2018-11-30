# Comments Guide

Comments in this library should be written as you develop new code. The practice of "code now, comment later" will leave you in a hole of procrastination that will inevitably lead to comment spurts. We would like to see comments describe functions fully, and sub comments describe blocks of code in full sentences.


## Header Files

Comments should be included to support Doxygen building over methods and classes in header files.

```
class Test
{
public:

    /** Describe Data Members. **/
    uint32_t nDataMember;

    /** Function
     *
     *  Function Description
     *
     *  @param[in] nArg The argument for...
     *
     **/
    void Function(uint32_t nArg);
}
```

## Source Files

Inside the corresponding source files, the method description should be non-doxygen, and be a repeat of the function description from the headers.

```
/* Function Description */
void Test::Function(uint32_t nArg)
{
    /* Brief Description of Local Variable. */
    uin32_t nSum = 0;

    /* Brief Description of Process. */
    for(int i = 0; i < 10; i++)
    {
        nSum += nSum;
    }
}
```
