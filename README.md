Setup instructions on windws:

0. install boost.

1. setup environment variables to help cmake find boost.  I set:
   BOOST_ROOT = boost install directory = C:\local\boost_1_63_0
   BOOST_LIBRARYDIR = boost library directory = C:\local\boost_1_63_0\lib64-msvc-14.0
   
   finally, add BOOST_ROOT to PATH variable so that executable can find boost dlls at runtime.

2. clean build folder, and cd into it.
   
3. run:

   cmake .. -G "Visual Studio 14 2015 Win64"
   
   type cmake -h to see available options to -G flag.  I choose this one because I have Visual Studio 2015.
